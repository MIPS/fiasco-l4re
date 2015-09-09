IMPLEMENTATION [mips32-debug]:

#include <cstdio>
#include <simpleio.h>
#include "jdb_tbuf.h"
#include "jdb_entry_frame.h"
#include "kdb_ke.h"
#include "cpu_lock.h"
#include "vkey.h"
#include "static_init.h"
#include "thread.h"
#include "processor.h"

//---------------------------------------------------------------------------
// the following register usage matches ABI in l4sys/include/ARCH-mips/kdebug.h

static inline Mword param1(Trap_state *ts)
{ return (ts->r[Syscall_frame::REG_T1]); }

static inline void set_param1(Trap_state *ts, Mword val)
{ ts->r[Syscall_frame::REG_T1] = val; }

static inline Mword param2(Trap_state *ts)
{ return (ts->r[Syscall_frame::REG_T2]); }

//------------------------------------------------------------------------------
static void outchar(Thread *, Trap_state *ts)
{ putchar(param1(ts) & 0xff); }

static void outstring(Thread *, Trap_state *ts)
{ putstr((char*)param1(ts)); }

static void outnstring(Thread *, Trap_state *ts)
{ putnstr((char*)param1(ts), param2(ts)); }

static void outdec(Thread *, Trap_state *ts)
{ printf("%ld", param1(ts)); }

static void outhex(Thread *, Trap_state *ts)
{ printf("%08lx", param1(ts)); }

static void outhex20(Thread *, Trap_state *ts)
{ printf("%05lx", param1(ts) & 0xfffff); }

static void outhex16(Thread *, Trap_state *ts)
{ printf("%04lx", param1(ts) & 0xffff); }

static void outhex12(Thread *, Trap_state *ts)
{ printf("%03lx", param1(ts) & 0xfff); }

static void outhex8(Thread *, Trap_state *ts)
{ printf("%02lx", param1(ts) & 0xff); }

static void inchar(Thread *, Trap_state *ts)
{
  set_param1(ts, Vkey::get());
  Vkey::clear();
}

static void tbuf(Thread *t, Trap_state *ts)
{
  Mem_space *s = t->mem_space();
  Address ip = ts->ip();
  Address_type user;
  Unsigned8 *str;
  int len;
  char c;

  Jdb_entry_frame *entry_frame = reinterpret_cast<Jdb_entry_frame*>(ts);
  user = entry_frame->from_user();

  switch (entry_frame->param())
    {
    case 0: // fiasco_tbuf_get_status()
	{
	  Jdb_status_page_frame *regs =
	    reinterpret_cast<Jdb_status_page_frame*>(entry_frame);
	  regs->set(Mem_layout::Tbuf_ustatus_page);
	}
      break;
    case 1: // fiasco_tbuf_log()
	{
	  Jdb_log_frame *regs = reinterpret_cast<Jdb_log_frame*>(entry_frame);
	  Tb_entry_ke *tb = Jdb_tbuf::new_entry<Tb_entry_ke>();
          str = regs->str();
	  tb->set(t, ip-4);
	  for (len=0; (c = s->peek(str++, user)); len++)
            tb->set_buf(len, c);
          tb->term_buf(len);
          regs->set_tb_entry(tb);
          Jdb_tbuf::commit_entry();
	}
      break;
    case 2: // fiasco_tbuf_clear()
      Jdb_tbuf::clear_tbuf();
      break;
    case 3: // fiasco_tbuf_dump()
      return; // => Jdb
    case 4: // fiasco_tbuf_log_3val()
        {
          // interrupts are disabled in handle_slow_trap()
          Jdb_log_3val_frame *regs =
            reinterpret_cast<Jdb_log_3val_frame*>(entry_frame);
          Tb_entry_ke_reg *tb = Jdb_tbuf::new_entry<Tb_entry_ke_reg>();
          str = regs->str();
          tb->set(t, ip-4);
          tb->v[0] = regs->val1();
          tb->v[1] = regs->val2();
          tb->v[2] = regs->val3();
          for (len=0; (c = s->peek(str++, user)); len++)
            tb->set_buf(len, c);
          tb->term_buf(len);
          regs->set_tb_entry(tb);
          Jdb_tbuf::commit_entry();
        }
      break;
    case 5: // fiasco_tbuf_get_status_phys()
        {
          Jdb_status_page_frame *regs =
            reinterpret_cast<Jdb_status_page_frame*>(entry_frame);
          regs->set(s->virt_to_phys(Mem_layout::Tbuf_ustatus_page));
        }
      break;
    case 6: // fiasco_timer_disable
      printf("JDB: no more timer disable\n");
      //Timer::disable();
      break;
    case 7: // fiasco_timer_enable
      printf("JDB: no more timer enable\n");
      //Timer::enable();
      break;
    case 8: // fiasco_tbuf_log_binary()
      // interrupts are disabled in handle_slow_trap()
      Jdb_log_frame *regs = reinterpret_cast<Jdb_log_frame*>(entry_frame);
      Tb_entry_ke_bin *tb = Jdb_tbuf::new_entry<Tb_entry_ke_bin>();
      str = regs->str();
      tb->set(t, ip-4);
      for (len=0; len < Tb_entry_ke_bin::SIZE; len++)
        tb->set_buf(len, s->peek(str++, user));
      regs->set_tb_entry(tb);
      Jdb_tbuf::commit_entry();
      break;
    }
}

static void init_dbg_extensions()
{
  Thread::dbg_extension[0x01] = &outchar;
  Thread::dbg_extension[0x02] = &outstring;
  Thread::dbg_extension[0x03] = &outnstring;
  Thread::dbg_extension[0x04] = &outdec;
  Thread::dbg_extension[0x05] = &outhex;
  Thread::dbg_extension[0x06] = &outhex20;
  Thread::dbg_extension[0x07] = &outhex16;
  Thread::dbg_extension[0x08] = &outhex12;
  Thread::dbg_extension[0x09] = &outhex8;
  Thread::dbg_extension[0x0d] = &inchar;
  Thread::dbg_extension[0x1d] = &tbuf;
}

STATIC_INITIALIZER(init_dbg_extensions);

