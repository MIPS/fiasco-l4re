//----------------------------------------------------------------------------
IMPLEMENTATION [amd64]:


PUBLIC template<typename T> inline
void FIASCO_NORETURN
Thread::fast_return_to_user(Mword ip, Mword sp, T arg)
{
  assert_kdb(cpu_lock.test());
  assert_kdb(current() == this);

  regs()->ip(ip);
  regs()->sp(sp);
  regs()->cs(Gdt::gdt_code_user | Gdt::Selector_user);
  regs()->flags(EFLAGS_IF);
  asm volatile
    ("mov %0, %%rsp \t\n"
     "iretq         \t\n"
     :
     : "r" (static_cast<Return_frame*>(regs())), "D"(arg)
    );
  __builtin_trap();
}

PROTECTED inline NEEDS[Thread::sys_gdt_x86]
L4_msg_tag
Thread::invoke_arch(L4_msg_tag tag, Utcb *utcb)
{
  switch (utcb->values[0] & Opcode_mask)
    {
    case Op_gdt_x86: return sys_gdt_x86(tag, utcb);
    case Op_set_segment_base_amd64:
      if (tag.words() < 2)
        return commit_result(-L4_err::EInval);
      switch (utcb->values[0] >> 16)
        {
        case 0:
          _fs = 0;
          _fs_base = utcb->values[1];
          if (current() == this)
            Cpu::wrmsr(_fs_base, MSR_FS_BASE);
          break;

        case 1:
          _gs = 0;
          _gs_base = utcb->values[1];
          if (current() == this)
            Cpu::wrmsr(_gs_base, MSR_GS_BASE);
          break;

        default: return commit_result(-L4_err::EInval);
        }
      return Kobject_iface::commit_result(0);
    default:
      return commit_result(-L4_err::ENosys);
    };
}

IMPLEMENT inline
Mword
Thread::user_sp() const
{ return exception_triggered()?_exc_cont.sp(regs()):regs()->sp(); }

IMPLEMENT inline
void
Thread::user_sp(Mword sp)
{
  if (exception_triggered())
    _exc_cont.sp(regs(), sp);
  else
    regs()->sp(sp);
}

PROTECTED inline
int
Thread::do_trigger_exception(Entry_frame *r, void *ret_handler)
{
  if (!exception_triggered())
    {
      _exc_cont.activate(r, ret_handler);
      return 1;
    }
  // else ignore change of IP because triggered exception already pending
  return 0;
}

PUBLIC inline
void
Thread::restore_exc_state()
{
  _exc_cont.restore(regs());
}

PRIVATE static inline
Return_frame *
Thread::trap_state_to_rf(Trap_state *ts)
{
  char *im = reinterpret_cast<char*>(ts + 1);
  return reinterpret_cast<Return_frame*>(im)-1;
}

PRIVATE static inline NEEDS[Thread::trap_state_to_rf]
bool FIASCO_WARN_RESULT
Thread::copy_utcb_to_ts(L4_msg_tag const &tag, Thread *snd, Thread *rcv,
                        L4_fpage::Rights rights)
{
  Trap_state *ts = (Trap_state*)rcv->_utcb_handler;
  Mword       s  = tag.words();
  Unsigned32  cs = ts->cs();
  Utcb *snd_utcb = snd->utcb().access();

  if (EXPECT_FALSE(rcv->exception_triggered()))
    {
      // triggered exception pending
      Mem::memcpy_mwords (ts, snd_utcb->values, s > 19 ? 19 : s);
      if (EXPECT_TRUE(s > 22))
	{
	  Continuation::User_return_frame const *s
	    = reinterpret_cast<Continuation::User_return_frame const *>((char*)&snd_utcb->values[19]);

	  rcv->_exc_cont.set(trap_state_to_rf(ts), s);
	}
    }
  else
    Mem::memcpy_mwords (ts, snd_utcb->values, s > 23 ? 23 : s);

  if (tag.transfer_fpu() && (rights & L4_fpage::Rights::W()))
    snd->transfer_fpu(rcv);

  // sanitize eflags
  ts->flags((ts->flags() & ~(EFLAGS_IOPL | EFLAGS_NT)) | EFLAGS_IF);

  // don't allow to overwrite the code selector!
  ts->cs(cs);

  bool ret = transfer_msg_items(tag, snd, snd_utcb,
                                rcv, rcv->utcb().access(), rights);

  rcv->state_del(Thread_in_exception);
  return ret;
}

PRIVATE static inline NEEDS[Thread::trap_state_to_rf]
bool FIASCO_WARN_RESULT
Thread::copy_ts_to_utcb(L4_msg_tag const &, Thread *snd, Thread *rcv,
                        L4_fpage::Rights rights)
{
  Trap_state *ts = (Trap_state*)snd->_utcb_handler;
  Utcb *rcv_utcb = rcv->utcb().access();
  {
    auto guard = lock_guard(cpu_lock);
    if (EXPECT_FALSE(snd->exception_triggered()))
      {
	Mem::memcpy_mwords (rcv_utcb->values, ts, 19);
	Continuation::User_return_frame *d
	    = reinterpret_cast<Continuation::User_return_frame *>((char*)&rcv_utcb->values[19]);

	snd->_exc_cont.get(d, trap_state_to_rf(ts));
      }
    else
      Mem::memcpy_mwords (rcv_utcb->values, ts, 23);

    if (rcv_utcb->inherit_fpu() && (rights & L4_fpage::Rights::W()))
      snd->transfer_fpu(rcv);
}
  return true;
}


IMPLEMENT
void
Thread::user_invoke()
{
  user_invoke_generic();

  Mword di = 0;

  if (current()->space()->is_sigma0())
    di = Kmem::virt_to_phys(Kip::k());

  asm volatile
    ("  mov %%rax,%%rsp \n"    // set stack pointer to regs structure
     "  mov %%ecx,%%es   \n"
     "  mov %%ecx,%%ds   \n"
     "  xor %%rax,%%rax \n"
     "  xor %%rcx,%%rcx \n"     // clean out user regs
     "  xor %%rdx,%%rdx \n"
     "  xor %%rsi,%%rsi \n"
     "  xor %%rbx,%%rbx \n"
     "  xor %%rbp,%%rbp \n"
     "  xor %%r8,%%r8   \n"
     "  xor %%r9,%%r9   \n"
     "  xor %%r10,%%r10 \n"
     "  xor %%r11,%%r11 \n"
     "  xor %%r12,%%r12 \n"
     "  xor %%r13,%%r13 \n"
     "  xor %%r14,%%r14 \n"
     "  xor %%r15,%%r15 \n"

     "  iretq           \n"
     :                          // no output
     : "a" (nonull_static_cast<Return_frame*>(current()->regs())),
       "c" (Gdt::gdt_data_user | Gdt::Selector_user),
       "D" (di)
     );

  // never returns here
}

PRIVATE inline
int
Thread::check_trap13_kernel (Trap_state * /*ts*/)
{ return 1; }


//----------------------------------------------------------------------------
IMPLEMENTATION [amd64 & (debug | kdb)]:

#include "kernel_task.h"

/** Call the nested trap handler (either Jdb::enter_kdebugger() or the
 * gdb stub. Setup our own stack frame */
PRIVATE static
int
Thread::call_nested_trap_handler(Trap_state *ts)
{
  Proc::cli();

  Cpu_number log_cpu = dbg_find_cpu();
  unsigned long &ntr = nested_trap_recover.cpu(log_cpu);

#if 0
  printf("%s: lcpu%u sp=%p t=%u nested_trap_recover=%ld\n",
      __func__, log_cpu, (void*)Proc::stack_pointer(), ts->_trapno,
      ntr);
#endif

  Unsigned64 ret;
  void *stack = 0;
  if (!ntr)
    stack = dbg_stack.cpu(log_cpu).stack_top;

  Unsigned64 dummy1, dummy2, dummy3;

  // don't set %esp if gdb fault recovery to ensure that exceptions inside
  // kdb/jdb don't overwrite the stack
  asm volatile
    ("mov    %%rsp,%[d2]	\n\t"	// save old stack pointer
     "cmpq   $0,%[recover]	\n\t"
     "jne    1f			\n\t"	// check trap within trap handler
     "mov    %[stack],%%rsp	\n\t"	// setup clean stack pointer
     "1:			\n\t"
     "incq   %[recover]		\n\t"
     "mov    %%cr3, %[d1]	\n\t"
     "push   %[d2]		\n\t"	// save old stack pointer on new stack
     "push   %[d1]		\n\t"	// save old pdbr
     "mov    %[pdbr], %%cr3	\n\t"
     "callq  *%[handler]	\n\t"
     "pop    %[d1]		\n\t"
     "mov    %[d1], %%cr3	\n\t"
     "pop    %%rsp		\n\t"	// restore old stack pointer
     "cmpq   $0,%[recover]	\n\t"	// check trap within trap handler
     "je     1f			\n\t"
     "decq   %[recover]		\n\t"
     "1:			\n\t"
     : [ret] "=a"(ret), [d2] "=&r"(dummy2), [d1] "=&r"(dummy1), "=D"(dummy3),
       [recover] "+m" (ntr)
     : [ts] "D" (ts),
       [pdbr] "r" (Kernel_task::kernel_task()->virt_to_phys((Address)Kmem::dir())),
       [cpu] "S" (log_cpu),
       [stack] "r" (stack),
       [handler] "m" (nested_trap_handler)
     : "rdx", "rcx", "r8", "r9", "memory");

  if (!ntr)
    handle_global_requests();

  return ret == 0 ? 0 : -1;
}

