/*
 * Derived in part from linux arch/mips/kernel/branch.c
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 97, 2000, 2001 by Ralf Baechle
 * Copyright (C) 2001 MIPS Technologies, Inc.
 */

INTERFACE [mips32]:

class Trap_state;

IMPLEMENTATION [mips32]:

#include <cassert>
#include <cstdio>
#include <feature.h>

#include "globals.h"
#include "kmem.h"
#include "mem_space.h"
#include "thread_state.h"
#include "trap_state.h"
#include "types.h"
#include "processor.h"
#include "utcb_support.h"
#include "thread.h"
#include "mipsregs.h"
#include "mipsvzregs.h"
#include "inst.h"

DEFINE_PER_CPU Per_cpu<Thread::Dbg_stack> Thread::dbg_stack;

/* 
 * The catch-all trap entry point.  Called by assembly code when a 
 * CPU trap (that's not specially handled, such as system calls) occurs.
 */
extern "C" FIASCO_FASTCALL
int
thread_handle_trap(Trap_state *ts, unsigned cpu, bool is_guestcontext)
{
  Mword exc_code = MIPS_TRAPTYPE(ts->cause);
  Mword badinstr = Thread::get_badinstr(ts);
  int handled = 0;

  assert(!is_guestcontext); // is_guestcontext not used in thread_handle_trap

  ts->set_errorcode(exc_code);

  if (!is_guestcontext) {
    switch (exc_code) {
      case Trap_state::Exc_code_Mod:
      case Trap_state::Exc_code_TLBL:
      case Trap_state::Exc_code_TLBS:
        return handle_tlb_exceptions(ts, cpu, is_guestcontext);
#ifdef CONFIG_JDB
      case Trap_state::Exc_code_Bp:
        Thread::set_debug_errorcode(ts);
        return Thread::call_nested_trap_handler(ts);
#endif
      case Trap_state::Exc_code_CpU:
        handled = Thread::handle_cop_unusable_exc(ts);
        break;

      case Trap_state::Exc_code_FPE:
        handled = Thread::handle_fpe(ts);
        break;

      default:
        Thread::print_trap_info("Trap handler", ts, cpu, is_guestcontext, badinstr);
        ts->Return_frame::dump();
        ts->Syscall_frame::dump();

        if (exc_code == Trap_state::Exc_code_MCheck)
          Mem_unit::dump_tlbs();

        if ((exc_code != Trap_state::Exc_code_AdEL) && 
            (exc_code != Trap_state::Exc_code_AdES))
          ts->badvaddr = 0; /* set badvaddr invalid */

        if (!MIPS_USERMODE(ts->status))
          panic("Unhandled exception in Kernel mode\n");

        break;
    }
  }

  if (!handled)
    handled =  current_thread()->handle_slow_trap(ts, cpu, is_guestcontext, badinstr);

  return handled;
}

PUBLIC static
void
Thread::print_trap_info(const char *msg, Trap_state *ts,
        unsigned cpu, bool guest, Mword badinstr)
{
  printf("%d:(%s%s) %s :: Trap type %s %ld\n"
          "Status %08lx EPC %08lx BadVaddr %08lx BadInstr %08lx\n",
          cpu, (guest ? "Guest ":""),
          (MIPS_USERMODE(ts->status) ? "User Mode" : "Kernel Mode"), msg,
          ts->exc_code_str(ts->trapno()), ts->trapno(),
          ts->status, ts->epc, ts->badvaddr, badinstr);
}

PRIVATE static
void
Thread::print_page_fault_error(Mword e)
{
  Mword exc_code = (e & Trap_state::Exc_code_mask);
  printf("%s (%ld), %s(%c)",
         Trap_state::exc_code_str(exc_code), exc_code,
         PF::is_usermode_error(e) ? "user" : "kernel",
         PF::is_read_error(e) ? 'r' : 'w');
}

//
// Public services
//
PUBLIC template<typename T> inline
void FIASCO_NORETURN
Thread::fast_return_to_user(Mword ip, Mword sp, T arg)
{
  Trap_state *ts = nonull_static_cast<Trap_state*>(regs());
  assert_kdb(cpu_lock.test());
  assert_kdb(current() == this);
  assert_kdb(ts->user_mode());

  ts->ip(ip);
  ts->sp(sp);

  {
    extern char fast_vcpu_resume[];
    asm volatile
      ("  move  $k1, %[entry_ts]  \n"
       "  move  $a0, %[arg]       \n"
       "  jr    %[rfe]            \n"
       "  nop                     \n"
       :
       :
       [entry_ts] "r" (ts), [arg] "r" (arg),
       [rfe] "r" (&fast_vcpu_resume)
       : "k1", "a0"
      );
  }

  panic("__builtin_trap()");
}

/*
 * Thread general exception handler for both user and 
 * kernel mode exceptions
 * Forwards the call to appropriate trap handlers 
 *
 * @return 0 if trap has been consumed by handler
 *         -1 if trap could not be handled.
 */
PUBLIC
int
Thread::handle_slow_trap(Trap_state *ts, unsigned cpu, bool guest, Mword badinstr)
{
  // send exception IPC if requested
  if (send_exception(ts))
    print_trap_info("Slow Trap IPC exception", ts, cpu, guest, badinstr);
  else
    print_trap_info("Slow Trap", ts, cpu, guest, badinstr);

  return 0;
}

/*
 * Call this method from within a Guest VM fiasco kernel to verify that
 * FRAME_USP is not clobbering FRAME_SP.
 *
 * NOTE: Only enable when testing, the emulated read from ddatalo is costly.
 *
 * The test saves ddatalo (the utcb) in $sp. This causes an exception and the
 * store to $sp is emulated by the host fiasco kernel. If in exception.S $sp is
 * restored from FRAME_USP when it should actually be restored from FRAME_SP
 * then this test will fail.
 */
PRIVATE
void
Thread::assertion_test_for_frame_sp()
{
  Mword save_sp = 0;
  Mword diff;
  Mword utcb = read_c0_ddatalo();

  asm volatile
    ("  move  %[save_sp], $sp     \n"
     "  mfc0  $sp, $28, 3         \n"
     "  xor   %[diff],%[utcb],$sp \n"
     "  move  $sp,%[save_sp]      \n"
     : [save_sp] "=r" (save_sp), [diff] "=r" (diff)
     : [utcb] "r" (utcb), "0" (save_sp)
     );
  assert(!diff);
}

/* ERET to user mode */

IMPLEMENT
void
Thread::user_invoke()
{

  user_invoke_generic();
  assert(current()->state() & Thread_ready);

  Trap_state *ts = nonull_static_cast<Trap_state*>(current()->regs());

  static_assert(sizeof(ts->r[0]) == sizeof(Mword), "Size mismatch");

  Mem::memset_mwords(&ts->r[0], 0, sizeof(ts->r) / sizeof(ts->r[0]));

  if (EXPECT_FALSE(current_thread()->mem_space()->is_sigma0())) {
    /* pass in kip phys address via $a0 */
    ts->r[Syscall_frame::REG_A0] = Mem_layout::pmem_to_phys(Kip::k());
  }

#if 1 // DEBUG
  {
    Mword utcb = read_c0_ddatalo();
    printf("\n[%lx] leaving kernel: ip %lx sp %lx utcb: 0x%08lx\n",
           current_thread()->dbg_id(), ts->ip(), ts->sp(), utcb);
    //assertion_test_for_frame_sp();
  }
#endif

  /* Sync the icache */
  Mem_op::cache()->icache_invalidate_all();

  /*
   * set user mode, enable interrupts and interrupt masks, and
   * set EXL to prepare for eret to user mode
   */
  ts->status = Cp0_Status::status_eret_to_user_ei(0);

  extern char user_invoke_entrypoint[];
  asm volatile
    ("  move  $k1, %[entry_ts]  \n"
     "  jr    %[rfe]            \n"
     "  nop                     \n"
     :
     :
     [entry_ts] "r" (ts),
     [rfe]      "r" (&user_invoke_entrypoint)
     );

  panic("should never be reached");
  while (1)
    {
      current()->state_del(Thread_ready);
      current()->schedule();
    };

  // never returns
}

IMPLEMENT inline NEEDS["space.h", <cstdio>, "types.h" ,"config.h"]
bool Thread::handle_sigma0_page_fault(Address pfa)
{
  if (Config::have_superpages) {
    return (mem_space()->v_insert(Mem_space::Phys_addr((pfa & Config::SUPERPAGE_MASK)),
                                  Virt_addr(pfa & Config::SUPERPAGE_MASK),
                                  Mem_space::Page_order(Config::SUPERPAGE_SHIFT),
                                  Mem_space::Attr(L4_fpage::Rights::URWX())
                                 )
        != Mem_space::Insert_err_nomem);
  } else {
    return (mem_space()->v_insert(Mem_space::Phys_addr(pfa & Config::PAGE_MASK),
                                  Virt_addr(pfa & Config::PAGE_MASK),
                                  Mem_space::Page_order(Config::PAGE_SHIFT),
                                  Mem_space::Attr(L4_fpage::Rights::URWX())
                                 )
        != Mem_space::Insert_err_nomem);
  }
}


/**
 * The low-level page fault handler.  We're invoked with
 * interrupts turned off.  Apart from turning on interrupts in almost
 * all cases (except for kernel page faults in TCB area), just forwards
 * the call to Thread::handle_page_fault().
 * @param pfa page-fault virtual address
 * @param error_code CPU error code
 * @return true if page fault could be resolved, false otherwise
 */
extern "C" FIASCO_FASTCALL
Mword
thread_page_fault(const Mword pfa, const Mword error_code,
                      const Mword epc, Trap_state *ts)
{

  Thread *t = current_thread();
  // Page fault in user mode or interrupts were enabled
  if (PF::is_usermode_error(error_code))
    {
      if (t->vcpu_pagefault(pfa, error_code, epc))
        return 1;

      Proc::sti();
    }
  else if (Proc::interrupts((Proc::Status)ts->status))
    {
      Proc::sti();
    }
  // Page fault in kernel mode and interrupts were disabled
  else
    {
      // page fault in kernel memory region
      if (Kmem::is_kmem_page_fault(pfa, error_code))
        {
          // We've interrupted a context in the kernel with disabled interrupts,
          // the page fault address is in the kernel region, the error code is
          // "not mapped" (as opposed to "access error"), and the region is
          // actually valid (that is, mapped in Kmem's shared page directory,
          // just not in the currently active page directory)
          // Remain cli'd !!!
        }
      else
        {
          // No error -- just enable interrupts.
          Proc::sti();
        }
    }

  return t->handle_page_fault(pfa, error_code, epc, nonull_static_cast<Return_frame*>(ts));
}

/**
 * The low-level TLB exception handler called from exception.S.
 * We're invoked with interrupts turned off.
 * @return 0 if exception could be resolved, -1 otherwise
 */
extern "C" FIASCO_FASTCALL
int
handle_tlb_exceptions(Trap_state *ts, unsigned cpu, bool is_guestcontext)
{
  Mword epc = ts->epc;
  Mword badvaddr = ts->badvaddr;
  Mword exc_code = MIPS_TRAPTYPE(ts->cause);
  Mword error_code = exc_code;
  int handled = 1;
  Space::Ku_mem const *ku_mem = 0;

  ts->set_errorcode(exc_code);

  Thread* t = current_thread();
  if (!t)
    panic ("No current thread");

  Space* space = t->vcpu_aware_space();

  if (exc_code == Trap_state::Exc_code_Mod)
    error_code |= PF::PF_ERR_WRITEPROT;
  else if (exc_code == Trap_state::Exc_code_TLBS)
    error_code |= (PF::PF_ERR_WRITEPROT | PF::PF_ERR_NOTPRESENT);
  else if (exc_code == Trap_state::Exc_code_TLBL)
    error_code |= PF::PF_ERR_NOTPRESENT;
  else {
    printf("handle_tlb_exceptions: invalid exc_code %ld\n", exc_code);
    handled = 0;
    goto out;
  }

  if (MIPS_USERMODE(ts->status))
    error_code |= PF::PF_ERR_USRMODE;

  if (is_guestcontext) {
    error_code |= PF::PF_ERR_VZGUEST;
    if (t->vcpu_pagefault(badvaddr, error_code, epc)) {
      ts->set_pagefault(badvaddr, error_code);
      handled = 1;
    } else {
      if (exc_code != Trap_state::Exc_code_Mod && space->v_pagein_mem(Virt_addr(badvaddr))) {
        handled = 1;
      } else {
        ts->set_pagefault(badvaddr, error_code);
        handled = 0;
      }
#if 0
      printf("Mapping MEM for guest VM (handled:%d) epc %08lx, badvaddr %08lx, asid %lx, vzguestid %lx\n",
        handled, epc, badvaddr, space->c_asid(), space->c_vzguestid());
#endif
    }
    return handled;
  }

  assert(!is_guestcontext);

  /* Handle KMEM traps before kernel thread is initialized */
  if (Kmem::is_kmem_page_fault((Address)badvaddr, error_code)) {
    if (Mem_space::v_pagein_kmem(Virt_addr(badvaddr))) {
      printf("Mapping KMEM for epc %08lx, badvaddr %08lx\n", epc, badvaddr);
      handled = 1;
      goto out;
    }
    else
      panic("Cannot find KMEM mapping for badvaddr %08lx @ epc %08lx", badvaddr, epc);
  } 

  if (exc_code != Trap_state::Exc_code_Mod ||
      /* handle kernel-user mappings directly */
      (ku_mem = space->find_ku_mem(User<void>::Ptr((void*)(badvaddr & Config::PAGE_MASK)),
                                   Config::PAGE_SIZE)))
  {
    if (space->v_pagein_mem(Virt_addr(badvaddr))) {
#if 0
      if (ku_mem)
        printf("Mapping KU_MEM for epc %08lx, badvaddr %08lx, asid %lx, vzguestid %lx\n",
          epc, badvaddr, space->c_asid(), space->c_vzguestid());
      else
        printf("Mapping MEM for epc %08lx, badvaddr %08lx, asid %lx, vzguestid %lx\n",
            epc, badvaddr, space->c_asid(), space->c_vzguestid());
#endif
      handled = 1;
      goto out;
    }
  }

  ts->set_pagefault(badvaddr, error_code);
  handled = thread_page_fault(badvaddr, error_code, epc, ts);

out:
  if (!handled) {
    Thread::print_trap_info("TLB handler", ts, cpu, is_guestcontext, 0);
    ts->Return_frame::dump();
    ts->Syscall_frame::dump();
    panic("TLB exception not handled");
  }

  return handled;
}

/* compute_return_epc borrows heavily from arch/mips/kernel/branch.c */

#define INVALID_INST      0xDEADBEEF
static unsigned long
compute_return_epc(Trap_state *ts, unsigned long instpc)
{
	unsigned int bit, fcr31, dspcontrol;
	union mips_instruction insn;
	long epc = instpc;
	long nextpc = INVALID_INST;

	if (epc & 3)
		goto unaligned;

	/*
	 * Read the instruction
	 */
	insn.word = *(Unsigned32 *)epc; // TODO implement as peek

	if (insn.word == INVALID_INST)
		return INVALID_INST;

	switch (insn.i_format.opcode) {
	/*
	 * jr and jalr are in r_format format.
	 */
	case spec_op:
		switch (insn.r_format.func) {
		case jalr_op:
			ts->r[insn.r_format.rd] = epc + 8;
			/* Fall through */
		case jr_op:
			nextpc = ts->r[insn.r_format.rs];
			break;
		}
		break;

	/*
	 * This group contains:
	 * bltz_op, bgez_op, bltzl_op, bgezl_op,
	 * bltzal_op, bgezal_op, bltzall_op, bgezall_op.
	 */
	case bcond_op:
		switch (insn.i_format.rt) {
		case bltz_op:
		case bltzl_op:
			if ((long)ts->r[insn.i_format.rs] < 0)
				epc = epc + 4 + (insn.i_format.simmediate << 2);
			else
				epc += 8;
			nextpc = epc;
			break;

		case bgez_op:
		case bgezl_op:
			if ((long)ts->r[insn.i_format.rs] >= 0)
				epc = epc + 4 + (insn.i_format.simmediate << 2);
			else
				epc += 8;
			nextpc = epc;
			break;

		case bltzal_op:
		case bltzall_op:
			ts->r[31] = epc + 8;
			if ((long)ts->r[insn.i_format.rs] < 0)
				epc = epc + 4 + (insn.i_format.simmediate << 2);
			else
				epc += 8;
			nextpc = epc;
			break;

		case bgezal_op:
		case bgezall_op:
			ts->r[31] = epc + 8;
			if ((long)ts->r[insn.i_format.rs] >= 0)
				epc = epc + 4 + (insn.i_format.simmediate << 2);
			else
				epc += 8;
			nextpc = epc;
			break;
		case bposge32_op:
			if (!cpu_has_dsp)
				goto sigill;

			dspcontrol = rddsp(0x01);

			if (dspcontrol >= 32) {
				epc = epc + 4 + (insn.i_format.simmediate << 2);
			} else
				epc += 8;
			nextpc = epc;
			break;
		}
		break;

	/*
	 * These are unconditional and in j_format.
	 */
	case jal_op:
		ts->r[31] = instpc + 8;
	case j_op:
		epc += 4;
		epc >>= 28;
		epc <<= 28;
		epc |= (insn.j_format.target << 2);
		nextpc = epc;
		break;

	/*
	 * These are conditional and in i_format.
	 */
	case beq_op:
	case beql_op:
		if (ts->r[insn.i_format.rs] ==
		    ts->r[insn.i_format.rt])
			epc = epc + 4 + (insn.i_format.simmediate << 2);
		else
			epc += 8;
		nextpc = epc;
		break;

	case bne_op:
	case bnel_op:
		if (ts->r[insn.i_format.rs] !=
		    ts->r[insn.i_format.rt])
			epc = epc + 4 + (insn.i_format.simmediate << 2);
		else
			epc += 8;
		nextpc = epc;
		break;

	case blez_op: /* not really i_format */
	case blezl_op:
		/* rt field assumed to be zero */
		if ((long)ts->r[insn.i_format.rs] <= 0)
			epc = epc + 4 + (insn.i_format.simmediate << 2);
		else
			epc += 8;
		nextpc = epc;
		break;

	case bgtz_op:
	case bgtzl_op:
		/* rt field assumed to be zero */
		if ((long)ts->r[insn.i_format.rs] > 0)
			epc = epc + 4 + (insn.i_format.simmediate << 2);
		else
			epc += 8;
		nextpc = epc;
		break;

	/*
	 * And now the FPA/cp1 branch instructions.
	 */
	case cop1_op:
#ifndef CONFIG_FPU
                (void)fcr31; (void)bit;
                printf("%s: unsupported cop1_op\n", __func__);
#else
		preempt_disable();
		Thread *t = current_thread();
		assert(Fpu::fpu.current().is_owner(current()) == (t->state() & Thread_fpu_owner));
		if (Fpu::fpu.current().is_owner(current()) && (t->state() & Thread_fpu_owner))
			fcr31 = Fpu::fcr_read();
		else
			fcr31 = Fpu::fcr(t->fpu_state());
		preempt_enable();

		bit = (insn.i_format.rt >> 2);
		bit += (bit != 0);
		bit += 23;
		switch (insn.i_format.rt & 3) {
		case 0: /* bc1f */
		case 2: /* bc1fl */
			if (~fcr31 & (1 << bit))
				epc = epc + 4 + (insn.i_format.simmediate << 2);
			else
				epc += 8;
			nextpc = epc;
			break;

		case 1: /* bc1t */
		case 3: /* bc1tl */
			if (fcr31 & (1 << bit))
				epc = epc + 4 + (insn.i_format.simmediate << 2);
			else
				epc += 8;
			nextpc = epc;
			break;
		}
#endif
		break;
	}

	return nextpc;

unaligned:
	printf("%s: unaligned epc\n", __func__);
	return nextpc;

sigill:
	printf("%s: DSP branch but not DSP ASE\n", __func__);
	return nextpc;
}

static int
update_pc(Trap_state *ts, Mword cause)
{
  unsigned long branch_pc;

  if (cause & CAUSEF_BD) {
    branch_pc = compute_return_epc(ts, ts->epc);
    if (branch_pc == INVALID_INST) {
      panic("Failed to emulate branch");
    } else {
      ts->epc = branch_pc;
    }
  } else
    ts->epc += 4;

  if (cause & CAUSEF_BD)
    printf("[%d] update_pc(): New PC: %#lx\n", cause & CAUSEF_BD ? 1 : 0, ts->epc);

  return 0;
}


PUBLIC static
bool
Thread::handle_cop0_unusable_exc(Trap_state *ts)
{
  /* Check cause register to figure out which co-processor */
  Mword cause = ts->cause;
  Mword epc = ts->epc;
  Mword inst, rt, rd, copz, sel, co_bit;
  bool handled = false;

  if (cause & CAUSEF_BD)
    {
      epc += 4;
      panic("EXC in BD slot not supported @ %08lx\n", epc);
    }

  /* Get instruction */
  inst = *(Unsigned32 *)epc; // TODO implement as peek
  copz = (inst >> 21) & 0x1f;
  rt = (inst >> 16) & 0x1f;
  rd = (inst >> 11) & 0x1f;
  sel = inst & 0x7;
  co_bit = (inst >> 25) & 1;

  /* Reading DDataLo (COP0, 28, 3) i.e. the UTCB */
  if (copz == mfc_op && rd == 28 && sel == 3)
    {
      ts->r[rt] = read_c0_ddatalo();
      //printf(">>>>>>>>>>>>>> Reading UTCB H/W Register @ %08lx: %08lx <<<<<<<<<<<<<<<<\n", epc, ts->r[rt]);
      update_pc(ts, cause);
      handled = true;
    }
  /* Setting the TLS by writing to UserLocal */
  else if (copz == mtc_op && rd == 4 && sel == 2)
    {
      //printf("************Writing TLS H/W Register @ %08lx: %08lx *******************\n", epc, ts->r[rt]);
      write_c0_userlocal(ts->r[rt]);
      update_pc(ts, cause);
      handled = true;
    }
  else
    printf("Unsupported COP0 operation @ %08lx\n", epc);

  return handled;
}

PUBLIC static
bool
Thread::handle_cop_unusable_exc(Trap_state *ts)
{
  /* Check cause register to figure out which co-processor */
  Mword cop = (ts->cause & CAUSEF_CE) >> CAUSEB_CE;
  bool handled = false;

  switch (cop)
    {
    case 0:
      handled = Thread::handle_cop0_unusable_exc(ts);
      break;
    case 1:
      handled = Thread::handle_fpu_trap(ts);
      break;
    default:
      panic("Unsupported COP%d operation @ %08lx", (int)cop, ts->epc);
    }
  return handled;
}

/*
 *  Fetch the bad instruction register
 */
PUBLIC static
Mword
Thread::get_badinstr(Trap_state *ts)
{
  Mword exccode = MIPS_TRAPTYPE(ts->cause);

  if (!cpu_has_badinstr)
      return 0;

  bool bad_instr_valid = Trap_state::is_badinstr_valid(exccode);

  if (exccode == Trap_state::Exc_code_GE) {
    Mword gexccode = ((read_c0_guestctl0() & GUESTCTL0_GEXC) >> GUESTCTL0_GEXC_SHIFT);
    switch (gexccode) {
      case GUESTCTL0_GEXC_GPSI:
      case GUESTCTL0_GEXC_GSFC:
      case GUESTCTL0_GEXC_HC:
      case GUESTCTL0_GEXC_GRR:
        bad_instr_valid = 1;
        break;
      case GUESTCTL0_GEXC_GVA:
      case GUESTCTL0_GEXC_GHFC:
      case GUESTCTL0_GEXC_GPA:
      default:
        bad_instr_valid = 0;
        break;
    }
  }

  if (bad_instr_valid)
    return read_c0_badinstr();
  else
    return 0;
}

IMPLEMENT inline
bool
Thread::pagein_tcb_request(Return_frame * /*regs*/)
{
  NOT_IMPL_PANIC;
  return false;
}

extern "C" FIASCO_FASTCALL
void
timer_handler()
{
  Return_frame *rf = nonull_static_cast<Return_frame*>(current()->regs());
  //disable power savings mode, when we come from privileged mode
  if(EXPECT_FALSE(rf->user_mode()))
    rf->status = Proc::wake(rf->status);

  Timer::update_system_clock(current_cpu());
  current_thread()->handle_timer_interrupt();
}

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

/** Constructor.
    @param id user-visible thread ID of the sender
    @param init_prio initial priority
    @param mcp thread's maximum controlled priority
    @post state() != 0
 */
IMPLEMENT
Thread::Thread()
  : Sender            (0),	// select optimized version of constructor
    _pager(Thread_ptr::Invalid),
    _exc_handler(Thread_ptr::Invalid),
    _del_observer(0)
{

  assert(state(false) == 0);

  inc_ref();
  _space.space(Kernel_task::kernel_task());

  if (Config::Stack_depth)
    std::memset((char*)this + sizeof(Thread), '5',
                Thread::Size - sizeof(Thread) - 64);

  // set a magic value -- we use it later to verify the stack hasn't
  // been overrun
  _magic = magic;
  _recover_jmpbuf = 0;
  _timeout = 0;

  *reinterpret_cast<void(**)()> (--_kernel_sp) = user_invoke;

  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Entry_frame *r = regs();
  r->sp(0);
  r->ip(0);

  state_add_dirty(Thread_dead, false);
  // ok, we're ready to go!
}

IMPLEMENT inline
Mword
Thread::user_sp() const
{ return regs()->sp(); }

IMPLEMENT inline
void
Thread::user_sp(Mword sp)
{ return regs()->sp(sp); }

IMPLEMENT inline NEEDS[Thread::exception_triggered]
Mword
Thread::user_ip() const
{ return exception_triggered() ? _exc_cont.ip() : regs()->ip(); }

IMPLEMENT inline
Mword
Thread::user_flags() const
{ return 0; }

IMPLEMENT inline NEEDS[Thread::exception_triggered]
void
Thread::user_ip(Mword ip)
{
  if (exception_triggered())
    _exc_cont.ip(ip);
  else
    {
      Entry_frame *r = regs();
      r->ip(ip);
    }
}

PUBLIC inline NEEDS ["trap_state.h"]
int
Thread::send_exception_arch(Trap_state * /*ts*/)
{
  // Kyma: send_exception_arch nothing to do for mips
  return 1; // We did it
}

// ------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mips_vz]:

#include "vm.h"

IMPLEMENT inline
bool
Thread::arch_ext_vcpu_enabled()
{ return true; }

IMPLEMENT
void
Thread::arch_init_vcpu_state(Vcpu_state *vcpu_state, bool ext)
{
  if (!ext || (state() & Thread_ext_vcpu_enabled))
    return;

  Vm::init_vm_state(vcpu_state);
}

// ------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

PROTECTED inline
void
Thread::vcpu_resume_user_arch()
{
  // Kyma: vcpu_resume_user_arch nothing to do for mips
}


PRIVATE static inline
void
Thread::save_fpu_state_to_utcb(Trap_state *, Utcb *)
{}

PROTECTED inline
int
Thread::do_trigger_exception(Entry_frame *r, void *ret_handler)
{
  if (!exception_triggered())
    {
      // Kyma: not implemented, catch call to leave_by_trigger_exception
      extern char leave_by_trigger_exception[];
      if(ret_handler == leave_by_trigger_exception)
        NOT_IMPL;

      _exc_cont.activate(r, ret_handler);
      return 1;
    }
  // else ignore change of IP because triggered exception already pending
  return 0;
}

PRIVATE static inline
bool FIASCO_WARN_RESULT
Thread::copy_utcb_to_ts(L4_msg_tag const &tag, Thread *snd, Thread *rcv,
                        L4_fpage::Rights rights)
{
  Trap_state *ts = (Trap_state*)rcv->_utcb_handler;
  Utcb *snd_utcb = snd->utcb().access();
  Mword       s  = tag.words();

  if (EXPECT_FALSE(rcv->exception_triggered()))
    {
      // triggered exception pending
      Mem::memcpy_mwords (ts, snd_utcb->values, s > Trap_state::TS_REG_GPR_END ? (Mword)Trap_state::TS_REG_GPR_END : s);
      if (EXPECT_TRUE(s > Trap_state::TS_REG_SR))
	{
	  // sanitize processor mode
          // XXX: fix race (Kyma: TODO copied from thread-arm.cpp, do we have this issue?)
	  snd_utcb->values[Trap_state::TS_REG_SR] &= ~Cp0_Status::ST_KSU_MASK; // clear mode
	  snd_utcb->values[Trap_state::TS_REG_SR] |= Cp0_Status::ST_IE; // enable irqs

	  Continuation::User_return_frame const *s
	    = reinterpret_cast<Continuation::User_return_frame const *>((char*)&snd_utcb->values[Trap_state::TS_REG_GPR_END]);

	  rcv->_exc_cont.set(ts, s);
	}
    }
  else
    {
      Mem::memcpy_mwords (ts, snd_utcb->values, s > Trap_state::TS_REG_EPC ? (Mword)Trap_state::TS_REG_EPC : s);
      if (EXPECT_TRUE(s > Trap_state::TS_REG_EPC))
	ts->epc = snd_utcb->values[Trap_state::TS_REG_EPC];
      if (EXPECT_TRUE(s > Trap_state::TS_REG_SR))
	{
	  // sanitize processor mode
	  Mword p = snd_utcb->values[Trap_state::TS_REG_SR];
	  p &= ~(Cp0_Status::ST_KSU_MASK | Cp0_Status::ST_IE); // clear mode & irq
	  p |=  Cp0_Status::ST_KSU_USER;
	  ts->status = p;
	}
    }

  if (tag.transfer_fpu() && (rights & L4_fpage::Rights::W()))
    snd->transfer_fpu(rcv);

  bool ret = transfer_msg_items(tag, snd, snd_utcb,
                                rcv, rcv->utcb().access(), rights);

  rcv->state_del(Thread_in_exception);
  return ret;
}

PRIVATE static inline NEEDS[Thread::save_fpu_state_to_utcb, "trap_state.h"]
bool FIASCO_WARN_RESULT
Thread::copy_ts_to_utcb(L4_msg_tag const &, Thread *snd, Thread *rcv,
                        L4_fpage::Rights rights)
{
  Trap_state *ts = (Trap_state*)snd->_utcb_handler;

  {
    auto guard = lock_guard(cpu_lock);
    Utcb *rcv_utcb = rcv->utcb().access();

    // Trap_state_regs + Syscall_frame: pfa, err, r[32]
    Mem::memcpy_mwords(rcv_utcb->values, ts, Trap_state::TS_REG_GPR_END);
    Continuation::User_return_frame *d
      = reinterpret_cast<Continuation::User_return_frame *>((char*)&rcv_utcb->values[Trap_state::TS_REG_GPR_END]);

    snd->_exc_cont.get(d, ts);


    if (EXPECT_TRUE(!snd->exception_triggered()))
      {
        rcv_utcb->values[Trap_state::TS_REG_EPC] = ts->epc;
        rcv_utcb->values[Trap_state::TS_REG_SR] = ts->status;
      }

    if (rcv_utcb->inherit_fpu() && (rights & L4_fpage::Rights::W()))
      {
        snd->save_fpu_state_to_utcb(ts, rcv_utcb);
        snd->transfer_fpu(rcv);
      }
  }
  return true;
}

PROTECTED inline
L4_msg_tag
Thread::invoke_arch(L4_msg_tag /*tag*/, Utcb * /*utcb*/)
{
  return commit_result(-L4_err::ENosys);
}

PROTECTED inline
int
Thread::sys_control_arch(Utcb *)
{
  return 0;
}

//-----------------------------------------------------------------------------
INTERFACE [mips32]:

#include "irq_chip.h"

class Thread_glbl_remote_rq_irq : public Irq_base {};
class Thread_remote_rq_irq : public Irq_base {};
class Thread_debug_ipi : public Irq_base {};

class Thread_ipi
{
public:
  static void init(Cpu_number cpu);
};

IMPLEMENTATION [mips32]:

IMPLEMENT_DEFAULT inline
void
Thread_ipi::init(Cpu_number)
{}

//-----------------------------------------------------------------------------
INTERFACE [mips32 && mp]:

#include "ipi.h"

//-----------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mp]:

#include "ipi.h"
#include "irq_mgr.h"
#include "kdb_ke.h"

EXTENSION class Thread
{
public:
  static void kern_kdebug_ipi_entry() { kdb_ke_ipi(); }
};

EXTENSION class Thread_glbl_remote_rq_irq
{
public:
  void handle(Upstream_irq const *)
  { Thread::handle_global_remote_requests_irq(); }

  Thread_glbl_remote_rq_irq()
  { set_hit(&handler_wrapper<Thread_glbl_remote_rq_irq>); }

  void switch_mode(bool) {}
};

EXTENSION class Thread_remote_rq_irq
{
public:
  void handle(Upstream_irq const *)
  { Thread::handle_remote_requests_irq(); }

  Thread_remote_rq_irq()
  { set_hit(&handler_wrapper<Thread_remote_rq_irq>); }

  void switch_mode(bool) {}
};

EXTENSION class Thread_debug_ipi
{
public:
  void handle(Upstream_irq const *)
  {
    Ipi::eoi(Ipi::Debug, current_cpu());
    Thread::kern_kdebug_ipi_entry();
  }

  Thread_debug_ipi()
  { set_hit(&handler_wrapper<Thread_debug_ipi>); }

  void switch_mode(bool) {}
};

EXTENSION class Thread_ipi
{
  Thread_glbl_remote_rq_irq glbl_remote_rq_ipi;
  Thread_remote_rq_irq remote_rq_ipi;
  Thread_debug_ipi debug_ipi;
};

#include "pic.h"

IMPLEMENT
void
Thread_ipi::init(Cpu_number cpu)
{
  if (!Pic::gic_present())
    return;

  if (cpu == Cpu_number::boot_cpu())
    thread_ipi.construct();

  thread_ipi->glbl_remote_rq_ipi.unmask();
  check(Irq_mgr::mgr->alloc(&thread_ipi->glbl_remote_rq_ipi,
        Ipi::irq_xlate(Ipi::Global_request, cpu)));

  thread_ipi->remote_rq_ipi.unmask();
  check(Irq_mgr::mgr->alloc(&thread_ipi->remote_rq_ipi,
        Ipi::irq_xlate(Ipi::Request, cpu)));

  thread_ipi->debug_ipi.unmask();
  check(Irq_mgr::mgr->alloc(&thread_ipi->debug_ipi,
        Ipi::irq_xlate(Ipi::Debug, cpu)));
}

Static_object<Thread_ipi> thread_ipi;

//-----------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

PUBLIC static
bool
Thread::handle_fpe(Trap_state *ts)
{
  Mword epc = ts->epc;

  if (ts->cause & CAUSEF_BD)
    epc += 4;

  panic("FPU emulation not supported @ %08lx", epc);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [mips32 && !fpu]:

PUBLIC static
bool
Thread::handle_fpu_trap(Trap_state *ts)
{
  Mword epc = ts->epc;

  if (ts->cause & CAUSEF_BD)
    epc += 4;

  panic("FPU processor not supported @ %08lx", epc);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [mips32 && fpu]:

#include "fpu.h"
#include "cpu.h"

PUBLIC static
bool
Thread::handle_fpu_trap(Trap_state *)
{
  assert (!Fpu::is_enabled());

  if (Fpu::fpu_debug())
    {
      Thread *t = current_thread();
      printf("###FPU own %p => cur %p %s\n", Fpu::fpu.current().owner(), t, Cpu::vz_str());
    }

  // don't update_pc, retry the faulting instruction
  if (current_thread()->switchin_fpu())
    return true;

  printf("handle_fpu_trap: switchin_fpu failed\n");
  return false;
}
