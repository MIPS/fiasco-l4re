/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * (c) 2009 Technische Universität Dresden
 * This file is part of TUD:OS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/sys/ipc.h>
#include <l4/sys/thread>
#include <l4/sys/factory>
#include <l4/sys/scheduler>
#include <l4/sys/utcb.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/kumem_alloc>
#include <l4/sys/debugger.h>
#include <l4/vcpu/vcpu>
#include <l4/cxx/iostream>

#include <l4/re/error_helper>

#include <l4/sys/task>
#include <l4/sys/irq>
#include <l4/sys/vcpu.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#define RUN_VCPU_IN_VZ_GUESTVM 1
#include <l4/sys/vm>

using L4Re::chksys;
using L4Re::chkcap;

static L4::Cap<L4::Irq> irq;

static char thread_stack[8 << 10];
static char hdl_stack[8 << 10];

#if !defined(RUN_VCPU_IN_VZ_GUESTVM)
static L4::Cap<L4::Task> vcpu_task;
static L4vcpu::Vcpu *vcpu;
#else
static L4::Cap<L4::Vm> vcpu_task; // Vm object is extension of Task object
static L4vcpu::Vcpu *vcpu;
static l4_vm_vz_state *vmstate;
const l4_addr_t vm_run_offset = 0x80000000;
#endif

const l4_addr_t super_code_map_addr = 0x10000;
extern char my_super_code[];
extern char my_super_code_excp[];
extern char my_super_code_excp_after[];

#if defined(ARCH_x86) || defined(ARCH_amd64)

static unsigned long fs;
static unsigned long ds;

asm
(
  ".p2align 12                      \t\n"
  ".global my_super_code            \t\n"
  ".global my_super_code_excp       \t\n"
  ".global my_super_code_excp_after \t\n"
  "my_super_code:                   \t\n"
  "1: add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "my_super_code_excp:              \t\n"
  "   ud2a                          \t\n"
  "my_super_code_excp_after:        \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   add $4, %eax                  \t\n"
  "   jmp 1b                        \t\n"
  );


static void setup_user_state_arch(L4vcpu::Vcpu *v)
{
  asm volatile ("mov %%fs, %0" : "=r"(fs));
  asm volatile ("mov %%ds, %0" : "=r"(ds));
#ifndef __amd64__
  v->r()->gs = ds;
  v->r()->fs = ds;
  v->r()->es = ds;
  v->r()->ds = ds;
#endif
  v->r()->ss = ds;
}

static void handler_prolog()
{
  asm volatile ("mov %0, %%es \t\n"
                "mov %0, %%ds \t\n"
                "mov %1, %%fs \t\n"
                : : "r"(ds), "r"(fs));
}

#elif defined(ARCH_arm)
asm
(
  ".p2align 12                      \t\n"
  ".global my_super_code            \t\n"
  ".global my_super_code_excp       \t\n"
  ".global my_super_code_excp_after \t\n"
  "my_super_code:                   \t\n"
  "1: add r0, r0, #4                \t\n"
  "   add r0, r0, #4                \t\n"
  "   add r0, r0, #4                \t\n"
  "   add r0, r0, #4                \t\n"
  "   add r0, r0, #4                \t\n"
  "   add r0, r0, #4                \t\n"
  "   add r0, r0, #4                \t\n"
  "   add r0, r0, #4               \t\n"
  "my_super_code_excp:              \t\n"
  "   swi 0                         \t\n"
  "my_super_code_excp_after:        \t\n"
  "   mrc 8, 0, r0, cr7, cr14       \t\n"
  "   add r0, r0, #4                \t\n"
  "   add r0, r0, #4                \t\n"
  "   add r0, r0, #4                \t\n"
  "   b 1b                          \t\n"
  );


static void setup_user_state_arch(L4vcpu::Vcpu *) { }
static void handler_prolog() {}

#elif defined(ARCH_ppc32)
asm
(
  ".p2align 12                      \t\n"
  ".global my_super_code            \t\n"
  ".global my_super_code_excp       \t\n"
  ".global my_super_code_excp_after \t\n"
  "my_super_code:                   \t\n"
  "1: addi %r4, %r4, 4              \t\n"
  "   addi %r4, %r4, 4              \t\n"
  "   addi %r4, %r4, 4              \t\n"
  "my_super_code_excp:              \t\n"
  "   trap                          \t\n"
  "my_super_code_excp_after:        \t\n"
  "   addi %r4, %r4, 4              \t\n"
  "   addi %r4, %r4, 4              \t\n"
  "   addi %r4, %r4, 4              \t\n"
  "   b 1b                          \t\n"
  );
static void setup_user_state_arch(L4vcpu::Vcpu *) { }
static void handler_prolog() {}
#elif defined(ARCH_sparc)
asm
(
  ".p2align 12                      \t\n"
  ".global my_super_code            \t\n"
  ".global my_super_code_excp       \t\n"
  ".global my_super_code_excp_after \t\n"
  "my_super_code:                   \t\n"
  "1: add %o0, 4, %o0               \t\n"
  "my_super_code_excp:              \t\n"
  "   ta 2                          \t\n"
  "my_super_code_excp_after:        \t\n"
  "   add %o0, 4, %o0               \t\n"
  "   b 1b                          \t\n"
  "   nop                           \t\n"
  );
static void setup_user_state_arch(L4vcpu::Vcpu *) { }
static void handler_prolog() {}

#elif defined(ARCH_mips)
asm
(
  ".p2align 14                      \t\n" // mapping must be L4_PAGESHIFT aligned
  ".global my_super_code            \t\n"
  ".global my_super_code_excp       \t\n"
  ".global my_super_code_excp_after \t\n"
  "my_super_code:                   \t\n"
  "1: addiu $a0, $a0, 4             \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "my_super_code_excp:              \t\n"
  "   .word(0x42000028) # ASM_HYPCALL, exit to host with Guest exit exception 0x1b \t\n"
  "my_super_code_excp_after:        \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "   addiu $a0, $a0, 4             \t\n"
  "   b 1b                          \t\n"
  "   nop                           \t\n"
  );
static void setup_user_state_arch(L4vcpu::Vcpu *) { }
static void handler_prolog() {}

#else
#error Add your architecture.
#endif


static void handler(void)
{
  static int irq_count = 0;
  printf("enterring handler...\n");
  handler_prolog();

  vcpu->state()->clear(L4_VCPU_F_EXCEPTIONS);

  if (0)
    vcpu->print_state();

  // very simple page-fault handling
  // we're just replying with the only page we have, without checking any
  // values
  if (vcpu->is_page_fault_entry())
    {
      printf("VCPU pagefault\n");
      vcpu_task->map(L4Re::This_task, l4_fpage((l4_addr_t)my_super_code,
                                               L4_PAGESHIFT, L4_FPAGE_RWX),
                                               super_code_map_addr);
      vcpu->saved_state()->add(L4_VCPU_F_USER_MODE);
      vcpu->saved_state()->add(L4_VCPU_F_PAGE_FAULTS);
    }
  else if (vcpu->is_irq_entry())
    {
      // We use the label 2000 for our IRQ
      if (vcpu->i()->label == 2000)
        printf("Our triggered IRQ %d\n", ++irq_count);
      else if (vcpu->i()->label == 0)
        // direct IPC message to vCPU without
        // going through an IPCgate, label is set to 0
        printf("IPC: %lx\n", vcpu->i()->tag.label());
      else
        printf("Unclassifiable message\n");
    }
  else
  {
    printf("VCPU exception %lx, ip=%lx\n", vcpu->r()->err, vcpu->r()->ip);
    vcpu->print_state();
    // we should also check the exception number here
#if !defined(RUN_VCPU_IN_VZ_GUESTVM)
    if (vcpu->r()->ip == (l4_addr_t)my_super_code_excp - (l4_addr_t)my_super_code + super_code_map_addr)
#else
    if (vcpu->r()->ip == (l4_addr_t)my_super_code_excp - (l4_addr_t)my_super_code + super_code_map_addr + vm_run_offset)
#endif
      vcpu->r()->ip += my_super_code_excp_after - my_super_code_excp;
  }

  printf("resume\n");
  L4::Cap<L4::Thread> self;
  l4_msgtag_t tag;
  long result;

  // we want to catch all exceptions, page faults, traps so vcpu resume should
  // only use upcalls and not vcpu exits unless there is an error.
  tag = self->vcpu_resume_commit(self->vcpu_resume_start());

  result = l4_error(tag);
  if (result == 0) {
    printf("vcpu exit\n");
  } else if (result > 0) {
    printf("vcpu resume returned %ld ... ipc waiting\n", result);
  } else {
    printf("vcpu resume failed: %s (%ld) ... exiting\n",
        l4sys_errtostr(result), result);
    exit(1);
  }
  while(1)
    ;
}

static void vcpu_thread(void)
{
  printf("Hello vCPU\n");

  memset(hdl_stack, 0, sizeof(hdl_stack));

  setup_user_state_arch(vcpu);
  vcpu->saved_state()->set(L4_VCPU_F_USER_MODE
                           | L4_VCPU_F_EXCEPTIONS
                           | L4_VCPU_F_PAGE_FAULTS
                           | L4_VCPU_F_IRQ);
#if !defined(RUN_VCPU_IN_VZ_GUESTVM)
  vcpu->r()->ip = super_code_map_addr;
#else
  vcpu->r()->ip = vm_run_offset + super_code_map_addr;
#endif
  vcpu->r()->sp = 0x40000; // actually doesn't matter, we're not using any
                           // stack memory in our code

  L4::Cap<L4::Thread> self;

  printf("IRET\n");

  vcpu->task(vcpu_task);
  self->vcpu_resume_commit(self->vcpu_resume_start());

  printf("IRET: failed!\n");
  while (1)
    ;
}

static int run(void)
{
  l4_utcb_t *u = l4_utcb();
  L4::Cap<L4::Thread> vcpu_cap;

  printf("vCPU example\n");

  l4_debugger_set_object_name(l4re_env()->main_thread, "vcputest");

#if !defined(RUN_VCPU_IN_VZ_GUESTVM)
  // new task
  vcpu_task = chkcap(L4Re::Util::cap_alloc.alloc<L4::Task>(),
                     "Task cap alloc");

  chksys(L4Re::Env::env()->factory()->create_task(vcpu_task,
                                                  l4_fpage_invalid()),
         "create task");
  l4_debugger_set_object_name(vcpu_task.cap(), "vcpu 'user' task");
#else
  // new VM task
  vcpu_task = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Vm>());
  chksys(L4Re::Env::env()->factory()->create_vm(vcpu_task), "create VM");
  l4_debugger_set_object_name(vcpu_task.cap(), "vcpu 'guest vm' task");
#endif

  /* new thread/vCPU */
  vcpu_cap = chkcap(L4Re::Util::cap_alloc.alloc<L4::Thread>(),
                    "vCPU cap alloc");

  l4_touch_rw(thread_stack, sizeof(thread_stack));

  chksys(L4Re::Env::env()->factory()->create_thread(vcpu_cap), "create thread");
  l4_debugger_set_object_name(vcpu_cap.cap(), "vcpu thread");

  // get an IRQ
  irq = chkcap(L4Re::Util::cap_alloc.alloc<L4::Irq>(),
               "Irq cap alloc");
  chksys(L4Re::Env::env()->factory()->create_irq(irq), "irq");
  l4_debugger_set_object_name(irq.cap(), "some irq");

#if !defined(RUN_VCPU_IN_VZ_GUESTVM)
  // get memory for vCPU state
  l4_addr_t kumem;
  if (0)
    kumem = (l4_addr_t)l4re_env()->first_free_utcb;
  else
    {
      if (L4Re::Util::kumem_alloc(&kumem, 0))
        exit(1);
    }
  l4_utcb_t *vcpu_utcb = (l4_utcb_t *)kumem;
  vcpu = L4vcpu::Vcpu::cast(kumem + L4_UTCB_OFFSET);

  printf("VCPU: utcb = %p, vcpu = %p\n", vcpu_utcb, vcpu);
#else
  // get memory for vCPU and VM state
  l4_addr_t _vmstate;
  int r = L4vcpu::Vcpu::ext_alloc(&vcpu, &_vmstate);
  if (r) {
      printf("Failed to allocate virtualization data structures: %d\n", r);
      return 0;
  }
  vmstate = (l4_vm_vz_state *)_vmstate;

  /* Kyma: In the VCPU example utcb is @0x0 and vcpu is @0x200 (L4_UTCB_OFFSET),
   * whereas ext_alloc returns a sz 0x1000 block with vcpu @0x0 and vm @0x400.
   * Move utcb @0x200 so the ipc doesn't corrupt vcpu state. 
   */
  l4_addr_t kumem = (l4_addr_t)vcpu;
  l4_utcb_t *vcpu_utcb = (l4_utcb_t *)(kumem + L4_UTCB_OFFSET);

  printf("VCPU: utcb = %p, vcpu = %p, vmstate = %p\n", vcpu_utcb, vcpu, vmstate);
#endif

  vcpu->entry_sp((l4_umword_t)hdl_stack + sizeof(hdl_stack));
  vcpu->entry_ip((l4_umword_t)handler);

  // Create and start vCPU thread
  L4::Thread::Attr attr;
  attr.pager(L4Re::Env::env()->rm());
  attr.exc_handler(L4Re::Env::env()->main_thread());
  attr.bind(vcpu_utcb, L4Re::This_task);
  chksys(vcpu_cap->control(attr), "control");
#if !defined(RUN_VCPU_IN_VZ_GUESTVM)
  chksys(vcpu_cap->vcpu_control((l4_addr_t)vcpu), "enable VCPU");
#else
  chksys(vcpu_cap->vcpu_control_ext((l4_addr_t)vcpu), "enable VCPU VM");
#endif

  chksys(vcpu_cap->ex_regs((l4_umword_t)vcpu_thread,
                           (l4_umword_t)thread_stack + sizeof(thread_stack),
                           0));

  chksys(L4Re::Env::env()->scheduler()->run_thread(vcpu_cap,
                                                   l4_sched_param(2)));

  // Attach irq to our vCPU thread
  chksys(irq->attach(2000, vcpu_cap));

  // Send some IPCs to the vCPU
  l4_sleep(10);
  for (int i = 0; i < 20; ++i)
    l4_ipc_send(vcpu_cap.cap(), u, l4_msgtag(10 + i, 0, 0, 0), L4_IPC_NEVER);

  // Some IRQ inbetween
  irq->trigger();

  l4_sleep(10);
  for (int i = 21; i < 40; ++i)
    l4_ipc_send(vcpu_cap.cap(), u, l4_msgtag(10 + i, 0, 0, 0), L4_IPC_NEVER);

  // finally, trigger IRQs
  while (1)
    {
      irq->trigger();
      l4_sleep(500);
    }


  l4_sleep_forever();
  return 0;
}

int main()
{
  try { return run(); }
  catch (L4::Runtime_error &e)
    {
      L4::cerr << "FATAL uncaught exception: " << e
               << "\nterminating...\n";
    }
  catch (...)
    {
      L4::cerr << "FATAL uncaught exception of unknown type\n"
               << "terminating...\n";
    }
  return 1;
}
