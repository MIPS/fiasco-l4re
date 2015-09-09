/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:
#include <cstddef>

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "config.h"
#include "globals.h"
#include "kip_init.h"
#include "kdb_ke.h"
#include "kernel_thread.h"
#include "kernel_task.h"
#include "kernel_console.h"
#include "reset.h"
#include "space.h"
#include "terminate.h"

#include "processor.h"
#include "thread_state.h"
#include "mipsregs.h"

static int exit_question_active = 0;

extern "C" void __attribute__ ((noreturn))
_exit(int)
{
  if (exit_question_active)
    platform_reset();

  printf("Exiting\n");
  while (1)
    {
      Proc::halt();
      Proc::pause();
    }
}

static void exit_question()
{
  exit_question_active = 1;

  while (1)
    {
      puts("\nReturn reboots, \"k\" enters L4 kernel debugger...");

      char c = Kconsole::console()->getchar();

      if (c == 'k' || c == 'K')
	{
	  kdb_ke("_exit");
	}
      else
	{
	  // it may be better to not call all the destruction stuff
	  // because of unresolved static destructor dependency
	  // problems.
	  // SO just do the reset at this point.
	  puts("\033[1mRebooting...\033[0m");
	  platform_reset();
	  break;
	}
    }
}

void kernel_main()
{
  // caution: no stack variables in this function because we're going
  // to change the stack pointer!

  set_exit_question(&exit_question);

  // disallow all interrupts before we selectively enable them
  Proc::cli();

  // create kernel thread
  static Kernel_thread *kernel = new (Ram_quota::root) Kernel_thread;
  Task *const ktask = Kernel_task::kernel_task();
  check(kernel->bind(ktask, User<Utcb>::Ptr(0)));

  void *sp = kernel->init_stack();
    {
      register Mword a0 __asm__("a0") = reinterpret_cast<Mword>(kernel);

      // switch to stack of kernel thread and bootstrap the kernel
      asm volatile
        ("	move $29,%0             \n"	// switch stack
         "	jal call_bootstrap      \n"
         :
         : "r" (sp), "r" (a0));
    }
}

//------------------------------------------------------------------------
IMPLEMENTATION[mips32 && mp]:

#include <cstdio>
#include "config.h"
#include "cpu.h"
#include "globals.h"
#include "app_cpu_thread.h"
#include "ipi.h"
#include "per_cpu_data_alloc.h"
#include "perf_cnt.h"
#include "pic.h"
#include "spin_lock.h"
#include "timer.h"
#include "utcb_init.h"

static int boot_ap_cpu();
void boot_ap_cpu_lock() __asm__("BOOT_AP_CPU");

void boot_ap_cpu_lock()
{
  /* do not use the stack before acquiring _tramp_mp_spinlock, all secondary
   * processors will use the same stack at the same time until this point. It's
   * ok because they all run the same bootstrap code with the same stack usage
   * pattern.
   */

  /* acquire spin lock for using _tramp_mp_init_stack */
  extern Spin_lock<Mword> _tramp_mp_spinlock;
  _tramp_mp_spinlock.test_and_set();

  boot_ap_cpu();
}

static int boot_ap_cpu()
{
  static Cpu_number last_cpu; // keep track of the last cpu ever appeared

  /* Switch to new exception base  */
  write_c0_status(read_c0_status() & ~ST0_BEV);

  Cpu_number _cpu = Cpu::cpus.find_cpu(Cpu::By_phys_id(Proc::cpu_id()));
  bool cpu_is_new = false;
  if (_cpu == Cpu_number::nil())
    {
      _cpu = ++last_cpu; // 0 is the boot cpu, so pre increment
      cpu_is_new = true;
    }

  assert (_cpu != Cpu_number::boot_cpu());

  if (cpu_is_new && !Per_cpu_data_alloc::alloc(_cpu))
    {
      extern Spin_lock<Mword> _tramp_mp_spinlock;
      printf("CPU allocation failed for CPU%u, disabling CPU.\n",
             cxx::int_value<Cpu_number>(_cpu));
      _tramp_mp_spinlock.clear();

      while (1)
	Proc::halt();
    }

  if (cpu_is_new)
    Per_cpu_data::run_ctors(_cpu);

  Cpu &cpu = Cpu::cpus.cpu(_cpu);
  cpu.init();

  Pic::init_ap(_cpu);
  Ipi::init(_cpu);
  Thread_ipi::init(_cpu);
  Timer::init(_cpu);
  //Perf_cnt::init_ap();

  // create kernel thread
  Kernel_thread *kernel = App_cpu_thread::may_be_create(_cpu, cpu_is_new);

  void *sp = kernel->init_stack();
    {
      register Mword a0 __asm__("a0") = reinterpret_cast<Mword>(kernel);
      register Mword a1 __asm__("a1") = !cpu_is_new;

      // switch to stack of kernel thread and continue thread init
      asm volatile
        ("	move $29,%0             \n"	// switch stack
         "	jal call_ap_bootstrap   \n"
         :
         : "r" (sp), "r" (a0), "r" (a1));
    }

  panic("should never be reached");
  return 0;
}
