
#include "virtio_proxy.h"
#include "virtio_console.h"
#include "arm_mmio_device.h"
#include "arm_hyp.h"
#include "virt_bus.h"
#include "vm_memmap.h"
#include "vm_ram.h"
#include "virtio.h"
#include "debug.h"

#include <cassert>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <iostream>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/re/debug>

#include <l4/sys/thread>
#include <l4/sys/factory>
#include <l4/sys/task>
#include <l4/sys/vcpu.h>
#include <l4/sys/compiler.h>

#include <l4/cxx/utils>
#include <l4/cxx/ipc_stream>
#include <l4/cxx/ipc_server>
#include <l4/cxx/static_container>

#include <l4/vbus/vbus>

#include <l4/sys/kdebug.h>
#include <l4/sys/ktrace.h>
#include <l4/sys/debugger.h>
#include <l4/re/dataspace>
#include <l4/l4re_vfs/backend>


extern "C" {
#include <libfdt.h>
}

class Vm_ram : public Vmm::Vm_ram
{
public:
  Vm_ram(L4::Cap<L4Re::Dataspace> ram, l4_addr_t vm_base = ~0UL)
  : _ram(ram)
  {
    Dbg info(Dbg::Info);

    _vm_start = vm_base;
    _size = ram->size();

    if (vm_base != ~0UL && (vm_base & ~0xf0000000))
      alignment_message();

    l4_size_t phys_size;
    l4_addr_t phys_ram;

    int err = ram->phys(0, phys_ram, phys_size);

    if (err < 0 || phys_size < _size)
      {
        info.printf("RAM dataspace not contiguous, should not use DMA w/o IOMMU\n");
        if (err >= 0 && _vm_start == ~0UL)
          {
            _vm_start = phys_ram;
            _ident = true;
          }
      }
    else
      {
        _cont = true;
        if (_vm_start == ~0UL)
          {
            if (phys_ram & ~0xf0000000)
              alignment_message();

            _vm_start = phys_ram;
            _ident = true;
          }
      }

    info.printf("RAM: @ %lx size=%zx (%c%c)\n",
                _vm_start, _size, _cont ? 'c' : '-', _ident ? 'i' : '-');

    _local_start = 0;
    L4Re::chksys(L4Re::Env::env()->rm()->attach(&_local_start, _size,
                                                L4Re::Rm::Search_addr
                                                | L4Re::Rm::Eager_map,
                                                ram, 0,
                                                L4_SUPERPAGESHIFT));
    _local_end = _local_start + _size;
    info.printf("RAM: VMM mapping @ %lx size=%zx\n", _local_start, _size);
    if (_vm_start != ~0UL)
      {
        _offset = _local_start - _vm_start;
        info.printf("RAM: VM offset=%lx\n", _offset);
      }
  }

  void alignment_message()
  {
    Dbg(Dbg::Info).printf(
        "WARNING: Guest memory not 256MB aligned!\n"
        "         If you run Linux as a guest, you might hit a bug\n"
        "         in the arch/arm/boot/compressed/head.S code\n"
        "         that misses an ISB after code has been relocated.\n"
        "         According to the internet a fix for this issue\n"
        "         is floating around.\n");
  }

  L4::Cap<L4Re::Dataspace> ram() const { return _ram; }

private:
  L4::Cap<L4Re::Dataspace> _ram;
};


__thread unsigned vmm_current_cpu_id;

static cxx::Static_container<Vm_ram> vm_ram;
static cxx::Static_container<Gic::Dist> gic_dist;
static cxx::Static_container<Ds_handler> ram_h;
static Vm_mem mmio;
static L4Re::Util::Object_registry registry;
static L4::Cap<L4::Task> vm_task;
static L4::Cap<L4Re::Dataspace> io_ds;


static Virtio::Ptr<void>
load_file(char const *name, l4_addr_t offset, l4_size_t *_size = 0)
{
  Dbg info(Dbg::Info, "file");

  info.printf("load: %s -> %lx\n", name, offset);
  int fd = open(name, O_RDONLY);
  if (fd < 0)
    {
      Err().printf("could not open file: %s:", name); perror("");
      return Virtio::Ptr<void>::Invalid;
    }

  cxx::Ref_ptr<L4Re::Vfs::File> file = L4Re::Vfs::vfs_ops->get_file(fd);
  if (!file)
    {
      Err().printf("bad file descriptor: %s\n", name);
      errno = EBADF;
      return Virtio::Ptr<void>::Invalid;
    }

  L4::Cap<L4Re::Dataspace> f = file->data_space();
  if (!f)
    {
      Err().printf("could not get data space for %s\n", name);
      errno = EINVAL;
      return Virtio::Ptr<void>::Invalid;
    }

  l4_size_t size = f->size();
  info.printf("copy in: %s -> %lx-%lx\n", name, offset, offset + size);

  L4Re::chksys(vm_ram->ram()->copy_in(offset, f, 0, size), "copy in");
  close(fd);
  if (_size)
    *_size = size;

  return Virtio::Ptr<void>(offset + vm_ram->vm_start());
}

static bool handle_mmio(l4_uint32_t pfa, l4_vcpu_state_t *vcpu)
{
  Vm_mem::const_iterator f = mmio.find(pfa);
  if (f == mmio.end())
    return false;

  (*f).second->access(pfa, pfa - (*f).first.start, vcpu, vm_task,
                      (*f).first.start, (*f).first.end);
  return true;
}




extern "C" void vcpu_entry(l4_vcpu_state_t *vcpu);

asm
(
 "vcpu_entry:                     \n"
 "  mov    r6, sp                 \n"
 "  bic    sp, #7                 \n"
 "  sub    sp, sp, #16            \n"
 "  mrc    p15, 0, r5, c13, c0, 2 \n"
 "  stmia  sp, {r4, r5, r6, lr}   \n"
 "  bl     c_vcpu_entry           \n"
 "  movw   r2, #0xf803            \n"
 "  movt   r2, #0xffff            \n"
 "  mov    r3, #0                 \n"
 "  mov    r5, sp                 \n"
 "  ldmia  r5, {r4, r6, sp, lr}   \n"
 "  mcr    p15, 0, r6, c13, c0, 2 \n"
 "  mov    pc, #" L4_stringify(L4_SYSCALL_INVOKE) " \n"
);

extern "C" l4_msgtag_t c_vcpu_entry(l4_vcpu_state_t *vcpu);

static void handle_ipc(l4_umword_t label)
{
  L4::Ipc::Iostream ios(l4_utcb());
  long r = registry.dispatch(label, ios);
  if (r != -L4_ENOREPLY)
    ios.reply(L4_IPC_SEND_TIMEOUT_0, r);
}

l4_msgtag_t c_vcpu_entry(l4_vcpu_state_t *vcpu)
{
  l4_utcb_t *utcb = *(l4_utcb_t **)((char *)vcpu + 0x200);
  asm volatile("mcr p15, 0, %0, c13, c0, 2" : : "r"(utcb));
  Vmm::Arm::Hsr hsr(vcpu->r.err);

  if (L4_UNLIKELY(l4_utcb() != utcb))
    enter_kdebug("vCPU invalid");

  switch (hsr.ec())
    {
    case 0x20: // insn abt
      // fall through
    case 0x24: // data abt
        {
          l4_addr_t pfa = vcpu->r.pfa;

          if (handle_mmio(pfa, vcpu))
            break;

          long res;
#if MAP_OTHER
          res = io_ds->map(pfa, L4Re::Dataspace::Map_rw, pfa,
                           l4_trunc_page(pfa), l4_round_page(pfa + 1),
                           vm_task);
          if (res < 0)
            {
              Err().printf("cannot handle VM memory access @ %lx ip=%lx\n",
                           pfa, vcpu->r.ip);
              enter_kdebug("STOP");
            }
#else

          l4_addr_t local_addr = 0;
          L4Re::Env const *e = L4Re::Env::env();
          res = e->rm()->reserve_area(&local_addr, L4_PAGESIZE,
                                      L4Re::Rm::Search_addr);
          if (res < 0)
            {
              Err().printf("cannot handle VM memory access @ %lx ip=%lx "
                           "(VM allocation failure)\n",
                           pfa, vcpu->r.ip);
              enter_kdebug("STOP");
              break;
            }

          res = io_ds->map(pfa, L4Re::Dataspace::Map_rw, local_addr,
                           l4_trunc_page(local_addr),
                           l4_round_page(local_addr + 1));
          if (res < 0)
            {
              Err().printf("cannot handle VM memory access @ %lx ip=%lx "
                           "(getting)\n",
                           pfa, vcpu->r.ip);
              enter_kdebug("STOP");
              break;
            }

          res = l4_error(vm_task->map(e->task(),
                                      l4_fpage(local_addr,
                                               L4_PAGESHIFT, L4_FPAGE_RW),
                                      l4_trunc_page(pfa)));
          if (res < 0)
            {
              Err().printf("cannot handle VM memory access @ %lx ip=%lx "
                           "(map to VM failure)\n",
                           pfa, vcpu->r.ip);
              enter_kdebug("STOP");
              break;
            }

#endif /* MAP_OTHER */
          break;
        }

    case 0x3d: // VIRTUAL PPI
      switch (hsr.svc_imm())
        {
        case 0: // VGIC IRQ
          gic_dist->handle_maintenance_irq(vmm_current_cpu_id);
          break;
        case 1: // VTMR IRQ
          gic_dist->inject_local(27, vmm_current_cpu_id);
          break;
        default:
          Err().printf("unknown virtual PPI: %d\n", (int)hsr.svc_imm());
          break;
        }
      break;

    case 0x3f: // IRQ
      handle_ipc(vcpu->i.label);
      break;

    case 0x01: // WFI, WFE
      if (hsr.wfe_trapped()) // WFE
        {
          // yield
        }
      else // WFI
        {
          if (gic_dist->schedule_irqs(vmm_current_cpu_id))
            {
              vcpu->r.ip += 2 << hsr.il();
              break;
            }

          l4_timeout_t to = L4_IPC_NEVER;
          Vmm::Arm::State *vm = Vmm::Arm::vm_state(vcpu);
          l4_utcb_t *u = l4_utcb();

          if ((vm->cntv_ctl & 3) == 1) // timer enabled and not masked
            {
              // calculate the timeout based on the VTIMER values !
              l4_uint64_t cnt, cmp;
              asm volatile ("mrrc p15, 1, %Q0, %R0, c14" : "=r"(cnt));
              asm volatile ("mrrc p15, 3, %Q0, %R0, c14" : "=r"(cmp));

              if (cmp <= cnt)
                {
                  vcpu->r.ip += 2 << hsr.il();
                  break;
                }

              l4_uint64_t diff = (cmp - cnt) / 24;
              if (0)
                printf("diff=%lld\n", diff);
              l4_rcv_timeout(l4_timeout_abs_u(l4_kip_clock(l4re_kip()) + diff, 8, u), &to);
            }

          l4_umword_t src;
          l4_msgtag_t res = l4_ipc_wait(u, &src, to);
          if (!res.has_error())
            handle_ipc(src);

          // skip insn
          vcpu->r.ip += 2 << hsr.il();
        }
      break;

    case 0x05: // MCR/MRC CP 14

      if (   hsr.mcr_opc1() == 0
          && hsr.mcr_crn() == 0
          && hsr.mcr_crm() == 1
          && hsr.mcr_opc2() == 0
          && hsr.mcr_read()) // DCC Status
        {
          // printascii in Linux is doing busyuart which wants to see a
          // busy flag to quit its loop while waituart does not want to
          // see a busy flag; this little trick makes it work
          static l4_umword_t flip;
          flip ^= 1 << 29;
          vcpu->r.r[hsr.mcr_rt()] = flip;
        }
      else if (   hsr.mcr_opc1() == 0
               && hsr.mcr_crn() == 0
               && hsr.mcr_crm() == 5
               && hsr.mcr_opc2() == 0) // DCC Get/Put
        {
          if (hsr.mcr_read())
            vcpu->r.r[hsr.mcr_rt()] = 0;
          else
            putchar(vcpu->r.r[hsr.mcr_rt()]);
        }
      else
        printf("%08lx: %s p14, %d, r%d, c%d, c%d, %d (hsr=%08lx)\n",
               vcpu->r.ip, hsr.mcr_read() ? "MRC" : "MCR",
               (unsigned)hsr.mcr_opc1(),
               (unsigned)hsr.mcr_rt(),
               (unsigned)hsr.mcr_crn(),
               (unsigned)hsr.mcr_crm(),
               (unsigned)hsr.mcr_opc2(),
               (l4_umword_t)hsr.raw());

      vcpu->r.ip += 2 << hsr.il();
      break;

    default:
      Err().printf("unknown trap: err=%lx ec=0x%x ip=%lx\n",
                   vcpu->r.err, (int)hsr.ec(), vcpu->r.ip);
      if (0)
        enter_kdebug("Unknown trap");
      break;
    }
  L4::Cap<L4::Thread> myself;

#if 1
  while (vcpu->sticky_flags & L4_VCPU_SF_IRQ_PENDING)
    {
      l4_umword_t src;
      l4_msgtag_t res = l4_ipc_wait(utcb, &src, L4_IPC_BOTH_TIMEOUT_0);
      if (!res.has_error())
        handle_ipc(src);
    }
#endif

  gic_dist->schedule_irqs(vmm_current_cpu_id);

  return myself->vcpu_resume_start(utcb);
}


static Vmm::Mmio_device *
create_virtio_cons(Gic::Dist *gic, unsigned irq)
{
  Dbg info;
  info.printf("Create virtual console\n");

  Virtio_console *cons = new Virtio_console(vm_ram, L4Re::Env::env()->log(),
                                            gic, irq);
  cons->register_obj(&registry);
  return cons;
}

static Vmm::Mmio_device *
create_virtio_net(Gic::Dist *gic, unsigned irq)
{
  Dbg info;
  info.printf("Create virtual net\n");

  Virtio::Proxy_dev *net = new Virtio::Proxy_dev(vm_ram, gic, irq);
  net->register_obj(&registry,
                    L4Re::Env::env()->get_cap<Virtio::Mmio_remote>("net"),
                    vm_ram->ram(), vm_ram->vm_start());

  return net;
}


static
void const *fdt_check_prop(void const *fdt, int node, char const *name, int minlen)
{
  int _len;
  void const *prop = fdt_getprop(fdt, node, name, &_len);
  if (!prop)
    {
      char buf[256];
      if (fdt_get_path(fdt, node, buf, sizeof(buf)) < 0)
        buf[0] = 0;
      Err().printf("could net get '%s' property of %s: %d\n", name, buf, _len);
      throw("abort");
    }

  if (_len < minlen)
    {
      char buf[256];
      if (fdt_get_path(fdt, node, buf, sizeof(buf)) < 0)
        buf[0] = 0;
      Err().printf("'%s' property of %s is too small (%d need %d)\n", name, buf, _len, minlen);
      throw("abort");
    }

  return prop;
}

static int config_virtio_device(void *dt, int startoffset = -1)
{
  Dbg info;
  Dbg warn(Dbg::Warn);

  int node = fdt_node_offset_by_compatible(dt, startoffset, "virtio,mmio");
  if (node < 0)
    return node;

  int type_len;
  char const *type = (char const *)fdt_getprop(dt, node, "l4vmm,virtiotype", &type_len);
  if (!type)
    {
      warn.printf("'l4vmm,virtiotype' property missing from virtio device, disable it\n");
      fdt_setprop_string(dt, node, "status", "disabled");
      return node;
    }


  fdt32_t const *prop = (fdt32_t const *)fdt_check_prop(dt, node, "reg", sizeof(uint32_t) * 2);

  uint32_t base = fdt32_to_cpu(prop[0]);
  uint32_t size = fdt32_to_cpu(prop[1]);

  prop = (fdt32_t const *)fdt_check_prop(dt, node, "interrupts", sizeof(uint32_t) * 3);
  if (fdt32_to_cpu(prop[0]) != 0)
    {
      Err().printf("virtio devices must use SPIs\n");
      throw("abort");
    }

  uint32_t irq = fdt32_to_cpu(prop[1]) + 32;
  info.printf("VIRTIO: @ %x %x (GSI=%d) type=%.*s\n", base, size, irq, type_len, type);

  Vmm::Mmio_device *dev = 0;
  if (fdt_stringlist_contains(type, type_len, "console"))
    dev = create_virtio_cons(gic_dist.get(), irq);
  else if (fdt_stringlist_contains(type, type_len, "net"))
    dev = create_virtio_net(gic_dist.get(), irq);

  if (!dev)
    {
      Err().printf("invalid virtio device type: '%.*s', disable device\n", type_len, type);
      fdt_setprop_string(dt, node, "status", "disabled");
      return node;
    }

  mmio[Region::ss(base, size)] = dev;

  return node;
}

static void config_gic(void const *dt, Vmm::Virt_bus *vm_bus)
{
  Dbg info;
  // -------------------------------------------------------------------
  // map the virtual GIC distributor and the GIC CPU interface to the VM

  int node = fdt_node_offset_by_compatible(dt, -1, "arm,cortex-a9-gic");
  if (node < 0)
    {
      node = fdt_node_offset_by_compatible(dt, -1, "arm,cortex-a15-gic");
      if (node < 0)
        {
          Err().printf("could not find GIC in device tree: %d\n", node);
          throw("abort");
        }
    }

  fdt32_t const *prop = (fdt32_t const *)fdt_check_prop(dt, node, "reg", sizeof(uint32_t) * 4);

  uint32_t base = fdt32_to_cpu(prop[0]);
  uint32_t size = fdt32_to_cpu(prop[1]);

  info.printf("GICD: @ %x %x\n", base, size);

  // 4 * 32 spis, 2 cpus
  gic_dist.construct(4, 2);

  // attach GICD to VM
  mmio[Region::ss(base, size)] = gic_dist;

  base = fdt32_to_cpu(prop[2]);
  size = fdt32_to_cpu(prop[3]);

  info.printf("GICC: @ %x %x\n", base, size);

  vm_bus->map_mmio(mmio, "GICC", "arm-gicc", base);
}

static char const *const options = "+k:d:r:c:";
static struct option const loptions[] =
  {
    { "kernel",   1, NULL, 'k' },
    { "dtb",      1, NULL, 'd' },
    { "ramdisk",  1, NULL, 'r' },
    { "cmdline",  1, NULL, 'c' },
    { 0, 0, 0, 0}
  };

static int run(int argc, char *argv[])
{
  L4Re::Env const *e = L4Re::Env::env();
  Dbg info;
  Dbg warn(Dbg::Warn);

  Dbg::set_level(0xffff);

  info.printf("Hello out there.\n");

  char const *cmd_line     = nullptr;
  char const *kernel_image = "rom/zImage";
  char const *device_tree  = "rom/virt.dtb";
  char const *ram_disk     = nullptr;

  int opt;
  while ((opt = getopt_long(argc, argv, options, loptions, NULL)) != -1)
    {
      switch (opt)
        {
        case 'c': cmd_line     = optarg; break;
        case 'k': kernel_image = optarg; break;
        case 'd': device_tree  = optarg; break;
        case 'r': ram_disk     = optarg; break;
        default:
          Err().printf("unknown command-line option\n");
          return 1;
        }
    }

  // create VM task (VMs guest physical memory)
  vm_task = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Task>(),
                         "allocate vm cap");
  L4Re::chksys(e->factory()->create_task(vm_task, l4_fpage_invalid()),
               "allocate vm");

  l4_debugger_set_object_name(vm_task.cap(), "vm-task");

  // get RAM data space and attach it to our (VMMs) address space
  L4::Cap<L4Re::Dataspace> ram
    = L4Re::chkcap(e->get_cap<L4Re::Dataspace>("ram"), "ram dataspace cap");
  vm_ram.construct(ram, ~0UL);

  // attach RAM to VM
  mmio[Region::ss(vm_ram->vm_start(), vm_ram->size())] = new Ds_handler(ram, vm_ram->local_start());

  // create VM BUS connection to IO for GICC pass through and device pass through
  Vmm::Virt_bus vm_bus(L4Re::chkcap(e->get_cap<L4vbus::Vbus>("vm_bus"),
                                    "Error getting vm_bus capability",
                                    -L4_ENOENT));
  io_ds = vm_bus.io_ds();


  // load device tree
  Virtio::Ptr<void> dt_vm = load_file(device_tree, 0x100);
  if (!dt_vm.is_valid())
    return 1;

  void *dt = vm_ram->access(dt_vm);

  if (fdt_check_header(dt) < 0)
    {
      printf("ERROR: %s is not a device tree\n", device_tree);
      return 1;
    }

  fdt_set_totalsize(dt, fdt_totalsize(dt) + 0x200);


  // -------------------------------------------------------------------
  // fill in memory node in the device tree

  int mem_nd = fdt_path_offset(dt, "/memory");

  if (mem_nd < 0)
    {
      Err().printf("cannot find '/memory' node: %d\n", mem_nd);
      return 1;
    }

  int err = fdt_setprop_u32(dt, mem_nd, "reg", vm_ram->vm_start());
  if (err < 0)
    {
      Err().printf("cannot set property for fdt node: %d\n", err);
      return 1;
    }

  err = fdt_appendprop_u32(dt, mem_nd, "reg", vm_ram->size());
  if (err < 0)
    {
      Err().printf("cannot set property for fdt node: %d\n", err);
      return 1;
    }

  config_gic(dt, &vm_bus);

  // create all virtio devices for the VM
  for (int node = -1;;)
    {
      node = config_virtio_device(dt, node);
      if (node < 0)
        break;
    }

  // pass all HW IRQs allowed by the vm_bus to the VM
  Irq_svr *irqs = new Irq_svr;
  unsigned nr_irqs = vm_bus.num_irqs();
  for (unsigned i = 0; i < nr_irqs; ++i)
    {
      if (vm_bus.map_irq(&registry, gic_dist, irqs, i, i + 32))
        irqs = new Irq_svr;
    }

  // load linux kernel image
  Virtio::Ptr<void> kernel_vm = load_file(kernel_image, 0x208000);
  if (!kernel_vm.is_valid())
    return 1;

  // load the ramdisk
  if (ram_disk)
    {
      info.printf("load ramdisk image %s\n", ram_disk);

      l4_size_t size;
      Virtio::Ptr<void> initrd = load_file(ram_disk, 0x2000000, &size);
      if (!initrd.is_valid())
        return 1;

      int node = fdt_path_offset(dt, "/chosen");
      if (node < 0)
        {
          Err().printf("cannot find '/chosen' node in device tree, cannot register ram disk\n");
          return 1;
        }

      if (int e = fdt_setprop_u32(dt, node, "linux,initrd-start", initrd.get()) < 0)
        {
          Err().printf("cannot set 'linux,initrd-start' property: %d\n", e);
          return 1;
        }

      if (int e = fdt_setprop_u32(dt, node, "linux,initrd-end",
                                  initrd.get() + size) < 0)
        {
          Err().printf("cannot set 'linux,initrd-start' property: %d\n", e);
          return 1;
        }
    }

  if (cmd_line)
    {
      info.printf("setting command line in device tree to: '%s'\n", cmd_line);

      int node = fdt_path_offset(dt, "/chosen");
      if (node < 0)
        {
          Err().printf("cannot find '/chosen' node in device tree, cannot set command line\n");
          return 1;
        }

      if (int e = fdt_setprop_string(dt, node, "bootargs", cmd_line) < 0)
        {
          Err().printf("cannot set 'bootargs' property: %d\n", e);
          return 1;
        }
    }

  l4_addr_t vcpu = 0x10000000;
  L4Re::chksys(e->rm()->reserve_area(&vcpu, L4_PAGESIZE,
                                     L4Re::Rm::Search_addr));
  L4Re::chksys(e->task()->add_ku_mem(l4_fpage(vcpu, L4_PAGESHIFT,
                                              L4_FPAGE_RWX)),
               "kumem alloc");

  l4_vcpu_state_t *vcpu_state = (l4_vcpu_state_t *)vcpu;
  *(l4_utcb_t **)(vcpu + 0x200) = l4_utcb();
  Vmm::Arm::State *vm = Vmm::Arm::vm_state(vcpu_state);

  L4::Cap<L4::Thread> myself;
  if (l4_error(myself->vcpu_control_ext(vcpu)))
    {
      Err().printf("FATAL: Could not create vCPU. "
                   "Running HYP-enabled kernel?\n");
      return 1;
    }
  printf("VCPU mapped @ %lx and enabled\n", vcpu);

  vcpu_state->r.flags     = 0x00000013;
  vcpu_state->r.sp        = 0;
  vcpu_state->r.r[0]      = 0;
  vcpu_state->r.r[1]      = ~0UL;
  vcpu_state->r.r[2]      = dt_vm.get();
  vcpu_state->r.r[3]      = 0;
  vcpu_state->r.ip        = kernel_vm.get();
  vcpu_state->user_task   = vm_task.cap();
  vcpu_state->saved_state =   L4_VCPU_F_FPU_ENABLED
                            | L4_VCPU_F_USER_MODE
                            | L4_VCPU_F_IRQ
                            | L4_VCPU_F_PAGE_FAULTS
                            | L4_VCPU_F_EXCEPTIONS;
  vcpu_state->entry_ip    = (l4_umword_t)&vcpu_entry;

  vm->vm_regs.hcr &= ~(1 << 27);
  vm->vm_regs.hcr |= 1 << 13;
  gic_dist->set_cpu(0, &vm->gic);
  vmm_current_cpu_id = 0;

  myself->vcpu_resume_commit(myself->vcpu_resume_start());

  Err().printf("ERROR: we must never reach this....\n");
  return 0;
}

int main(int argc, char *argv[])
{
  try
    {
      return run(argc, argv);
    }
  catch (L4::Runtime_error &e)
    {
      Err().printf("FATAL: %s %s\n", e.str(), e.extra_str());
    }
  return 1;
}


