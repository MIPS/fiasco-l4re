/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <l4/sys/factory>

#include <l4/util/util.h>
#include <l4/re/env>
#include <l4/re/util/env_ns>
#include <l4/re/dataspace>
#include <l4/re/c/util/cap_alloc.h>
#include <l4/re/c/mem_alloc.h>
#include <l4/sys/debugger.h>
#include <l4/sys/cache.h>
#include <l4/vcpu/vcpu>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <pthread.h>
#include <pthread-l4.h>
#include <getopt.h>

#include "vmm.h"
#include "mem_man.h"
#include "loader.h"
#include "params.h"
#include "serial.h"
#include "debug.h"


/*****************************************************************************
 * Build options
 */

#define VM_GUEST_DEFAULT_DEMO
#ifdef VM_GUEST_DEFAULT_DEMO
#define VM_KERN_IMAGE_DEFAULT "rom/guest_hello"
#define VM_IMAGE_0_DEFAULT 0
#endif

#define VM_GUEST_MEM_BASE (0x0)
#define VM_GUEST_MEM_SIZE (8*1024*1024)
#define VM_GUEST_PREMAP_DEFAULT (0)

#undef USE_TIMER_THREAD

/*****************************************************************************
 * prototypes
 */

int parse_config(int argc, char *argv[]);
void handle_upcall(void);
int handle_vmexit(void);
vz_vmm_t* alloc_vmm(void);
void init_vmm(vz_vmm_t *vmm);
void alloc_vm(vz_vmm_t *vmm);
void load_vm(vz_vmm_t *vmm);
void load_vm_image(vz_vmm_t *vmm, const char* vm_image, l4_addr_t *entrypoint);
void init_vm(vz_vmm_t *vmm);
void run_vm(vz_vmm_t *vmm);
void exit_vm(vz_vmm_t *vmm);
void vm_resume(void);
void * timer_thread(void * );

#define error(fmt, ...) \
  do { \
      karma_log(ERROR, "Error: " fmt ,##__VA_ARGS__); \
      exit(1); \
  } while(0)


/*****************************************************************************
 * globals
 */

#define HDL_STACKSIZE (0x2000)
static char hdl_stack[HDL_STACKSIZE];
vz_vmm_t *vmm = NULL;

static int cnt_trig, cnt_handler, cnt_vmexit;

#ifdef USE_TIMER_THREAD
l4_cap_idx_t timer_irq;
pthread_t pthread_timer_thread;
pthread_barrier_t barrier;
#endif


/*****************************************************************************
 * config options
 */

struct __params params;

static struct option long_options[] =
{
    {"debug_level", 1, 0, 'd'},
    {"image", 1, 0, 'i'},        // additional guest ELF files up to MAX_FILES
    {"kernel_image", 1, 0, 'k'}, // guest kernel image; the kernel ELF file specifies the guest entrypoint
    {"kernel_opts", 1, 0, 'o'},  // options passed to guest kernel
    {"mem", 1, 0, 'm'},          // guest memory in MB, i.e. 16 or 16M
    {"premap", 1, 0, 'p'},       // pre-map guest memory instead of using demand paging
    {0, 0, 0, 0}
};

const char *short_options = "d:i:k:m:o:p:";

int parse_config(int argc, char *argv[])
{
    // get parameters
    int opt_index = 0;
    int c;
    int i = 0;

    // set defaults
    params.debug_level = INFO;
    params.mem_size = VM_GUEST_MEM_SIZE;
    params.premap = VM_GUEST_PREMAP_DEFAULT;
    params.kernel_opts = "";

    while( (c = getopt_long(argc, argv, short_options, long_options, &opt_index)) != -1 )
    {
        switch (c)
        {
            case 'd':
                params.debug_level = atoi(optarg);
                break;
            case 'i':
                if (i < MAX_FILES) {
                    params.images[i] = optarg;
                    i++;
                } else
                    karma_log(WARN, "Max number of loadable images reached %d\n", MAX_FILES);
                break;
            case 'k':
                params.kernel_image = optarg;
                break;
            case 'm':
                params.mem_size = atoi(optarg)*1024*1024;
                break;
            case 'o':
                params.kernel_opts = optarg;
                break;
            case 'p':
                params.premap = !!atoi(optarg);
                break;
            default:
                break;
        }
    }

#ifdef VM_GUEST_DEFAULT_DEMO
    if (!params.kernel_image) {
      params.kernel_image = VM_KERN_IMAGE_DEFAULT;
      params.images[0] = VM_IMAGE_0_DEFAULT;
    }
#endif

    if (!params.kernel_image)
      error("Error: VM Guest no kernel image. try --kernel_image=rom/<module name>\n");

    karma_log(INFO, "Using VM Guest mem_size=%d(%#x)\n", params.mem_size, params.mem_size);
    karma_log(INFO, "Using VM Guest %s memory\n", params.premap ? "pre-mapped" : "demand paged");
    karma_log(INFO, "Using VM Guest kernel_image=%s\n", params.kernel_image);
    karma_log(INFO, "Using VM Guest kernel_opts=%s\n", params.kernel_opts);
    i = 0;
    while (params.images[i]) {
      karma_log(INFO, "Using VM Guest image=%s\n", params.images[i]);
      i++;
    }

    return 0;
}


/*****************************************************************************
 * VMM exit handler routines
 */

/*
 * Handle asynchronous guest exceptions and ipc events
 */
void handle_upcall(void) {
  cnt_handler++;
  L4_vm_exit_reason e = static_cast<L4_vm_exit_reason>(vmm->vmstate->exit_reason);

  karma_log(TRACE, "handle_upcall(%d) exit_reason=%s:%x\n",
      cnt_handler, L4_vm_exit_reason_to_str(e), e);

  if (e == L4_vm_exit_reason_irq) {
    karma_log(DEBUG, "handle_upcall(%d) interrupt %d trigger123: %d\n",
        cnt_handler, (int)vmm->vcpu->i.label, cnt_trig);
  } else if (e == L4_vm_exit_reason_ipc) {
    karma_log(DEBUG, "handle_upcall(%d) IPC interrupt %d \n",
        cnt_handler, (int)vmm->vcpu->i.label);
  } else {
    karma_log(DEBUG, "handle_upcall(%d) interrupt %d\n", cnt_handler, (int)vmm->vcpu->i.label);
  }
  vm_resume();
}

/*
 * Handle synchronous guest exits
 */
int handle_vmexit(void) {
  cnt_vmexit++;
  L4_vm_exit_reason e = static_cast<L4_vm_exit_reason>(vmm->vmstate->exit_reason);

  karma_log(TRACE, "[%#08lx] handle_vmexit(%d) exit_reason=%s:%x\n",
      vmm_regs(vmm).ip, cnt_vmexit, L4_vm_exit_reason_to_str(e), e);

  if (RESUME_EXIT == vz_trap_handler(vmm))
    return 1;

  vmm->vmstate->exit_reason = L4_vm_exit_reason_undef;
  return 0;
}

static void _simulate_upcall(l4_addr_t target_stack){
  // call handle_upcall without modifying stack
  asm volatile (
    "  .set push \n"
    "  .set noreorder \n"
    "  move $sp, %0 \n"
    "  jr   %1 \n"
    "       nop \n"
    "  .set pop \n"
    : : "r" (target_stack), "r" (&handle_upcall)
    );
  // never reached
  error("Error jumping to handle_upcall\n");
}

void vm_resume(void) {
  l4_msgtag_t tag;
  long result;

  vmm->vmstate->exit_reason = L4_vm_exit_reason_undef;

  // KYMAXXX if fiasco clears L4_VCPU_F_USER_MODE (as it does for x86)
  // then we must set it again here.
  vmm->vcpu->saved_state |= L4_VCPU_F_USER_MODE;

  L4::Cap<L4::Thread> self;
  tag = self->vcpu_resume_commit(self->vcpu_resume_start());
  result = l4_error(tag);
  if (result == 0) {
    if (handle_vmexit() != 0) {
      // exit vm
      exit_vm(vmm);
      karma_log(INFO, "VM Exit.\n");
      exit(0);
    }
  } else if (result > 0) {
    // we received an ipc
    karma_log(DEBUG, "vm_resume returned %ld ... ipc waiting, simulate irq upcall\n", result);
    vmm->vcpu->i.tag = l4_ipc_wait(l4_utcb(), &vmm->vcpu->i.label, L4_IPC_NEVER);
    if (!l4_msgtag_has_error(vmm->vcpu->i.tag)){
      vmm->vmstate->exit_reason = L4_vm_exit_reason_ipc;
      _simulate_upcall(vmm->vcpu->entry_sp);
    }
  } else {
    error("vm_resume failed: %s (%ld) ... exiting\n", l4sys_errtostr(result), result);
  }

  // sp doesn't increment since handle_vmexit() has no params.
  vm_resume();
}


/*****************************************************************************
 * VMM and VM Initialization routines
 */

vz_vmm_t* alloc_vmm(void) {
  static vz_vmm_t _vmm;
  memset(&_vmm, 0, sizeof(vz_vmm_t));
  return &_vmm;
}

void init_vmm(vz_vmm_t *vmm) {

  vmm->vm_guest_mapbase = VM_GUEST_MEM_BASE;
  vmm->vm_guest_entrypoint = -1UL;
  vmm->host_addr_mapbase = 0;

#ifdef USE_TIMER_THREAD
  int err;
  timer_irq = l4re_util_cap_alloc();
  if (l4_error(l4_factory_create_irq(l4re_env()->factory, timer_irq)))
    error("irq creation failed\n");

  if (l4_error(l4_irq_attach(timer_irq, 123, L4_INVALID_CAP)))
    error("irq attach failed\n");

  err = pthread_barrier_init(&barrier, NULL, 2);
  if (err)
    error("pthread_barrier_init failed %s\n", strerror(err));

  err = pthread_create(&pthread_timer_thread, NULL, timer_thread, NULL);
  if (err)
    error("pthread_create failed %s\n", strerror(err));
#endif

}

void alloc_vm(vz_vmm_t *vmm) {
  long result;
  l4_msgtag_t tag;
  l4_addr_t ext_vmstate;
  void *mapbase;

  if ((result = l4vcpu_ext_alloc(&vmm->vcpu, &ext_vmstate, L4_BASE_TASK_CAP,
          l4re_env()->rm))) {
    error("Failed to allocate VM state memory: %ld\n", result);
  }

  assert(ext_vmstate);
  vmm->vmstate = (l4_vm_vz_state_t *)ext_vmstate;

  L4::Cap<L4::Thread> self;
  tag = self->vcpu_control_ext((l4_addr_t)vmm->vcpu);
  if (l4_error(tag))
    error("Failed to enable virtualization extension for vCPU\n");

  vmm->vcpu->saved_state = L4_VCPU_F_USER_MODE | L4_VCPU_F_IRQ;

  l4_touch_rw(hdl_stack, sizeof(hdl_stack));

  l4_umword_t * stack = (l4_umword_t*)(hdl_stack + HDL_STACKSIZE);
  --stack;
  *stack = 0;
  vmm->vcpu->entry_sp = (l4_umword_t)stack;
  vmm->vcpu->entry_ip = (l4_umword_t)handle_upcall;

  vmm->vmtask = l4re_util_cap_alloc();
  if (l4_is_invalid_cap(vmm->vmtask))
    error("No more caps for VM task.\n");

  l4_debugger_set_object_name(pthread_getl4cap(pthread_self()), "VM Guest");

  tag = l4_factory_create_vm(l4re_env()->factory, vmm->vmtask);
  if (l4_error(tag))
    error("Failed to create new VM task\n");

  vmm->vcpu->user_task = vmm->vmtask;

  /* allocate guest memory */
  result = allocate_mem(params.mem_size, L4RE_MA_PINNED | L4RE_MA_CONTINUOUS, &mapbase);
  if (result)
    error("Failed to allocate VM guest memory. Error: %s\n", l4sys_errtostr(result));

  // KYMAXXX clear all of guest memory or just bss?
  karma_log(INFO, "Clearing VM guest memory ...\n");
  memset(mapbase, 0, params.mem_size);
  karma_log(INFO, " ... done\n");

  vmm->host_addr_mapbase = (l4_addr_t)mapbase;
  vmm->vm_guest_mem_size = params.mem_size;

}

void load_vm_image(vz_vmm_t *vmm, const char* vm_image, l4_addr_t *entrypoint_p) {

  (void)vmm;
  int result;
  L4::Cap<L4Re::Dataspace> ds;
  l4_addr_t ds_addr;
  l4_addr_t entrypoint;

  karma_log(INFO, "Loading guest vm image %s ...\n", vm_image);

  /* map guest image from rom fs into vmm address space */
  L4Re::Util::Env_ns ns;
  if (!(ds = ns.query<L4Re::Dataspace>(vm_image)))
    error("Failed to query vm image %s\n", vm_image);

  if (L4Re::Env::env()->rm()->attach(&ds_addr,
                                     l4_round_page(ds->size()),
                                     L4Re::Rm::Read_only |
                                     L4Re::Rm::Search_addr,
                                     ds)) {
    error("Failed to attach vm image %s\n", vm_image);
  }
 
  /* load guest image into guest memory */
  result = loader_elf_load((l4_uint8_t*)ds_addr, ds->size(),
                           (l4_uint8_t*)vmm->host_addr_mapbase,
                           (l4_addr_t)vmm->vm_guest_mapbase,
                           &entrypoint);
  if (result)
    error("Failed to parse vm ELF image %s @ %08lx. Error: %s\n", vm_image, ds_addr, strerror(result));

  if (L4Re::Env::env()->rm()->detach(ds_addr, &ds))
    error("Failed to detach vm image %s\n", vm_image);

  if (entrypoint_p)
    *entrypoint_p = entrypoint;

  karma_log(INFO, " ... entrypoint 0x%08lx\n", entrypoint);

}

void load_vm(vz_vmm_t *vmm) {

  L4::Cap<L4Re::Dataspace> vm_image_ds;
  L4::Cap<L4Re::Dataspace> vm_guest_mem_ds;
  l4_addr_t entrypoint;

  /*
   * When guest memory is pre-mapped (instead of using demand paging) slowpath
   * VMM tlb exception handling is minimized but startup time is longer.
   */
  if (params.premap) {
    /* map program memory into guest address space */
    l4_addr_t mem_top = map_mem(vmm, vmm->vm_guest_mapbase, vmm->host_addr_mapbase, vmm->vm_guest_mem_size,
        memmap_write, memmap_not_io);
    (void)mem_top; //karma_log(DEBUG, "mem_top=%#08lx\n", mem_top);
  }
  karma_log(INFO, "Guest memory %s 0x%08lx:%08lx -> 0x%08lx:%08lx\n",
      params.premap ? "pre-mapped" : "demand-mapped",
      vmm->vm_guest_mapbase, vmm->vm_guest_mapbase + vmm->vm_guest_mem_size-1,
      vmm->host_addr_mapbase, vmm->host_addr_mapbase + vmm->vm_guest_mem_size-1);

  load_vm_image(vmm, params.kernel_image, &entrypoint);

  vmm->vm_guest_entrypoint = entrypoint;

  int i = 0;
  while (params.images[i]) {
    load_vm_image(vmm, params.images[i], NULL);
    i++;
  }

  // call syncICache using root address
  syncICache(vmm->host_addr_mapbase, vmm->vm_guest_mem_size);

}

void init_vm(vz_vmm_t *vmm) {

  if (vz_init(vmm))
    error("vz_init failed\n");

  assert(vmm->host_addr_mapbase);
  if (serial_init(vmm))
    error("serial_init failed\n");

  assert(vmm->vm_guest_entrypoint != -1UL);
  vmm_regs(vmm).ip = vmm->vm_guest_entrypoint;
}

void exit_vm(vz_vmm_t *vmm) {

#ifdef USE_TIMER_THREAD
  int err;
  void *res;

  err = pthread_cancel(pthread_timer_thread);
  if (err != 0)
    error("pthread_cancel failed %s\n", strerror(err));

  err = pthread_join(pthread_timer_thread, &res);
  if (err != 0)
    error("pthread_join failed %s\n", strerror(err));

  if (res == PTHREAD_CANCELED)
    karma_log(DEBUG, "timer_thread cancelled\n");
  else
    error("timer_thread could not be cancelled!\n");
#endif

  if (vmm->host_addr_mapbase)
    free_mem((void*)vmm->host_addr_mapbase);
}

void run_vm(vz_vmm_t *vmm) {

  karma_log(INFO, "Starting VM Guest kernel_image=%s @ 0x%lx\n",
      params.kernel_image, vmm->vm_guest_entrypoint);

#ifdef USE_TIMER_THREAD
  int err;
  err = pthread_barrier_wait(&barrier);
  if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD)
    error("pthread_barrier_wait failed to wait on barrier\n");
#endif

  // never returns
  vm_resume();
}


#ifdef USE_TIMER_THREAD
/*****************************************************************************
 * Timer thread for injecting irq messages
 */

void * timer_thread(void * data)
{
  (void)data;
  int err;

  err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (err != 0)
    error("pthread_setcancelstate failed %s\n", strerror(err));

  err = pthread_barrier_wait(&barrier);
  if (err != 0 && err != PTHREAD_BARRIER_SERIAL_THREAD)
    error("pthread_barrier_wait failed to wait on barrier\n");

  karma_log(DEBUG, "trigger FIRST\n");
  while(1){
      cnt_trig++;
      karma_log(DEBUG, "# trigger123 %d\n", cnt_trig);
      l4_irq_trigger(timer_irq);
      /* check for exit condition */
      pthread_testcancel();
      l4_sleep(500);
  }
  return NULL;
}
#endif


/*****************************************************************************
 * Main program
 */

int main(int argc, char *argv[])
{
  params.debug_level = INFO;
  karma_log(INFO, "Starting L4/Fiasco Virtual Machine Monitor for MIPS VZ.\n");

  parse_config(argc, argv);

  vmm = alloc_vmm();
  init_vmm(vmm);

  alloc_vm(vmm);
  load_vm(vmm);
  init_vm(vmm);
  l4_sleep(2000);

  // never returns
  run_vm(vmm);

  return 0;
}
