/*
 * Copyright (C) 2015 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "initcalls.h"
#include "l4_types.h"
#include "string_buffer.h"
#include "kdb_ke.h"

class Thread;
class Task;
class Space;
class Jdb_entry_frame;

class Breakpoint
{
public:
  enum Mode { INSTRUCTION=0, WRITE=1, READ=2, ACCESS=3 };
  enum Log  { BREAK=0, LOG=1 };
  enum { MAX_BP = 4 };

private:
  Address      addr;
  Address      phys;
  Mword        patch;
  Unsigned8    len;
  Mode         mode;
  Log          log;
  Task         *task_ptr;
  static char const * const mode_names[MAX_BP];
};

class Jdb_bp
{
public:
  static int            global_breakpoints();
  static void           init_arch();

private:
  static void           at_jdb_enter();
  static void           at_jdb_leave();

  static int            test_log_only(Cpu_number);
  static int            test_break(Cpu_number, String_buffer *buf);

  static Breakpoint     bps[Breakpoint::MAX_BP];
};


IMPLEMENTATION[mips32]:

#include <cstdio>

#include "jdb.h"
#include "jdb_entry_frame.h"
#include "jdb_input_task.h"
#include "jdb_module.h"
#include "jdb_handler_queue.h"
#include "jdb_tbuf.h"
#include "static_init.h"
#include "task.h"
#include "thread.h"
#include "asm.h"
#include "mipsregs.h"

class Jdb_set_bp : public Jdb_module, public Jdb_input_task_addr
{
public:
  Jdb_set_bp() FIASCO_INIT;
private:
  static char     breakpoint_cmd;
  static Mword    breakpoint_number;
  static Mword    breakpoint_length;
  static int      state;
};

Breakpoint Jdb_bp::bps[Breakpoint::MAX_BP];

char const * const Breakpoint::mode_names[Breakpoint::MAX_BP] =
{
  "instruction", "write access", "read access", "r/w access"
};

char     Jdb_set_bp::breakpoint_cmd;
Mword    Jdb_set_bp::breakpoint_number;
Mword    Jdb_set_bp::breakpoint_length;
int      Jdb_set_bp::state;

PUBLIC
Breakpoint::Breakpoint() : addr(Invalid_address), phys(Invalid_address)
{
}

PUBLIC inline NOEXPORT
void
Breakpoint::clear()
{
  addr = phys = Invalid_address;
  patch = 0;
}

PUBLIC inline NOEXPORT
Task*
Breakpoint::get_task()
{
  return task_ptr;
}

PUBLIC inline NOEXPORT
Mword
Breakpoint::get_addr()
{
  return addr;
}

PUBLIC inline NOEXPORT
Mword
Breakpoint::get_phys()
{
  return phys;
}

PUBLIC inline NOEXPORT
Mword
Breakpoint::get_len()
{
  return len;
}

PUBLIC inline NOEXPORT
Mword
Breakpoint::get_mode()
{
  return mode;
}

PUBLIC inline NOEXPORT
Mword
Breakpoint::get_patch()
{
  return patch;
}

PUBLIC inline NOEXPORT
int
Breakpoint::unused()
{
  return addr == Invalid_address;
}

PUBLIC inline NOEXPORT
int
Breakpoint::break_at_instruction()
{
  return mode == INSTRUCTION;
}

PUBLIC inline NOEXPORT
int
Breakpoint::match_addr(Address virt, Mode m)
{
  return !unused() && addr == virt && mode == m;
}

PUBLIC inline NOEXPORT
void
Breakpoint::set_logmode(char m)
{
  log = (m == '*') ? LOG : BREAK;
}

PUBLIC inline NOEXPORT
int
Breakpoint::is_break()
{
  return !unused() && log == BREAK;
}

PUBLIC
void
Breakpoint::show()
{
  if (!unused())
    {
      printf("%5s on %12s at " L4_PTR_FMT,
             log ? "LOG" : "BREAK", mode_names[mode & 3], addr);
      if (mode != INSTRUCTION)
        printf(" len %d", len);
      else
        putstr("      ");

      Task* task = get_task();
      if (task)
        printf(" (task %lx)\n", task->dbg_info()->dbg_id());
      else
        printf(" (phys-mem)\n");
    }
  else
    puts("disabled");
}

PUBLIC
int
Breakpoint::test_break(int num, String_buffer *buf, Thread *t)
{
  (void)t;
  buf->printf("BREAK #%i on %s at " L4_PTR_FMT, num + 1, mode_names[mode], addr);
#if 0 // TODO no watchpoint support
  if (mode == WRITE || mode == READ || mode == ACCESS)
    {
      Space *task = t->space();

      // If it's a write, read or access breakpoint, we look at the
      // appropriate place and print the bytes we find there. We do
      // not need to look if the page is present because the x86 CPU
      // enters the debug exception immediately _after_ the memory
      // access was performed.
      Mword val = 0;
      if (len > sizeof(Mword))
        return 0;

      if (Jdb::peek_task(addr, task, &val, len) != 0)
        return 0;

      buf->printf(" [%08lx]", val);
    }
#endif
  return 1;
}

// Create log entry if breakpoint matches
PUBLIC
void
Breakpoint::test_log(Thread *t)
{
  (void)t;
  Jdb_entry_frame *e = Jdb::get_entry_frame(Jdb::current_cpu);

  if (log)
    {
      // log breakpoint
      Mword value = 0;
#if 0 // TODO no watchpoint support
      if (mode == WRITE || mode == READ || mode == ACCESS)
        {
          Space *task = t->space();

          // If it's a write, read or access breakpoint, we look at the
          // appropriate place and print the bytes we find there. We do
          // not need to look if the page is present because the x86 CPU
          // enters the debug exception immediately _after_ the memory
          // access was performed.
          if (len > sizeof(Mword))
            return;

          if (Jdb::peek_task(addr, task, &value, len) != 0)
            return;
        }
#endif
      // is called with disabled interrupts
      Tb_entry_bp *tb = static_cast<Tb_entry_bp*>(Jdb_tbuf::new_entry());
      tb->set(t, e->ip(), mode, len, value, addr);
      Jdb_tbuf::commit_entry();
    }
}

PUBLIC inline
void
Breakpoint::set(Address _addr, Address _phys, Mword _patch, Mword _len, Mode _mode, Log _log, Task *_task)
{
  addr = _addr;
  phys = _phys;
  patch = _patch;
  mode = _mode;
  log  = _log;
  len  = _len;
  task_ptr  = _task;
}


STATIC_INITIALIZE_P(Jdb_bp, JDB_MODULE_INIT_PRIO);

PUBLIC static FIASCO_INIT
void
Jdb_bp::init()
{
  static Jdb_handler enter(at_jdb_enter);
  static Jdb_handler leave(at_jdb_leave);

  Jdb::jdb_enter.add(&enter);
  Jdb::jdb_leave.add(&leave);

  Jdb::bp_test_log_only = test_log_only;
  Jdb::bp_test_break    = test_break;
}

IMPLEMENT
void
Jdb_bp::at_jdb_enter()
{
  // disable breakpoints (and restore patched addresses) while we are in jdb
  int i;

  for (i = 0; i < Breakpoint::MAX_BP; i++)
    {
      if (bps[i].unused())
        continue;

      disable_breakpoint(i);
    }
}

IMPLEMENT
void
Jdb_bp::at_jdb_leave()
{
  // enable breakpoints when leaving jdb
  int i;
  Mword bp_opcode = kdb_ke_breakpoint_opcode();

  for (i = 0; i < Breakpoint::MAX_BP; i++)
    {
      if (bps[i].unused())
        continue;

      Task* task = bps[i].get_task();
      Mword addr = bps[i].get_addr();
      Mword len = bps[i].get_len();

      if (Jdb::poke_task(addr, task, &bp_opcode, len) != 0)
        {
          printf("\nFailed to enable breakpoint %i at " L4_PTR_FMT "\n", i + 1, addr);
        }
    }
}

PUBLIC static
int
Jdb_bp::set_breakpoint(int num, Address addr, Mword len,
                       Breakpoint::Mode mode, Breakpoint::Log log,
                       Task *task)
{
  Mword patch;

  // align
  addr &= ~(sizeof(Address)-1);

  Address phys = Jdb::translate_task_addr_to_phys(addr, task);
  if (phys == Invalid_address)
    return 0;

  if (Jdb::peek_task(addr, task, &patch, len) == 0)
    {
      // defer patching breakpoint addresses until leaving jdb
      bps[num].set(addr, phys, patch, len, mode, log, task);
      puts("");
      bps[num].show();
      return 1;
    }

  return 0;
}

PUBLIC static
void
Jdb_bp::disable_breakpoint(int num)
{
  Task* task = bps[num].get_task();
  Mword addr = bps[num].get_addr();
  Mword patch = bps[num].get_patch();
  Mword len = bps[num].get_len();
  Mword peek;
  Mword bp_opcode = kdb_ke_breakpoint_opcode();

  if (Jdb::peek_task(addr, task, &peek, len) == 0)
    if (peek == bp_opcode)
      Jdb::poke_task(addr, task, &patch, len);
}

PUBLIC static
void
Jdb_bp::clr_breakpoint(int num)
{
  disable_breakpoint(num);
  bps[num].clear();
}

PUBLIC static inline NOEXPORT
void
Jdb_bp::logmode_breakpoint(int num, char mode)
{
  bps[num].set_logmode(mode);
}

PUBLIC static
int
Jdb_bp::first_unused()
{
  int i;

  for (i = 0; i < Breakpoint::MAX_BP && !bps[i].unused(); i++)
    ;

  return i;
}

PRIVATE static inline NOEXPORT
Address
Jdb_bp::get_breakpoint_addr(Jdb_entry_frame *ef)
{
  // check for break exception in branch delay slot
  if (ef->cause & CAUSEF_BD)
    return ef->epc + 4;
  else
    return ef->epc;
}

/** @return 1 if breakpoint occured */
IMPLEMENT
int
Jdb_bp::test_break(Cpu_number cpu, String_buffer *buf)
{
  // TODO implement SMP breakpoint support
  Thread *t = Jdb::get_thread(cpu);
  Jdb_entry_frame *ef = Jdb::get_entry_frame(cpu);
  Address epc = get_breakpoint_addr(ef);

  if (!ef->debug_breakpoint())
    return 0;

  for (int i = 0; i < Breakpoint::MAX_BP; i++)
    {
      if (bps[i].unused())
        continue;

      Address phys = Jdb::translate_task_addr_to_phys(epc, t->space());
      if (bps[i].get_phys() != phys)
        continue;

      bps[i].test_break(i, buf, t);

      // TODO no support to singlestep instruction under the breakpoint
      Jdb_bp::clr_breakpoint(i);

      return 1;
    }

  buf->printf("BREAK #? at " L4_PTR_FMT, epc);
  return 1;
}

/** @return 1 if only breakpoints were logged and jdb should not be entered */
IMPLEMENT
int
Jdb_bp::test_log_only(Cpu_number cpu)
{
  Thread *t = Jdb::get_thread(cpu);
  Jdb_entry_frame *ef = Jdb::get_entry_frame(cpu);
  Address epc = get_breakpoint_addr(ef);

  if (!ef->debug_breakpoint())
    return 0;

  for (int i = 0; i < Breakpoint::MAX_BP; i++)
    {
      if (bps[i].unused())
        continue;

      Address phys = Jdb::translate_task_addr_to_phys(epc, t->space());
      if (bps[i].get_phys() != phys)
        continue;

      if (bps[i].is_break())
        continue;

      bps[i].test_log(t);

      // TODO no support to singlestep instruction under the breakpoint
      Jdb_bp::clr_breakpoint(i);

      return 1;
    }

  return 0;
}

PUBLIC static
Mword
Jdb_bp::test_match(Address addr, Breakpoint::Mode mode)
{
  for (int i = 0; i < Breakpoint::MAX_BP; i++)
    if (bps[i].match_addr(addr, mode))
      return i + 1;

  return 0;
}

PUBLIC static
int
Jdb_bp::instruction_bp_at_addr(Address addr)
{ return test_match(addr, Breakpoint::INSTRUCTION); }


PUBLIC static
void
Jdb_bp::list()
{
  putchar('\n');

  for(int i = 0; i < Breakpoint::MAX_BP; i++)
    {
      printf("  #%d: ", i + 1);
      bps[i].show();
    }

  putchar('\n');
}


//---------------------------------------------------------------------------//

IMPLEMENT
Jdb_set_bp::Jdb_set_bp()
  : Jdb_module("DEBUGGING")
{}

PUBLIC
Jdb_module::Action_code
Jdb_set_bp::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  enum State
  {
    Give_address = 1,
    Give_num = 2,
    Do_bp = 3,
  };

  Jdb_module::Action_code code;
  Breakpoint::Mode mode;

  if (cmd != 0)
    return NOTHING;

  if (args == &breakpoint_cmd)
    {
      switch (breakpoint_cmd)
        {
        case 'i':
        case 'a':
        case 'r':
        case 'w':
          if ((breakpoint_number = Jdb_bp::first_unused()) < Breakpoint::MAX_BP)
            {
              printf("[t<taskno>|p]");
              fmt   = " addr=%C";
              args  = &Jdb_input_task_addr::first_char;
              state = Give_address; // breakpoints are global for all tasks
              return EXTRA_INPUT;
            }
          puts(" No breakpoints available");
          return NOTHING;
        case 'l':
          // show all breakpoints
          Jdb_bp::list();
          return NOTHING;
        case '-':
          // delete breakpoint
        case '+':
          // set logmode of breakpoint to <STOP>
        case '*':
          // set logmode of breakpoint to <LOG>
          fmt   = " bpn=%1x";
          args  = &breakpoint_number;
          state = Give_num;
          return EXTRA_INPUT;
        default:
          return ERROR;
        }
    }
  else switch (state)
    {
    case Give_address:
      code = Jdb_input_task_addr::action(args, fmt, next_char);
      if (code == ERROR)
        return ERROR;
      if (code != NOTHING)
        // more input for Jdb_input_task_addr
        return code;

      // ok, continue
      if (breakpoint_cmd != 'i')
        {
          fmt   = " len (1, 2, 4, ...)=%1x";
          args  = &breakpoint_length;
          state = Do_bp;
          return EXTRA_INPUT;
        }
      // insn breakpoint
      breakpoint_length = 4;
      state = Do_bp;
      break;
    case Give_num:
      if (breakpoint_number < 1 || breakpoint_number > Breakpoint::MAX_BP)
        return ERROR;
      // input is 1..4 but numbers are 0..3
      breakpoint_number -= 1;
      // we know the breakpoint number
      switch (breakpoint_cmd)
        {
        case '-':
          Jdb_bp::clr_breakpoint(breakpoint_number);
          putchar('\n');
          return NOTHING;
        case '+':
        case '*':
          Jdb_bp::logmode_breakpoint(breakpoint_number, breakpoint_cmd);
          putchar('\n');
          return NOTHING;
        default:
          return ERROR;
        }
      break;
    }

  if (state == Do_bp)
    {
      // validate length
      if (breakpoint_length & (breakpoint_length - 1))
        return NOTHING;
      if (breakpoint_length > sizeof(Mword))
        return NOTHING;
      switch (breakpoint_cmd)
        {
        default : return ERROR;
        case 'i': mode = Breakpoint::INSTRUCTION; break;
        case 'w': mode = Breakpoint::WRITE;       break;
        case 'r': mode = Breakpoint::READ;        break;
        case 'a': mode = Breakpoint::ACCESS;      break;
        }
      // abort if no address was given
      if (Jdb_input_task_addr::addr() == (Address)-1)
        return ERROR;
      if (mode == Breakpoint::INSTRUCTION)
        {
          if (Jdb_bp::set_breakpoint(breakpoint_number, Jdb_input_task_addr::addr(),
                                 breakpoint_length, mode, Breakpoint::BREAK,
                                 Jdb_input_task_addr::task()) == 0)
            printf("\nFailed to set breakpoint at " L4_PTR_FMT "\n", Jdb_input_task_addr::addr());
        }
      else
        printf("\n'%c' watchpoints not yet handled\n", breakpoint_cmd);

      putchar('\n');
    }
  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_set_bp::cmds() const
{
  static Cmd cs[] =
    {
        { 0, "b", "bp", "%c",
          "bi[t<taskno>|p]<addr>\tset breakpoint on insn of given/current task at\n"
          "\t<addr>, or physical <addr>\n"
          "b{a|r|w}[t<taskno>|p]<addr>\tset breakpoint on access/read/write\n"
          "\tof given/current task at <addr>, or physical <addr>\n"
          "b{-|+|*}<num>\tdisable/enable/log breakpoint\n"
          "bl\tlist breakpoints\n",
          &breakpoint_cmd },
    };

  return cs;
}

PUBLIC
int
Jdb_set_bp::num_cmds() const
{
  return 1;
}

static Jdb_set_bp jdb_set_bp INIT_PRIORITY(JDB_MODULE_INIT_PRIO);
