INTERFACE [arm]:

#include "jdb.h"
#include "jdb_input_task.h"
#include "jdb_module.h"

class Jdb_bp : public Jdb_module, public Jdb_input_task_addr
{
};

IMPLEMENTATION [arm]:

#include <cstdio>
#include "string_buffer.h"

EXTENSION class Jdb_bp
{
public:
  Jdb_bp() FIASCO_INIT;
private:
  static void   at_jdb_enter();
  static void   at_jdb_leave();

  static int    test_log_only(Cpu_number);
  static int    test_break(Cpu_number, String_buffer *buf);


  static char  breakpoint_cmd;
  static bool inited;
  static char state;
  static char breakpoint_length;
  static int  breakpoint_number;
  static char breakpoint_type;

  enum
  {
    Vers_v7_1 = 5,
  };
};

struct Test_debug_data
{
  String_buffer *buf;
  bool          disable;
  Address       addr;
  char          type;
};

struct Set_debug_data
{
  int   idx;
  Mword cr_val;
  Mword cr_mask;
  Mword addr;
  char  type;
};

char Jdb_bp::breakpoint_cmd;
bool Jdb_bp::inited;
char Jdb_bp::state;
char Jdb_bp::breakpoint_length;
int  Jdb_bp::breakpoint_number;
char Jdb_bp::breakpoint_type;

PRIVATE static
void
Jdb_bp::init_cpu(Cpu_number)
{
  asm volatile("mcr p14, 0, %0, c0, c7, 0" : : "r" (0)); // vector catch
  asm volatile("mcr p14, 0, %0, c1, c0, 4" : : "r" (0)); // OS lock unlock

  for (unsigned i = 0; i < num_watchpoints(); ++i)
    {
      wcr(i, 0);
      wvr(i, 0);
    }

  for (unsigned i = 0; i < num_breakpoints(); ++i)
    {
      bcr(i, 0);
      bvr(i, 0);
    }

  Mword v;
  asm volatile("mrc p14, 0, %0, c0, c1, 0" : "=r" (v));
  v |= 1 << 15; // MDBen
  asm volatile("mcr p14, 0, %0, c0, c2, 2" : : "r" (v));
}


IMPLEMENT
Jdb_bp::Jdb_bp()
  : Jdb_module("DEBUGGING")
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
  // disable breakpoints while we are in kernel debugger
}

IMPLEMENT
void
Jdb_bp::at_jdb_leave()
{
  // enable again
}

PRIVATE static
unsigned
Jdb_bp::num_watchpoints()
{
  Mword v;
  asm volatile("mrc p14, 0, %0, c0, c0, 0" : "=r" (v));
  return ((v >> 28) & 0xf) + 1;
}

PRIVATE static
unsigned
Jdb_bp::num_breakpoints()
{
  Mword v;
  asm volatile("mrc p14, 0, %0, c0, c0, 0" : "=r" (v));
  return ((v >> 24) & 0xf) + 1;
}

PRIVATE static
void
Jdb_bp::wvr(int num, Mword v)
{
  switch (num)
    {
    case  0: asm volatile("mcr p14, 0, %0, c0,  c0, 6" : : "r" (v)); break;
    case  1: asm volatile("mcr p14, 0, %0, c0,  c1, 6" : : "r" (v)); break;
    case  2: asm volatile("mcr p14, 0, %0, c0,  c2, 6" : : "r" (v)); break;
    case  3: asm volatile("mcr p14, 0, %0, c0,  c3, 6" : : "r" (v)); break;
    case  4: asm volatile("mcr p14, 0, %0, c0,  c4, 6" : : "r" (v)); break;
    case  5: asm volatile("mcr p14, 0, %0, c0,  c5, 6" : : "r" (v)); break;
    case  6: asm volatile("mcr p14, 0, %0, c0,  c6, 6" : : "r" (v)); break;
    case  7: asm volatile("mcr p14, 0, %0, c0,  c7, 6" : : "r" (v)); break;
    case  8: asm volatile("mcr p14, 0, %0, c0,  c8, 6" : : "r" (v)); break;
    case  9: asm volatile("mcr p14, 0, %0, c0,  c9, 6" : : "r" (v)); break;
    case 10: asm volatile("mcr p14, 0, %0, c0, c10, 6" : : "r" (v)); break;
    case 11: asm volatile("mcr p14, 0, %0, c0, c11, 6" : : "r" (v)); break;
    case 12: asm volatile("mcr p14, 0, %0, c0, c12, 6" : : "r" (v)); break;
    case 13: asm volatile("mcr p14, 0, %0, c0, c13, 6" : : "r" (v)); break;
    case 14: asm volatile("mcr p14, 0, %0, c0, c14, 6" : : "r" (v)); break;
    case 15: asm volatile("mcr p14, 0, %0, c0, c15, 6" : : "r" (v)); break;
    default: panic("Invalid watchpoint index");
    };
}

PRIVATE static
Mword
Jdb_bp::wvr(int num)
{
  Mword v;
  switch (num)
    {
    case  0: asm volatile("mrc p14, 0, %0, c0,  c0, 6" : "=r" (v)); break;
    case  1: asm volatile("mrc p14, 0, %0, c0,  c1, 6" : "=r" (v)); break;
    case  2: asm volatile("mrc p14, 0, %0, c0,  c2, 6" : "=r" (v)); break;
    case  3: asm volatile("mrc p14, 0, %0, c0,  c3, 6" : "=r" (v)); break;
    case  4: asm volatile("mrc p14, 0, %0, c0,  c4, 6" : "=r" (v)); break;
    case  5: asm volatile("mrc p14, 0, %0, c0,  c5, 6" : "=r" (v)); break;
    case  6: asm volatile("mrc p14, 0, %0, c0,  c6, 6" : "=r" (v)); break;
    case  7: asm volatile("mrc p14, 0, %0, c0,  c7, 6" : "=r" (v)); break;
    case  8: asm volatile("mrc p14, 0, %0, c0,  c8, 6" : "=r" (v)); break;
    case  9: asm volatile("mrc p14, 0, %0, c0,  c9, 6" : "=r" (v)); break;
    case 10: asm volatile("mrc p14, 0, %0, c0, c10, 6" : "=r" (v)); break;
    case 11: asm volatile("mrc p14, 0, %0, c0, c11, 6" : "=r" (v)); break;
    case 12: asm volatile("mrc p14, 0, %0, c0, c12, 6" : "=r" (v)); break;
    case 13: asm volatile("mrc p14, 0, %0, c0, c13, 6" : "=r" (v)); break;
    case 14: asm volatile("mrc p14, 0, %0, c0, c14, 6" : "=r" (v)); break;
    case 15: asm volatile("mrc p14, 0, %0, c0, c15, 6" : "=r" (v)); break;
    default: panic("Invalid watchpoint index");
    };
  return v;
}

PRIVATE static
void
Jdb_bp::wcr(int num, Mword v)
{
  switch (num)
    {
    case  0: asm volatile("mcr p14, 0, %0, c0,  c0, 7" : : "r" (v)); break;
    case  1: asm volatile("mcr p14, 0, %0, c0,  c1, 7" : : "r" (v)); break;
    case  2: asm volatile("mcr p14, 0, %0, c0,  c2, 7" : : "r" (v)); break;
    case  3: asm volatile("mcr p14, 0, %0, c0,  c3, 7" : : "r" (v)); break;
    case  4: asm volatile("mcr p14, 0, %0, c0,  c4, 7" : : "r" (v)); break;
    case  5: asm volatile("mcr p14, 0, %0, c0,  c5, 7" : : "r" (v)); break;
    case  6: asm volatile("mcr p14, 0, %0, c0,  c6, 7" : : "r" (v)); break;
    case  7: asm volatile("mcr p14, 0, %0, c0,  c7, 7" : : "r" (v)); break;
    case  8: asm volatile("mcr p14, 0, %0, c0,  c8, 7" : : "r" (v)); break;
    case  9: asm volatile("mcr p14, 0, %0, c0,  c9, 7" : : "r" (v)); break;
    case 10: asm volatile("mcr p14, 0, %0, c0, c10, 7" : : "r" (v)); break;
    case 11: asm volatile("mcr p14, 0, %0, c0, c11, 7" : : "r" (v)); break;
    case 12: asm volatile("mcr p14, 0, %0, c0, c12, 7" : : "r" (v)); break;
    case 13: asm volatile("mcr p14, 0, %0, c0, c13, 7" : : "r" (v)); break;
    case 14: asm volatile("mcr p14, 0, %0, c0, c14, 7" : : "r" (v)); break;
    case 15: asm volatile("mcr p14, 0, %0, c0, c15, 7" : : "r" (v)); break;
    default: panic("Invalid watchpoint index");
    };
}

PRIVATE static
Mword
Jdb_bp::wcr(int num)
{
  Mword v;
  switch (num)
    {
    case  0: asm volatile("mrc p14, 0, %0, c0,  c0, 7" : "=r" (v)); break;
    case  1: asm volatile("mrc p14, 0, %0, c0,  c1, 7" : "=r" (v)); break;
    case  2: asm volatile("mrc p14, 0, %0, c0,  c2, 7" : "=r" (v)); break;
    case  3: asm volatile("mrc p14, 0, %0, c0,  c3, 7" : "=r" (v)); break;
    case  4: asm volatile("mrc p14, 0, %0, c0,  c4, 7" : "=r" (v)); break;
    case  5: asm volatile("mrc p14, 0, %0, c0,  c5, 7" : "=r" (v)); break;
    case  6: asm volatile("mrc p14, 0, %0, c0,  c6, 7" : "=r" (v)); break;
    case  7: asm volatile("mrc p14, 0, %0, c0,  c7, 7" : "=r" (v)); break;
    case  8: asm volatile("mrc p14, 0, %0, c0,  c8, 7" : "=r" (v)); break;
    case  9: asm volatile("mrc p14, 0, %0, c0,  c9, 7" : "=r" (v)); break;
    case 10: asm volatile("mrc p14, 0, %0, c0, c10, 7" : "=r" (v)); break;
    case 11: asm volatile("mrc p14, 0, %0, c0, c11, 7" : "=r" (v)); break;
    case 12: asm volatile("mrc p14, 0, %0, c0, c12, 7" : "=r" (v)); break;
    case 13: asm volatile("mrc p14, 0, %0, c0, c13, 7" : "=r" (v)); break;
    case 14: asm volatile("mrc p14, 0, %0, c0, c14, 7" : "=r" (v)); break;
    case 15: asm volatile("mrc p14, 0, %0, c0, c15, 7" : "=r" (v)); break;
    default: panic("Invalid watchpoint index");
    };
  return v;
}

PRIVATE static
void
Jdb_bp::bvr(int num, Mword v)
{
  switch (num)
    {
    case  0: asm volatile("mcr p14, 0, %0, c0,  c0, 4" : : "r" (v)); break;
    case  1: asm volatile("mcr p14, 0, %0, c0,  c1, 4" : : "r" (v)); break;
    case  2: asm volatile("mcr p14, 0, %0, c0,  c2, 4" : : "r" (v)); break;
    case  3: asm volatile("mcr p14, 0, %0, c0,  c3, 4" : : "r" (v)); break;
    case  4: asm volatile("mcr p14, 0, %0, c0,  c4, 4" : : "r" (v)); break;
    case  5: asm volatile("mcr p14, 0, %0, c0,  c5, 4" : : "r" (v)); break;
    case  6: asm volatile("mcr p14, 0, %0, c0,  c6, 4" : : "r" (v)); break;
    case  7: asm volatile("mcr p14, 0, %0, c0,  c7, 4" : : "r" (v)); break;
    case  8: asm volatile("mcr p14, 0, %0, c0,  c8, 4" : : "r" (v)); break;
    case  9: asm volatile("mcr p14, 0, %0, c0,  c9, 4" : : "r" (v)); break;
    case 10: asm volatile("mcr p14, 0, %0, c0, c10, 4" : : "r" (v)); break;
    case 11: asm volatile("mcr p14, 0, %0, c0, c11, 4" : : "r" (v)); break;
    case 12: asm volatile("mcr p14, 0, %0, c0, c12, 4" : : "r" (v)); break;
    case 13: asm volatile("mcr p14, 0, %0, c0, c13, 4" : : "r" (v)); break;
    case 14: asm volatile("mcr p14, 0, %0, c0, c14, 4" : : "r" (v)); break;
    case 15: asm volatile("mcr p14, 0, %0, c0, c15, 4" : : "r" (v)); break;
    default: panic("Invalid breakoint index");
    };
}

PRIVATE static
Mword
Jdb_bp::bvr(int num)
{
  Mword v;
  switch (num)
    {
    case  0: asm volatile("mrc p14, 0, %0, c0,  c0, 4" : "=r" (v)); break;
    case  1: asm volatile("mrc p14, 0, %0, c0,  c1, 4" : "=r" (v)); break;
    case  2: asm volatile("mrc p14, 0, %0, c0,  c2, 4" : "=r" (v)); break;
    case  3: asm volatile("mrc p14, 0, %0, c0,  c3, 4" : "=r" (v)); break;
    case  4: asm volatile("mrc p14, 0, %0, c0,  c4, 4" : "=r" (v)); break;
    case  5: asm volatile("mrc p14, 0, %0, c0,  c5, 4" : "=r" (v)); break;
    case  6: asm volatile("mrc p14, 0, %0, c0,  c6, 4" : "=r" (v)); break;
    case  7: asm volatile("mrc p14, 0, %0, c0,  c7, 4" : "=r" (v)); break;
    case  8: asm volatile("mrc p14, 0, %0, c0,  c8, 4" : "=r" (v)); break;
    case  9: asm volatile("mrc p14, 0, %0, c0,  c9, 4" : "=r" (v)); break;
    case 10: asm volatile("mrc p14, 0, %0, c0, c10, 4" : "=r" (v)); break;
    case 11: asm volatile("mrc p14, 0, %0, c0, c11, 4" : "=r" (v)); break;
    case 12: asm volatile("mrc p14, 0, %0, c0, c12, 4" : "=r" (v)); break;
    case 13: asm volatile("mrc p14, 0, %0, c0, c13, 4" : "=r" (v)); break;
    case 14: asm volatile("mrc p14, 0, %0, c0, c14, 4" : "=r" (v)); break;
    case 15: asm volatile("mrc p14, 0, %0, c0, c15, 4" : "=r" (v)); break;
    default: panic("Invalid breakoint index");
    };
  return v;
}

PRIVATE static
void
Jdb_bp::bcr(int num, Mword v)
{
  switch (num)
    {
    case  0: asm volatile("mcr p14, 0, %0, c0,  c0, 5" : : "r" (v)); break;
    case  1: asm volatile("mcr p14, 0, %0, c0,  c1, 5" : : "r" (v)); break;
    case  2: asm volatile("mcr p14, 0, %0, c0,  c2, 5" : : "r" (v)); break;
    case  3: asm volatile("mcr p14, 0, %0, c0,  c3, 5" : : "r" (v)); break;
    case  4: asm volatile("mcr p14, 0, %0, c0,  c4, 5" : : "r" (v)); break;
    case  5: asm volatile("mcr p14, 0, %0, c0,  c5, 5" : : "r" (v)); break;
    case  6: asm volatile("mcr p14, 0, %0, c0,  c6, 5" : : "r" (v)); break;
    case  7: asm volatile("mcr p14, 0, %0, c0,  c7, 5" : : "r" (v)); break;
    case  8: asm volatile("mcr p14, 0, %0, c0,  c8, 5" : : "r" (v)); break;
    case  9: asm volatile("mcr p14, 0, %0, c0,  c9, 5" : : "r" (v)); break;
    case 10: asm volatile("mcr p14, 0, %0, c0, c10, 5" : : "r" (v)); break;
    case 11: asm volatile("mcr p14, 0, %0, c0, c11, 5" : : "r" (v)); break;
    case 12: asm volatile("mcr p14, 0, %0, c0, c12, 5" : : "r" (v)); break;
    case 13: asm volatile("mcr p14, 0, %0, c0, c13, 5" : : "r" (v)); break;
    case 14: asm volatile("mcr p14, 0, %0, c0, c14, 5" : : "r" (v)); break;
    case 15: asm volatile("mcr p14, 0, %0, c0, c15, 5" : : "r" (v)); break;
    default: panic("Invalid breakoint index");
    };
}

PRIVATE static
Mword
Jdb_bp::bcr(int num)
{
  Mword v;
  switch (num)
    {
    case  0: asm volatile("mrc p14, 0, %0, c0,  c0, 5" : "=r" (v)); break;
    case  1: asm volatile("mrc p14, 0, %0, c0,  c1, 5" : "=r" (v)); break;
    case  2: asm volatile("mrc p14, 0, %0, c0,  c2, 5" : "=r" (v)); break;
    case  3: asm volatile("mrc p14, 0, %0, c0,  c3, 5" : "=r" (v)); break;
    case  4: asm volatile("mrc p14, 0, %0, c0,  c4, 5" : "=r" (v)); break;
    case  5: asm volatile("mrc p14, 0, %0, c0,  c5, 5" : "=r" (v)); break;
    case  6: asm volatile("mrc p14, 0, %0, c0,  c6, 5" : "=r" (v)); break;
    case  7: asm volatile("mrc p14, 0, %0, c0,  c7, 5" : "=r" (v)); break;
    case  8: asm volatile("mrc p14, 0, %0, c0,  c8, 5" : "=r" (v)); break;
    case  9: asm volatile("mrc p14, 0, %0, c0,  c9, 5" : "=r" (v)); break;
    case 10: asm volatile("mrc p14, 0, %0, c0, c10, 5" : "=r" (v)); break;
    case 11: asm volatile("mrc p14, 0, %0, c0, c11, 5" : "=r" (v)); break;
    case 12: asm volatile("mrc p14, 0, %0, c0, c12, 5" : "=r" (v)); break;
    case 13: asm volatile("mrc p14, 0, %0, c0, c13, 5" : "=r" (v)); break;
    case 14: asm volatile("mrc p14, 0, %0, c0, c14, 5" : "=r" (v)); break;
    case 15: asm volatile("mrc p14, 0, %0, c0, c15, 5" : "=r" (v)); break;
    default: panic("Invalid breakoint index");
    };
  return v;
}

/** @return 1 if only breakpoints were logged and jdb should not be entered */
IMPLEMENT
int
Jdb_bp::test_log_only(Cpu_number )
{
  // code for bp-logging
  return 0; // enter jdb
}

PRIVATE static
void
Jdb_bp::test_debug_on_cpu(Cpu_number cpu, void *data)
{
  Test_debug_data *d = reinterpret_cast<Test_debug_data *>(data);
  Mword v;
  asm volatile("mrc p14, 0, %0, c0, c1, 0" : "=r" (v));
  Mword moe = (v >> 2) & 0xf;

  Jdb_entry_frame *ef = Jdb::get_entry_frame(cpu);
  switch (moe)
    {
    case 1: // breakpoint
        {
          d->buf->printf("Break");

          d->type    = 'b';
          d->disable = true;
          d->addr    = ef->pc;
        }
      break;
    case 10: // synchr. watchpoint
        {
          Mword wfar;
          asm volatile("mrc p14, 0, %0, c0, c6, 0" : "=r" (wfar));
          wfar -= (ef->psr & Proc::Status_thumb) ? 4 : 8;

          String_buf<12> datacontent;
          Mword val = 0;

          if (Jdb::peek_task(ef->pf_address, Jdb::get_thread(cpu)->space(),
                             &val, 4) == 0)
            datacontent.printf(" [%lx]", val);

          String_buf<15> pcstr;
          if (wfar != ef->pc)
            pcstr.printf(" from %lx", wfar);

          d->buf->printf("Break on %s at %lx%s%s",
                         (ef->error_code & (1 << 11)) ? "wr" : "rd",
                         ef->pf_address, datacontent.c_str(), pcstr.c_str());

          d->type    = 'w';
          d->disable = true;
          d->addr    = ef->pf_address;
        }
      break;
    default:
      d->buf->printf("Unknown debug event 0x%lx", moe);
      break;
    }
}

PRIVATE static
void
Jdb_bp::disable_breakpoint_cpu(Cpu_number, void *data)
{
  Test_debug_data *d = reinterpret_cast<Test_debug_data *>(data);

  for (unsigned i = 0; i < num_breakpoints(); ++i)
    {
      if (bvr(i) == d->addr) // need to check for more conditions here
        bcr(i, bcr(i) & ~1);
    }
}

PUBLIC static
int
Jdb_bp::instruction_bp_at_addr(Address addr)
{
  for (unsigned i = 0; i < num_breakpoints(); ++i)
    if (bvr(i) == addr)
      return i + 1;
  return 0;
}


PRIVATE static
void
Jdb_bp::disable_watchpoint_cpu(Cpu_number, void *data)
{
  Test_debug_data *d = reinterpret_cast<Test_debug_data *>(data);

  for (unsigned i = 0; i < num_watchpoints(); ++i)
    {
      if (hw_version() < Vers_v7_1) // No DFAR here
        wcr(i, wcr(i) & ~1);
      else
        if (wvr(i) == d->addr) // need to check for more conditions here
          wcr(i, wcr(i) & ~1);
    }
}

PRIVATE static
void
Jdb_bp::set_bw(Cpu_number, void *data)
{
  Set_debug_data *d = reinterpret_cast<Set_debug_data *>(data);
  if (d->type == 'w')
    {
      wvr(d->idx, d->addr);
      wcr(d->idx, (wcr(d->idx) & d->cr_mask) | d->cr_val);
    }
  else
    {
      bvr(d->idx, d->addr);
      bcr(d->idx, (bcr(d->idx) & d->cr_mask) | d->cr_val);
    }
}

/** @return 1 if breakpoint occured */
IMPLEMENT
int
Jdb_bp::test_break(Cpu_number cpu, String_buffer *buf)
{
  Test_debug_data data;

  data.buf     = buf;
  data.disable = false;

  Jdb::remote_work(cpu, [&data](Cpu_number cpu){ test_debug_on_cpu(cpu, &data); },
                   true);

  // or do it on jdb exit?
  Jdb::on_each_cpu_pl([&data](Cpu_number cpu)
        {
          if (data.type == 'b')
            disable_breakpoint_cpu(cpu, &data);
          else
            disable_watchpoint_cpu(cpu, &data);
        });

  return 1;
}

PRIVATE
void
Jdb_bp::wp_bas(String_buffer *b, unsigned bas)
{
  for (unsigned i = 0; i < 8; ++i)
    if (bas == ((1u << i) - 1))
      {
        b->printf("len:%d", i);
        return;
      }

  for (unsigned i = 0; i < 8; ++i)
    b->append((bas & (1 << (7 - i))) ? '1' : '0');
}

PRIVATE
void
Jdb_bp::show_wp(unsigned idx)
{
  const char *lsc_str[4] = { "?", "rd", "wr", "all" };
  Mword v = wvr(idx), c = wcr(idx);

  if (c & 1)
    {
      String_buf<10> b;
      wp_bas(&b, (c >> 5) & 0xff);
      printf("W#%d: addr=%08lx type=%s bas=%s priv= WT=%ld LBN=%ld mask=%lx\n",
             idx, v, lsc_str[(c >> 3) & 3], b.c_str(),
             (c >> 20) & 1, (c >> 16) & 0xf,
             0UL);
    }
  else
    printf("W#%d: disabled\n", idx);
}

PRIVATE
void
Jdb_bp::show_bp(unsigned idx)
{
  Mword v = bvr(idx), c = bcr(idx);

  if (c & 1)
    {
      printf("B#%d: addr=%08lx type=%ld LBN=%ld mask=%lx\n",
             idx, v,
             (c >> 20) & 0xf,
             (c >> 16) & 0xf,
             (c >> 24) & 0x1f);
    }
  else
    printf("B#%d: disabled\n", idx);
}


PRIVATE
void
Jdb_bp::show_bps()
{
  putchar('\n');

  for (unsigned i = 0; i < num_breakpoints(); ++i)
    show_bp(i);

  for (unsigned i = 0; i < num_watchpoints(); ++i)
    show_wp(i);
}

PRIVATE static
const char *
Jdb_bp::version_string(unsigned vers)
{
  const char *versionstr[8] = { 0, "v6", "v6.1", "v7 base", "v7 all",
                                "v7.1", 0, 0 };

  return vers > 7 ? 0 : versionstr[vers];
}

PRIVATE static inline
Mword
Jdb_bp::dbgdidr()
{
  Mword v;
  asm volatile("mrc p14, 0, %0, c0, c0, 0" : "=r" (v));
  return v;
}

PRIVATE static inline
Mword
Jdb_bp::hw_version()
{
  return (dbgdidr() >> 16) & 0xf;
}

PRIVATE static
void
Jdb_bp::show_hwinfo_cpu(Cpu_number cpu)
{
  Mword v = dbgdidr();
  const char *vs = version_string(hw_version());
  printf("pCPU%d: Watchpoints=%ld Breakpoints=%ld Ctx_cmp=%ld "
         "\"%s\" variant=%lx rev=%lx\n",
         cxx::int_value<Cpu_phys_id>(Cpu::cpus.cpu(cpu).phys_id()),
         ((v >> 28) & 0xf) + 1,
         ((v >> 24) & 0xf) + 1,
         ((v >> 20) & 0xf) + 1,
         vs ? vs : "reserved",
         (v >> 4) & 0xf,
         (v >> 0) & 0xf);
}

PRIVATE static
bool
Jdb_bp::dbg_avail()
{
  return !!version_string(hw_version());
}

PRIVATE
int
Jdb_bp::get_free_wp()
{
  for (unsigned i = 0; i < num_watchpoints(); ++i)
    if (!(wcr(i) & 1))
      return i;

  return -1;
}

PRIVATE
int
Jdb_bp::get_free_bp()
{
  for (unsigned i = 0; i < num_breakpoints(); ++i)
    if (!(bcr(i) & 1))
      return i;

  return -1;
}

PUBLIC
Jdb_module::Action_code
Jdb_bp::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  enum State
  {
    Give_address = 1,
    Give_type = 2,
    Give_num = 3,
    Do_bp = 4,
    Do_mod = 5,
  };

  Jdb_module::Action_code code;

  if (!dbg_avail())
    {
      printf("\nNo hardware debugging support available.\n");
      return NOTHING;
    }

  if (!inited)
    {
      Jdb::on_each_cpu_pl(init_cpu);
      inited = true;
    }

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
          breakpoint_number = breakpoint_cmd == 'i'
                              ? get_free_bp() : get_free_wp();
          if (breakpoint_number < 0)
            {
              printf("\nNo more %spoints available.\n",
                     breakpoint_cmd == 'i' ? "break" : "watch");
              return NOTHING; // Could also select one for overwrite now
            }

          fmt   = " addr=%C";
          args  = &Jdb_input_task_addr::first_char;
          state = Give_address;
          return EXTRA_INPUT;
        case '-':
          // delete breakpoint
        case '+':
          // set logmode of breakpoint to <STOP>
        case '*':
          // set logmode of breakpoint to <LOG>
          fmt   = " type (b or w)=%C";
          args  = &breakpoint_type;
          state = Give_num;
          return EXTRA_INPUT;
        case 'l':
          show_bps();
          return NOTHING;
        case 'I':
          putchar('\n');
          Jdb::on_each_cpu(show_hwinfo_cpu, true);
          return NOTHING;
        default:
          return ERROR;
        };
    }
  else if (state == Give_address)
    {
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
    }

  if (state == Give_num)
    {
      if (breakpoint_type != 'w' && breakpoint_type != 'b')
        return NOTHING;
      fmt   = " nr=%2x";
      args  = &breakpoint_number;
      state = Do_mod;
      return EXTRA_INPUT;
    }

  if (state == Do_mod)
    {
      if (breakpoint_cmd == '-')
        {
          Set_debug_data d;
          d.idx     = breakpoint_number;
          d.type    = breakpoint_type;
          d.addr    = Jdb_input_task_addr::addr();
          d.cr_val  = 0;
          d.cr_mask = ~1UL;

          Jdb::on_each_cpu_pl([&d, this](Cpu_number cpu){ set_bw(cpu, &d); });
        }
      else
        printf("'%c' not yet handled\n", breakpoint_cmd);
      return NOTHING;
    }

  if (state == Do_bp)
    {
      if (breakpoint_length > 8)
        return NOTHING;

      putchar('\n');
      Set_debug_data d;

      // set watchpoint/breakpoint here
      if (breakpoint_cmd == 'i')
        {
          d.cr_val =  ( 0 << 24) // MASK: None
                    | ( 0 << 20) // WT: unlinked data address match
                    | ( 0 << 16) // LBN: none
                    | ( 0 << 14) // SSC: all
                    | ( 0 << 13) // HMC: no virt
                    | (15 << 5)  // BAS: could also do different values here
                    | ( 0 << 1)  // PMC: all
                    | ( 1 << 0); // enable


          d.idx     = breakpoint_number;
          d.type    = 'b';
          d.addr    = Jdb_input_task_addr::addr();
          d.cr_mask = 0;

          Jdb::on_each_cpu_pl([&d, this](Cpu_number cpu){ set_bw(cpu, &d); });

          show_bp(breakpoint_number);
        }
      else
        {
          unsigned l = (1 << breakpoint_length) - 1;
          unsigned a;
          switch (breakpoint_cmd)
            {
            case 'r': a = 1; break;
            case 'w': a = 2; break;
            default:  a = 3; break;
            };
          d.cr_val =  ( 0 << 24)  // MASK: no mask
                    | ( 0 << 20)  // BT: Unlinked insn addr match
                    | ( 0 << 16)  // LBN: none
                    | ( 0 << 14)  // SSC: all
                    | ( 0 << 13)  // HMC: no virt
                    | ( l <<  5)  // BAS: one bit per byte
                    | ( a <<  3)  // LSC: 3: all access match, 1: load, 2: store
                    | ( 3 <<  1)  // PAC all
                    | ( 1 <<  0); // enable

          d.idx     = breakpoint_number;
          d.type    = 'w';
          d.addr    = Jdb_input_task_addr::addr();
          d.cr_mask = 0;

          Jdb::on_each_cpu_pl([&d, this](Cpu_number cpu){ set_bw(cpu, &d); });

          show_wp(breakpoint_number);
        }
    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_bp::cmds() const
{
  static Cmd cs[] =
    {
        {
          0, "b", "bp", "%c",
          "b{i|a|r|w}<addr>\tset breakpoint on insn/access/read/write "
          "access\n"
          "b-{b|w}<nr>\tdisable breakpoint\n"
          "bl\tlist breakpoints\n"
          "bI\tshow info on hw debugging",
          &breakpoint_cmd
        },
    };

  return cs;
}

PUBLIC
int
Jdb_bp::num_cmds() const
{
  return 1;
}

static Jdb_bp jdb_set_bp INIT_PRIORITY(JDB_MODULE_INIT_PRIO);
