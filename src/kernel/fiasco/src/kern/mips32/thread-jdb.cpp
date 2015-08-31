/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32 && debug]:

#include "trap_state.h"
#include "util.h"

EXTENSION class Thread
{
public:
  typedef void (*Dbg_extension_entry)(Thread *t, Trap_state *ts);
  static Dbg_extension_entry dbg_extension[64];

  enum Kernel_entry_op
  {
    Op_kdebug_none = 0,
    Op_kdebug_text = 1,
    Op_kdebug_call = 2,
  };

private:
  static Trap_state::Handler nested_trap_handler;
};

//------------------------------------------------------------------------------
IMPLEMENTATION [mips32 && debug]:

#include "kernel_task.h"
#include "mem_layout.h"
#include "thread.h"
#include "cpu.h"
#include "kdb_ke.h"
#include "inst.h"

#include <cstring>

Thread::Dbg_extension_entry Thread::dbg_extension[64];
Trap_state::Handler Thread::nested_trap_handler;

extern "C" void sys_kdb_ke()
{
  cpu_lock.lock();
  char str[32] = "USER ENTRY";
  Thread *t = current_thread();
  Entry_frame *regs = t->regs();
  // Use the Entry_frame to get the enclosing Trap_state frame
  Trap_state *ts = reinterpret_cast<Trap_state*>((Unsigned8 *)regs - sizeof(Trap_state_regs));
  Mword kdb_ke_op = ts->r[Syscall_frame::REG_S6];

  if (static_cast<Thread::Kernel_entry_op>(kdb_ke_op) == Thread::Op_kdebug_call)
    {
      Mword kdb_ke_func = ts->r[Syscall_frame::REG_T0];

      kdb_ke_func %= (sizeof(Thread::dbg_extension)/sizeof(Thread::dbg_extension[0]));

      if (Thread::dbg_extension[kdb_ke_func])
	{
	  Thread::dbg_extension[kdb_ke_func](t, ts);
	  return;
	}
    }

  if (static_cast<Thread::Kernel_entry_op>(kdb_ke_op) == Thread::Op_kdebug_text)
    {
      int from_user = MIPS_USERMODE(ts->status);
      char const *kdb_ke_str =
        reinterpret_cast<char const *>(ts->r[Syscall_frame::REG_T0]);

      if (t->space()->virt_to_phys(reinterpret_cast<Address>(kdb_ke_str))
          != Invalid_address)
        {
          for (unsigned i = 0; i < sizeof(str); ++i)
            {
              str[i] = t->space()->peek(kdb_ke_str + i, from_user);
              if (str[i] == 0)
                break;
            }
          str[sizeof(str)-1] = 0;
        }
    }

  kdb_ke(str);
}

PUBLIC static
int
Thread::call_nested_trap_handler(Trap_state *ts)
{
  Cpu_phys_id phys_cpu = Proc::cpu_id();
  Cpu_number log_cpu = Cpu::cpus.find_cpu(Cpu::By_phys_id(phys_cpu));
  if (log_cpu == Cpu_number::nil())
    {
      printf("Trap on unknown CPU phys_id=%x\n",
             cxx::int_value<Cpu_phys_id>(phys_cpu));
      log_cpu = Cpu_number::boot_cpu();
    }

  unsigned long &ntr = nested_trap_recover.cpu(log_cpu);

  void *stack = 0;

  if (!ntr)
    stack = dbg_stack.cpu(log_cpu).stack_top;

  Mem_space *m = Mem_space::current_mem_space(log_cpu);

  if (Kernel_task::kernel_task() != m)
    Kernel_task::kernel_task()->make_current();

  Mword dummy1, tmp, ret;
  {
    register Mword _ts asm("a0") = (Mword)ts;
    register Cpu_number _lcpu asm("a1") = log_cpu;

    asm volatile(
  ".set push                        \n"
  ".set noreorder                   \n"
	" move  %[origstack], $sp         \n"
	" lw    %[tmp], 0(%[ntr])         \n"
  " bnez  %[tmp], 1f                \n"
  " nop                             \n"
	" move  $sp, %[stack]             \n"
	"1:                               \n"
	" addiu %[tmp], %[tmp], 1         \n"
	" sw    %[tmp], 0(%[ntr])         \n"
	" addu  $sp, $sp, -32             \n" //set up call frame
	" sw    %[origstack], 28($sp)     \n"
	" sw    %[ntr], 24($sp)           \n"
  " jalr  %[handler]                \n"
  " nop                             \n"
  " lw    %[ntr], 24($sp)           \n"
	" lw    $sp, 28($sp)              \n"
	" lw    %[tmp], 0(%[ntr])         \n"
	" addiu %[tmp], %[tmp], -1        \n"
	" sw    %[tmp], 0(%[ntr])         \n"
  ".set pop                         \n"
	: [origstack] "=&r" (dummy1), [tmp] "=&r" (tmp),
	  "=r" (_ts), "=r" (_lcpu)
	: [ntr] "r" (&ntr), [stack] "r" (stack),
	  [handler] "r" (*nested_trap_handler),
	  "2" (_ts), "3" (_lcpu)
	: "memory");

    ret = _ts;

    if (!ts->is_debug_break_exception())
      ts->epc += 4;
  }

  // the jdb-cpu might have changed things we shouldn't miss!
  Mmu<Mem_layout::Cache_flush_area, true>::flush_cache();

  if (m != Kernel_task::kernel_task())
    m->make_current();

  return ret;
}

PUBLIC static
void
Thread::set_debug_errorcode(Trap_state *ts)
{
  union mips_instruction insn;
  int from_user = MIPS_USERMODE(ts->status);
  Mword epc = ts->epc;

  // NOTE: During a nested Bp exception (where Status.EXL is already set) the
  // architecture does not update epc with the nested exception pc. Using
  // badinstr allows proper identification of the breakpoint code (without it
  // the wrong epc is read). Nevertheless, the nested breakpoint is not handled
  // correctly: the correct epc can only be known by adding support for the
  // optional Nested Fault feature.
  // TODO detect nested Bp and fail gracefully
  if (cpu_has_badinstr)
    insn.word = read_c0_badinstr();
  else
    {
      // epc may not always be correct, as explained above
      insn.word = current_thread()->space()->peek((Mword*)epc, from_user);
    }

  if (!kdb_ke_is_break_opcode(insn.word))
    return;

  switch (insn.b_format.code)
  {
    case KDB_KE_USER_TRAP:
      ts->set_errorcode(ts->error() | Trap_state::Jdb_debug_trap);
      break;
    case KDB_KE_USER_SEQ:
      ts->set_errorcode(ts->error() | Trap_state::Jdb_debug_sequence);
      break;
    case KDB_KE_IPI:
      ts->set_errorcode(ts->error() | Trap_state::Jdb_debug_ipi);
      break;
    case KDB_KE_BREAKPOINT:
      ts->set_errorcode(ts->error() | Trap_state::Jdb_debug_break);
      break;
    default:
      break;
  }
}

IMPLEMENTATION [mips32 && !debug]:

extern "C" void sys_kdb_ke()
{}

PUBLIC static inline
int
Thread::call_nested_trap_handler(Trap_state *)
{ return -1; }
