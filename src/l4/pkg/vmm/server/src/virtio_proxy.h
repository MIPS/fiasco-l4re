#pragma once

#include <l4/sys/capability>
#include <l4/sys/meta>

#include <l4/re/dataspace>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>

#include <l4/re/env>
#include <l4/re/rm>

#include <l4/cxx/ipc_stream>
#include <l4/cxx/ipc_server>

#include "gic.h"
#include "virtio.h"
#include "arm_mmio_device.h"
#include "vm_ram.h"

namespace Virtio {

class Mmio_remote : public L4::Kobject_t<Mmio_remote, L4::Kobject>
{
  L4_KOBJECT(Mmio_remote)

public:
  int read(unsigned reg, char size, l4_uint32_t *value)
  {
    L4::Ipc::Iostream ios(l4_utcb());
    ios << L4::Opcode(0) << reg << size;
    int err = l4_error(ios.call(cap(), Protocol));
    if (err < 0)
      return err;

    ios >> *value;
    return err;
  }

  int write(unsigned reg, char size, l4_uint32_t value)
  {
    L4::Ipc::Iostream ios(l4_utcb());
    ios << L4::Opcode(1) << reg << size << value;
    return l4_error(ios.call(cap(), Protocol));
  }

  int init(L4::Cap<L4::Irq> kick_guest_irq,
           L4::Cap<L4Re::Dataspace> ds, l4_addr_t ds_base,
           L4::Cap<L4::Irq> kick_host_irq, L4::Cap<L4Re::Dataspace> config_ds)
  {
    L4::Ipc::Iostream ios(l4_utcb());
    ios << L4::Opcode(2) << ds_base << kick_guest_irq << ds;
    ios << L4::Ipc::Small_buf(kick_host_irq) << L4::Ipc::Small_buf(config_ds);

    return l4_error(ios.call(cap(), Protocol));
  }
};



class Proxy_dev : public Vmm::Mmio_device_t<Proxy_dev>,
                  private L4::Server_object
{
private:

  unsigned _current_q;

  // -- network specific
  Ring _txq;
  // -------------------

  Vmm::Vm_ram *_iommu;

public:
  Proxy_dev(Vmm::Vm_ram *iommu, Gic::Dist *gic, unsigned irq)
  : _iommu(iommu), _gic(gic), _irq(irq) {}

  l4_uint32_t read(unsigned reg, char size, Vmm::Cpu *)
  {
    switch (reg >> 2)
      {
      case 0: return *reinterpret_cast<l4_uint32_t const *>("virt");
      case 1: return 1;
      case 2: return device_config->device;
      case 3: return device_config->vendor;
      case 4: return device_config->host_features.raw;
      case 24: return 1;
               if (0)
                 {
                   l4_uint32_t tmp = device_config->irq_status;
                   device_config->irq_status = 0;
                   return tmp;
                 }
      case 28: return device_config->status.raw;
      default: break;
      }
    l4_uint32_t res;
    L4Re::chksys(host->read(reg, size, &res));
    return res;
  }

  void write(unsigned reg, char size, l4_uint32_t value, Vmm::Cpu *)
  {
    switch (reg >> 2)
      {
      case 20:
        // -- network specific
        _txq.used->flags.no_notify() = true;
        // -------------------
        kick_host->trigger();
        return; /* do not forward synchronously */

      case 12:
        _current_q = value;
        break;

      case 14:
        if (_current_q == 1)
          _txq.num = value;
        break;

      case 15:
        if (_current_q == 1)
          _txq.align = value;
        break;


      case 16:
        // -- network specific
        if (_current_q == 1)
          _txq.setup(_iommu->access(Virtio::Ptr<void>(value * device_config->page_size)));
        // -------------------
        break;

      case 25:
        break;

      default:
        if (0)
          printf("proxy write: reg = %d\n", reg);
        break;
      }
    L4Re::chksys(host->write(reg, size, value));
  }

  template<typename REG>
  void register_obj(REG *registry, L4::Cap<Mmio_remote> host,
                    L4::Cap<L4Re::Dataspace> ram, l4_addr_t ram_base)
  {
    this->host = host;
    kick_host = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Irq>(),
                             "allocate HOST IRQ cap");
    device_config_ds
      = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>());
    L4Re::chksys(host->init(L4Re::chkcap(registry->register_irq_obj(this)),
                            ram, ram_base, kick_host, device_config_ds));

    device_config = 0;
    L4Re::chksys(L4Re::Env::env()->rm()->attach(&device_config, L4_PAGESIZE,
                                                L4Re::Rm::Search_addr,
                                                device_config_ds));
  }

  int dispatch(l4_umword_t, L4::Ipc::Iostream &)
  {
    _gic->inject_spi(_irq - 32, vmm_current_cpu_id);
    return -L4_ENOREPLY;
  }

private:
  Gic::Dist *_gic;
  unsigned _irq;

  L4::Cap<L4::Irq> kick_host;
  L4::Cap<Mmio_remote> host;
  L4::Cap<L4Re::Dataspace> device_config_ds;
  Virtio::Dev_config *device_config;
};
}
