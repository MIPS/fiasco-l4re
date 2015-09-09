/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips_vz]:

#include "kmem_slab.h"
#include "task.h"
#include "trap_state.h"
#include "thread.h"

class Vm : public Task
{
public:

  enum l4_vcpu_consts_mips
  {
    L4_VM_STATE_VERSION = 1,
    L4_VCPU_OFFSET_VM_STATE = 0x400, // synch with l4/pkg/l4sys/include/vcpu.h and mips32/exception.S
    N_MIPS_COPROC_REGS = 32,
    N_MIPS_COPROC_SEL  = 8
  };

  enum Exit_reasons
  {
    ER_undef      = 0,
    ER_none       = 1,
    ER_vmm_exit   = 2,
    ER_irq        = 3,
    ER_tlb        = 4,
    ER_ipc        = 5,
    ER_max        = 6,
  };

  enum Cp0_regs
  {
    MIPS_CP0_TLB_INDEX      = 0,
    MIPS_CP0_TLB_RANDOM     = 1,
    MIPS_CP0_TLB_LOW        = 2,
    MIPS_CP0_TLB_LO0        = 2,
    MIPS_CP0_TLB_LO1        = 3,
    MIPS_CP0_TLB_CONTEXT    = 4,
    MIPS_CP0_TLB_PG_MASK    = 5,
    MIPS_CP0_TLB_WIRED      = 6,
    MIPS_CP0_HWRENA         = 7,
    MIPS_CP0_BAD_VADDR      = 8,
    MIPS_CP0_COUNT          = 9,
    MIPS_CP0_TLB_HI         = 10,
    MIPS_CP0_COMPARE        = 11,
    MIPS_CP0_STATUS         = 12,
    MIPS_CP0_CAUSE          = 13,
    MIPS_CP0_EXC_PC         = 14,
    MIPS_CP0_PRID           = 15,
    MIPS_CP0_CONFIG         = 16,
    MIPS_CP0_LLADDR         = 17,
    MIPS_CP0_WATCH_LO       = 18,
    MIPS_CP0_WATCH_HI       = 19,
    MIPS_CP0_TLB_XCONTEXT   = 20,
    MIPS_CP0_ECC            = 26,
    MIPS_CP0_CACHE_ERR      = 27,
    MIPS_CP0_TAG_LO         = 28,
    MIPS_CP0_TAG_HI         = 29,
    MIPS_CP0_ERROR_PC       = 30,
    MIPS_CP0_DEBUG          = 23,
    MIPS_CP0_DEPC           = 24,
    MIPS_CP0_PERFCNT        = 25,
    MIPS_CP0_ERRCTL         = 26,
    MIPS_CP0_DATA_LO        = 28,
    MIPS_CP0_DATA_HI        = 29,
    MIPS_CP0_DESAVE         = 31,

    MIPS_CP0_CONFIG_SEL     = 0,
    MIPS_CP0_CONFIG1_SEL    = 1,
    MIPS_CP0_CONFIG2_SEL    = 2,
    MIPS_CP0_CONFIG3_SEL    = 3,

    MIPS_CP0_INTCTL         = 12,
    MIPS_CP0_INTCTL_SEL     = 1,
    MIPS_CP0_SRSCTL         = 12,
    MIPS_CP0_SRSCTL_SEL     = 2,
    MIPS_CP0_SRSMAP         = 12,
    MIPS_CP0_SRSMAP_SEL     = 3,

    MIPS_CP0_GTOFFSET           = 12,
    MIPS_CP0_GTOFFSET_SEL       = 7,

    MIPS_CP0_GUESTCTL0          = 12,
    MIPS_CP0_GUESTCTL0_SEL      = 6,

    MIPS_CP0_GUESTCTL0EXT       = 11,
    MIPS_CP0_GUESTCTL0EXT_SEL   = 4,

    MIPS_CP0_GUESTCTL1          = 10,
    MIPS_CP0_GUESTCTL1_SEL      = 4,

    MIPS_CP0_GUESTCTL2          = 10,
    MIPS_CP0_GUESTCTL2_SEL      = 5,

    MIPS_CP0_USERLOCAL          = 4,
    MIPS_CP0_USERLOCAL_SEL      = 2,

    MIPS_CP0_PRID_SEL           = 0,
  };

  // Kyma: synch with l4_vm_vz_state in l4/pkg/l4sys/include/ARCH-mips/vm.h
  // bump L4_VM_STATE_VERSION in both files when changing Vm_state
  struct Vm_state
  {
    Trap_state *ts;

    Mword version;        // initialized by kernel to L4_VM_STATE_VERSION
    Mword sizeof_vmstate; // initialized by kernel to sizeof(Vm_state)

    enum Exit_reasons exit_reason;

    Mword exit_gexccode;  // contains GuestCtl0 GExcCode field
    // It is left to the VMM to determine if badinstr contents are valid for a given exception
    Mword exit_badinstr;  // contains faulting instruction
    Mword exit_badinstrp; // contains prior branch instr when faulting instr is in BD slot
    Mword host_cp0_gtoffset;
    Mword host_cp0_guestctl0;
    Mword host_cp0_guestctl0ext;
    Mword host_cp0_guestctl1;
    Mword host_cp0_guestctl2;

    Unsigned8 set_cp0_guestctl0;
    Unsigned8 set_cp0_guestctl0ext;

    Unsigned8 set_injected_ipx;
    Mword injected_ipx;

    Mword gcp0[N_MIPS_COPROC_REGS][N_MIPS_COPROC_SEL];
  };

private:
  Vm_state *state_for_dbg;
  static Vm_state vmstate_at_reset;
};

struct Vm_allocator
{
  static Kmem_slab_t<Vm> a;
};

//-----------------------------------------------------------------------------
INTERFACE [debug && mips_vz]:

#include "tb_entry.h"

EXTENSION class Vm
{
public:
  struct Vm_log : public Tb_entry
  {
    bool is_entry;
    Mword pc;
    Mword sr;
    Mword exit_reason;
    Mword exit_exccode;
    Mword exit_gexccode;
    void print(String_buffer *buf) const;
 };
};

//-----------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mips_vz]:

#include "cpu.h"
#include "cpu_lock.h"
#include "entry_frame.h"
#include "ipc_timeout.h"
#include "logdefs.h"
#include "mem_space.h"
#include "thread_state.h"
#include "timer.h"

Kmem_slab_t<Vm> Vm_allocator::a("Vm");

Vm::Vm_state Vm::vmstate_at_reset;

PUBLIC explicit
Vm::Vm(Ram_quota *q) : Task(q, Caps::mem() | Caps::obj())
{
  Mem_space::is_vmspace(true);
}

PUBLIC inline
Page_number
Vm::mem_space_map_max_address() const
{ return Page_number(1UL << (MWORD_BITS - Mem_space::Page_shift)); }

PUBLIC inline virtual
Page_number
Vm::map_max_address() const
{ return Page_number(1UL << (MWORD_BITS - Mem_space::Page_shift)); }

PUBLIC static
Slab_cache *
Vm::allocator()
{ return &Vm_allocator::a; }

PUBLIC inline
void *
Vm::operator new(size_t size, void *p) throw()
{
  (void)size;
  assert (size == sizeof(Vm));
  return p;
}

PUBLIC
void
Vm::operator delete(void *ptr)
{
  Vm *t = reinterpret_cast<Vm*>(ptr);
  allocator()->q_free(t->ram_quota(), ptr);
}

// ------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mips_vz]:

#include "warn.h"
#include "thread.h"
#include "vm.h"
#include "cpu.h"

/*
 * Restore Guest.Count, Guest.Compare and Guest.Cause taking care to
 * preserve the value of Guest.Cause[TI] while restoring Guest.Cause.
 *
 * Based on the algorithm in VZ ASE specification v1.04 - Section: Guest Timer,
 * with additional changes to account for elapsed time while a guest was
 * preempted.
 */
Mword ti_pending_count;

PRIVATE
void
Vm::vz_restore_guest_timer_int(Vm_state *vm_state)
{
  Mword current_guest_count;
  Mword saved_guest_cause = vm_state->gcp0[MIPS_CP0_CAUSE][0];
  Mword saved_guest_count = vm_state->gcp0[MIPS_CP0_COUNT][0];
  Mword saved_guest_compare = vm_state->gcp0[MIPS_CP0_COMPARE][0];

  if (saved_guest_cause & CAUSEF_TI) {
    ti_pending_count++;
    write_c0_gtoffset(vm_state->host_cp0_gtoffset);
    /* XXXKYMA: Writing compare will clear Guest.Cause.TI */
    write_c0_guest_compare(saved_guest_compare);
    write_c0_guest_cause(saved_guest_cause);
    return;
  }

  /* Adjust GTOffset for the time guest was preempted */
  write_c0_gtoffset(vm_state->host_cp0_gtoffset);

  /* XXXKYMA: Writing compare will clear Guest.Cause.TI */
  write_c0_guest_compare(saved_guest_compare);

  /* Restore Guest.Cause since Guest.Cause.TI may be 1. Guest.Cause must
   * be saved after Guest.Count to provide most current Cause.TI. */
  write_c0_guest_cause(saved_guest_cause);
  current_guest_count = read_c0_guest_count();

  /* set Guest.Cause[TI] if it would have been set while the guest was
   * sleeping.  Since GTOffset for the guest and Guest.Compare restore is
   * not atomic, this code is required to ensure that Guest.Cause.TI is
   * set appropriately, since current Guest.Count could have raced ahead
   * of saved_count before restoring Guest.Compare.
   */
  if (current_guest_count > saved_guest_count) {
    if ((saved_guest_compare > saved_guest_count)
        && (saved_guest_compare < current_guest_count)) {
      saved_guest_cause |= CAUSEF_TI;
      write_c0_guest_cause(saved_guest_cause);
    }
  } else {
    /* The count has wrapped. Check to see if guest count has
     * passed the saved compare value */
    if ((saved_guest_compare > saved_guest_count)
        || (saved_guest_compare < current_guest_count)) {
      saved_guest_cause |= CAUSEF_TI;
      write_c0_guest_cause(saved_guest_cause);
    }
  }
}

PRIVATE
int
Vm::install_guest_cp0_regs(Vm_state *vm_state)
{
  /* some registers are not restored
   * prid                 : emulation-only
   * random, count        : read-only
   * config6              : not implemented in processor variant
   * compare, cause       : defer to vz_restore_guest_timer_int
   */

  write_c0_guest_index(vm_state->gcp0[MIPS_CP0_TLB_INDEX][0]);
  write_c0_guest_entrylo0(vm_state->gcp0[MIPS_CP0_TLB_LO0][0]);
  write_c0_guest_entrylo1(vm_state->gcp0[MIPS_CP0_TLB_LO1][0]);
  write_c0_guest_context(vm_state->gcp0[MIPS_CP0_TLB_CONTEXT][0]);
  write_c0_guest_pagemask(vm_state->gcp0[MIPS_CP0_TLB_PG_MASK][0]);
  write_c0_guest_pagegrain(vm_state->gcp0[MIPS_CP0_TLB_PG_MASK][1]);
  write_c0_guest_wired(vm_state->gcp0[MIPS_CP0_TLB_WIRED][0]);
  write_c0_guest_hwrena(vm_state->gcp0[MIPS_CP0_HWRENA][0]);
  write_c0_guest_badvaddr(vm_state->gcp0[MIPS_CP0_BAD_VADDR][0]);
  /* skip write_c0_guest_count */
  write_c0_guest_entryhi(vm_state->gcp0[MIPS_CP0_TLB_HI][0]);
  /* defer write_c0_guest_compare */
  write_c0_guest_status(vm_state->gcp0[MIPS_CP0_STATUS][0]);
  write_c0_guest_intctl(vm_state->gcp0[MIPS_CP0_INTCTL][MIPS_CP0_INTCTL_SEL]);
  /* defer write_c0_guest_cause */
  write_c0_guest_epc(vm_state->gcp0[MIPS_CP0_EXC_PC][0]);
  /* skip prid, emulation only */
  write_c0_guest_ebase(vm_state->gcp0[MIPS_CP0_PRID][1]);

  /* only restore implemented config registers */
  write_c0_guest_config(vm_state->gcp0[MIPS_CP0_CONFIG][0]);

  if ((vm_state->gcp0[MIPS_CP0_CONFIG][0] & MIPS_CONF_M) && cpu_vz_has_config1)
    write_c0_guest_config1(vm_state->gcp0[MIPS_CP0_CONFIG][1]);

  if ((vm_state->gcp0[MIPS_CP0_CONFIG][1] & MIPS_CONF_M) && cpu_vz_has_config2)
    write_c0_guest_config2(vm_state->gcp0[MIPS_CP0_CONFIG][2]);

  if ((vm_state->gcp0[MIPS_CP0_CONFIG][2] & MIPS_CONF_M) && cpu_vz_has_config3)
  {
    write_c0_guest_config3(vm_state->gcp0[MIPS_CP0_CONFIG][3]);

    /* restore userlocal */
    if (vm_state->gcp0[MIPS_CP0_CONFIG][3] & MIPS_CONF3_ULRI)
      write_c0_guest_userlocal(vm_state->gcp0[MIPS_CP0_USERLOCAL][MIPS_CP0_USERLOCAL_SEL]);
  }

  if ((vm_state->gcp0[MIPS_CP0_CONFIG][3] & MIPS_CONF_M) && cpu_vz_has_config4)
    write_c0_guest_config4(vm_state->gcp0[MIPS_CP0_CONFIG][4]);

  if ((vm_state->gcp0[MIPS_CP0_CONFIG][4] & MIPS_CONF_M) && cpu_vz_has_config5)
    write_c0_guest_config5(vm_state->gcp0[MIPS_CP0_CONFIG][5]);

  if (cpu_vz_has_config6)
    write_c0_guest_config6(vm_state->gcp0[MIPS_CP0_CONFIG][6]);
  if (cpu_vz_has_config7)
    write_c0_guest_config7(vm_state->gcp0[MIPS_CP0_CONFIG][7]);

  write_c0_guest_errorepc(vm_state->gcp0[MIPS_CP0_ERROR_PC][0]);

  sanitize_guest_cp0_regs(vm_state);

  /* call after setting MIPS_CP0_CAUSE to avoid having it overwritten
   * this will set guest compare and cause.TI if necessary
   */
  vz_restore_guest_timer_int(vm_state);

  return 0;
}

PRIVATE static
int
Vm::readout_guest_cp0_regs(Vm_state *vm_state)
{
  vm_state->gcp0[MIPS_CP0_TLB_INDEX][0] = read_c0_guest_index();
  vm_state->gcp0[MIPS_CP0_TLB_LO0][0] = read_c0_guest_entrylo0();
  vm_state->gcp0[MIPS_CP0_TLB_LO1][0] = read_c0_guest_entrylo1();
  vm_state->gcp0[MIPS_CP0_TLB_CONTEXT][0] = read_c0_guest_context();
  vm_state->gcp0[MIPS_CP0_TLB_PG_MASK][0] = read_c0_guest_pagemask();
  vm_state->gcp0[MIPS_CP0_TLB_PG_MASK][1] = read_c0_guest_pagegrain();
  vm_state->gcp0[MIPS_CP0_TLB_WIRED][0] = read_c0_guest_wired();
  vm_state->gcp0[MIPS_CP0_HWRENA][0] = read_c0_guest_hwrena();
  vm_state->gcp0[MIPS_CP0_BAD_VADDR][0] = read_c0_guest_badvaddr();
  vm_state->gcp0[MIPS_CP0_TLB_HI][0] = read_c0_guest_entryhi();
  vm_state->gcp0[MIPS_CP0_STATUS][0] = read_c0_guest_status();
  vm_state->gcp0[MIPS_CP0_INTCTL][MIPS_CP0_INTCTL_SEL] = read_c0_guest_intctl();
  vm_state->gcp0[MIPS_CP0_SRSCTL][MIPS_CP0_SRSCTL_SEL] = 0;
  vm_state->gcp0[MIPS_CP0_SRSMAP][MIPS_CP0_SRSMAP_SEL] = 0;
  vm_state->gcp0[MIPS_CP0_EXC_PC][0] = read_c0_guest_epc();
  /* skip prid, emulation only */
  vm_state->gcp0[MIPS_CP0_PRID][1] = read_c0_guest_ebase();

  vm_state->gcp0[MIPS_CP0_CONFIG][0] = read_c0_guest_config();

  /* only save implemented config registers */
  vm_state->gcp0[MIPS_CP0_CONFIG][1] =
    ((vm_state->gcp0[MIPS_CP0_CONFIG][0] & MIPS_CONF_M) &&
    cpu_vz_has_config1) ?  read_c0_guest_config1() : 0;
  vm_state->gcp0[MIPS_CP0_CONFIG][2] =
    ((vm_state->gcp0[MIPS_CP0_CONFIG][1] & MIPS_CONF_M) &&
    cpu_vz_has_config2) ?  read_c0_guest_config2() : 0;
  vm_state->gcp0[MIPS_CP0_CONFIG][3] =
    ((vm_state->gcp0[MIPS_CP0_CONFIG][2] & MIPS_CONF_M) &&
    cpu_vz_has_config3) ?  read_c0_guest_config3() : 0;
  vm_state->gcp0[MIPS_CP0_CONFIG][4] =
    ((vm_state->gcp0[MIPS_CP0_CONFIG][3] & MIPS_CONF_M) &&
    cpu_vz_has_config4) ?  read_c0_guest_config4() : 0;
  vm_state->gcp0[MIPS_CP0_CONFIG][5] =
    ((vm_state->gcp0[MIPS_CP0_CONFIG][4] & MIPS_CONF_M) &&
    cpu_vz_has_config5) ?  read_c0_guest_config5() : 0;

  vm_state->gcp0[MIPS_CP0_CONFIG][6] = cpu_vz_has_config6 ?
    read_c0_guest_config6() : 0;
  vm_state->gcp0[MIPS_CP0_CONFIG][7] = cpu_vz_has_config7 ?
    read_c0_guest_config7() : 0;

  /* save userlocal */
  vm_state->gcp0[MIPS_CP0_USERLOCAL][MIPS_CP0_USERLOCAL_SEL] =
    ((vm_state->gcp0[MIPS_CP0_CONFIG][3] & MIPS_CONF3_ULRI) &&
    cpu_vz_has_config3) ?  read_c0_guest_userlocal() : 0;

  vm_state->gcp0[MIPS_CP0_ERROR_PC][0] = read_c0_guest_errorepc();

  /* XXXKYMA read these out last, so that we have the most up to date
   * values for Guest.Compare/Guest.Cause.TI
   */
  vm_state->gcp0[MIPS_CP0_COMPARE][0] = read_c0_guest_compare();
  vm_state->gcp0[MIPS_CP0_COUNT][0] = read_c0_guest_count();
  vm_state->gcp0[MIPS_CP0_CAUSE][0] = read_c0_guest_cause();

  sanitize_guest_cp0_regs(vm_state);

  return 0;
}

PRIVATE static
int
Vm::sanitize_guest_cp0_regs(Vm_state *vm_state)
{
  // no Vm cop0 MAAR or MAARI support, however Guest.Config5.MRP is only
  // optionally root-writable so guest may still see it as MRP=1.
  vm_state->gcp0[MIPS_CP0_CONFIG][5] &= ~MIPS_CONF5_MRP;

  return 0;
}

PRIVATE
void
Vm::inject_guest_interrupts(Vm_state *vm_state, bool readout_regs)
{
  // writing guestctl2.VIP causes Guest.Status.IP to be modified.  call
  // inject_guest_interrupts after install_guest_cp0_regs or else the
  // injected interrupt could get overwritten.
  if (vm_state->set_injected_ipx && cpu_has_guestctl2)
    {
      write_c0_guestctl2(vm_state->injected_ipx & GUESTCTL2_VIP);
      vm_state->set_injected_ipx = 0;

      // save the injected interrupt state so it can later be restored;
      // only required if not going to enter vz guest directly aftewards.
      // i.e. guaranteed not to be switched to another VM guest
      if (readout_regs)
        {
          vm_state->gcp0[MIPS_CP0_STATUS][0] = read_c0_guest_status();
          vm_state->host_cp0_guestctl2 = read_c0_guestctl2();
        }
    }
}

PRIVATE static
void
Vm::readout_host_cp0(Vm_state *vm_state)
{
  // NOTE: these host registers are generally provided to the VMM as read-only.

  vm_state->host_cp0_gtoffset = read_c0_gtoffset();
  vm_state->host_cp0_guestctl0 = read_c0_guestctl0();
  vm_state->host_cp0_guestctl0ext = cpu_has_guestctl0ext ? read_c0_guestctl0ext() : 0;
  vm_state->host_cp0_guestctl1 = cpu_has_guestctl1 ? read_c0_guestctl1() : 0;
  vm_state->host_cp0_guestctl2 = cpu_has_guestctl2 ? read_c0_guestctl2() : 0;

  /* badinstr and badinstrp are only valid for exceptions which are synchronous
   * to an instruction: Coprocessor Unusable, Reserved Instruction, Integer
   * Overflow, Trap, System Call, Breakpoint, Floating Point, Coprocessor 2
   * exception, Address Error, TLB Refill, TLB Invalid, TLB Read Inhibit, TLB
   * Execute Inhibit, TLB Modified
   */

  /* Save badinstr before another exception can overwrite it. It is left to the
   * VMM to determine if the contents are valid for the particular exception */
  vm_state->exit_badinstr = cpu_has_badinstr ? read_c0_badinstr() : 0;
  vm_state->exit_badinstrp = cpu_has_badinstrp ? read_c0_badinstrp() : 0;
}

PRIVATE static
void
Vm::apply_host_cp0_changes(Vm_state *vm_state)
{
  // NOTE: Unlike install_guest_cp0_regs, install_host_cp0 is not implemented.
  // VMM changes to the vm_state host_cp0 registers are selectively applied;
  // this is partially a security issue and partly because some host_cp0
  // registers are handled elsewhere:
  //
  //   gtoffset - is handled within vz_restore_guest_timer_int()
  //   guestctl1 - in swithin_context()
  //   guestctl2 - in inject_guest_interrupts()

  // Kyma: consider sanitizing these VMM changes to the vm_state
  if (vm_state->set_cp0_guestctl0)
    write_c0_guestctl0(vm_state->host_cp0_guestctl0);
  if (vm_state->set_cp0_guestctl0ext && cpu_has_guestctl0ext)
    write_c0_guestctl0ext(vm_state->host_cp0_guestctl0ext);
}

/*
 * do_resume_vcpu - enter and exit VZ guest mode
 *
 * On exit:
 * set vm_state->exit_reason to ER_irq, ER_xxx
 * return 1 for IRQ exit
 *        0 for other exits
 *        -L4_err::EInval for errors
 */
PRIVATE inline NOEXPORT
int
Vm::do_resume_vcpu(Context *ctxt, Vcpu_state *vcpu, Vm_state *vm_state)
{
  Trap_state* ts = vm_state->ts;

  Mword exc_code;
  int handled;

  assert(cpu_lock.test());

  /*
   * call switchin_context to set guest asid and vzguestid.
   * set user_mode so that vcpu_aware_space in handle_tlb_exceptions walks the
   * page table for the correct mem_space.
   */
  ctxt->space_ref()->user_mode(true);
  switchin_context(ctxt->space());

  // Kyma: TODO save/restore guest TLB if required

  // Kyma: optimization consider installing regs less frequently, perhaps only
  // on context switch?
  install_guest_cp0_regs(vm_state);
  apply_host_cp0_changes(vm_state);

  // call inject_guest_interrupts after install_guest_cp0_regs or else the
  // injected interrupt could get overwritten. No need to readout regs, we are
  // going right into resume_vm_vz, they will be readout after next guest exit.
  inject_guest_interrupts(vm_state, false);

  /*
   * Set EXL bit to stay in root context when GM is set;
   * Set user mode while in guest mode because various tlb checks depend on it;
   * Enable timer and any other interrupts
   */
  ts->status = Cp0_Status::status_eret_to_guest_ei(0);

  /* enter vz guest */
  {
    extern char resume_vm_vz[];
    asm volatile
      ("  move  $a0, %[vcpu]      \n"
       "  move  $a1, %[vm_state]  \n"
       "  jal   %[rfe]            \n"
       "  nop                     \n"
       : : [vcpu] "r" (vcpu), [vm_state] "r" (vm_state),
       [rfe] "r" (&resume_vm_vz)
       : "k1", "a0", "a1", "ra"
      );
  }
  /* exit vz guest */

  assert(cpu_lock.test());

  readout_guest_cp0_regs(vm_state);
  readout_host_cp0(vm_state);

  /* update vm state exit reason */

  exc_code = MIPS_TRAPTYPE(ts->cause);
  ts->set_errorcode(exc_code);

  handled = 0;
  switch(exc_code) {
    case Trap_state::Exc_code_Int:
      vm_state->exit_reason = ER_irq;
      break;
    case Trap_state::Exc_code_Mod:
    case Trap_state::Exc_code_TLBL:
    case Trap_state::Exc_code_TLBS:
      vm_state->exit_reason = ER_tlb;
      /* needs to be called while guest mem_space is still current */
      handled = handle_tlb_exceptions(ts, cxx::int_value<Cpu_number>(current_cpu()), true /*guest*/);
      Proc::cli();
      if (handled)
        vm_state->exit_reason = ER_none;
      break;
    case Trap_state::Exc_code_GE:
      vm_state->exit_reason = ER_vmm_exit;
      vm_state->exit_gexccode = ((read_c0_guestctl0() & GUESTCTL0_GEXC) >> GUESTCTL0_GEXC_SHIFT);
      break;
    default:
      vm_state->exit_reason = ER_vmm_exit;
      break;
  }

  // handle L4_VCPU_F_EXCEPTIONS and exception reflection for use with libvcpu
  // (not used by karma; refer to l4re examples/sys/vcpu-vz)
  if (current_thread()->vcpu_exceptions_enabled(vcpu) && (vm_state->exit_reason == ER_vmm_exit)) {
    handled = current_thread()->send_exception(ts);
    if (handled)
      vm_state->exit_reason = ER_none;
  }

  /* call switchin_context to set host asid and clear vzguestid */
  ctxt->space_ref()->user_mode(false);
  ctxt->space()->switchin_context(this);

  if (vm_state->exit_reason == ER_irq)
    return 1;

  // Kyma: if we are going to clear Vcpu_state::F_user_mode here then the VM
  // needs to set it each time it does vm_resume (as karma does).
  if (!handled)
    vcpu->state &= ~(Vcpu_state::F_traps | Vcpu_state::F_user_mode);
  return 0;
}

PUBLIC
int
Vm::resume_vcpu(Context *ctxt, Vcpu_state *vcpu, bool user_mode)
{
  (void)user_mode;
  assert_kdb(user_mode);
  assert(cpu_lock.test());

  if (!cpu_has_vz)
    return -L4_err::EInval;

  if (!(ctxt->state(true) & Thread_ext_vcpu_enabled))
    {
      ctxt->arch_load_vcpu_kern_state(vcpu, true);
      return -L4_err::EInval;
    }

  Vm_state *vm_state = get_vm_state(vcpu);

  state_for_dbg = vm_state;

  int err = 0;
  for (;;)
    {
      // in the case of disabled IRQs and a pending IRQ directly simulate an
      // external interrupt intercept
      if (   !(vcpu->_saved_state & Vcpu_state::F_irqs)
          && (vcpu->sticky_flags & Vcpu_state::Sf_irq_pending))
        {
          vm_state->exit_reason = ER_irq;
          err = 1; // return 1 to indicate pending IRQs (IPCs)
          break;
        }

      log_vm(vm_state, 1);

      if (!get_fpu())
        {
          err = -L4_err::EInval;
          break;
        }

      err = do_resume_vcpu(ctxt, vcpu, vm_state);

      log_vm(vm_state, 0);

      // check for IRQ exits and allow to handle the IRQ
      if (vm_state->exit_reason == ER_irq)
        Proc::preemption_point();

      if (vm_state->exit_reason == ER_tlb ||
          vm_state->exit_reason == ER_vmm_exit)
        break;

      // Check if the current context got a message delivered.
      // This is done by testing for a valid continuation.
      // When a continuation is set we have to directly
      // leave the kernel to not overwrite the vcpu-regs
      // with bogus state.
      Thread *t = nonull_static_cast<Thread*>(ctxt);

      if (t->continuation_test_and_restore())
        {
          ctxt->arch_load_vcpu_kern_state(vcpu, true);
          t->fast_return_to_user(vcpu->_entry_ip, vcpu->_entry_sp,
                                 t->vcpu_state().usr().get());
        }
    }

  ctxt->arch_load_vcpu_kern_state(vcpu, true);

  return err;
}

// --------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mips_vz && fpu]:

PUBLIC
bool
Vm::get_fpu()
{
  if (current()->state() & Thread_vcpu_fpu_disabled)
    return true;

  if (!(current()->state() & Thread_fpu_owner))
    {
      if (!current_thread()->switchin_fpu())
        {
          printf("mips_vz: switchin_fpu failed\n");
          return false;
        }
    }
  return true;
}

// --------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mips_vz && !fpu]:

PUBLIC
bool
Vm::get_fpu()
{ return true; }

// --------------------------------------------------------------------------
IMPLEMENTATION [debug && mips_vz]:

#include "jdb.h"

PRIVATE
Mword
Vm::jdb_get(Mword *state_ptr)
{
  Mword v = ~0UL;
  Jdb::peek(state_ptr, this, v);
  return v;
}

PUBLIC
void
Vm::dump_vm_state()
{
  Vm_state *s = state_for_dbg;
  Trap_state *ts = s->ts;

  printf("epc: %08lx  sr: %08lx exit_reason:%ld \n",
         jdb_get(&ts->epc), jdb_get(&ts->status), jdb_get((Mword*)&s->exit_reason));
  printf("r0[%2d]: %08lx at[%2d]: %08lx v0[%2d]: %08lx v1[%2d]: %08lx\n",
    0, jdb_get(&ts->r[0]), 1, jdb_get(&ts->r[1]), 2, jdb_get(&ts->r[2]), 3, jdb_get(&ts->r[3]));
  printf("a0[%2d]: %08lx a1[%2d]: %08lx a2[%2d]: %08lx a3[%2d]: %08lx\n",
    4, jdb_get(&ts->r[4]), 5, jdb_get(&ts->r[5]), 6, jdb_get(&ts->r[6]), 7, jdb_get(&ts->r[7]));
  printf("t0[%2d]: %08lx t1[%2d]: %08lx t2[%2d]: %08lx t3[%2d]: %08lx\n",
    8, jdb_get(&ts->r[8]), 9, jdb_get(&ts->r[9]), 10, jdb_get(&ts->r[10]), 11, jdb_get(&ts->r[11]));
  printf("t4[%2d]: %08lx t5[%2d]: %08lx t6[%2d]: %08lx t7[%2d]: %08lx\n",
    12, jdb_get(&ts->r[12]), 13, jdb_get(&ts->r[13]), 14, jdb_get(&ts->r[14]), 15, jdb_get(&ts->r[15]));
  printf("s0[%2d]: %08lx s1[%2d]: %08lx s2[%2d]: %08lx s3[%2d]: %08lx\n",
    16, jdb_get(&ts->r[16]), 17, jdb_get(&ts->r[17]), 18, jdb_get(&ts->r[18]), 19, jdb_get(&ts->r[19]));
  printf("s4[%2d]: %08lx s5[%2d]: %08lx s6[%2d]: %08lx s7[%2d]: %08lx\n",
    20, jdb_get(&ts->r[20]), 21, jdb_get(&ts->r[21]), 22, jdb_get(&ts->r[22]), 23, jdb_get(&ts->r[23]));
  printf("t8[%2d]: %08lx t9[%2d]: %08lx k0[%2d]: %08lx k1[%2d]: %08lx\n",
    24, jdb_get(&ts->r[24]), 25, jdb_get(&ts->r[25]), 26, jdb_get(&ts->r[26]), 27, jdb_get(&ts->r[27]));
  printf("gp[%2d]: %08lx sp[%2d]: %08lx s8[%2d]: %08lx ra[%2d]: %08lx\n",
    28, jdb_get(&ts->r[28]), 29, jdb_get(&ts->r[29]), 30, jdb_get(&ts->r[30]), 31, jdb_get(&ts->r[31]));
}

PUBLIC
void
Vm::show_short(String_buffer *buf)
{
  buf->printf(" utcb:%lx epc:%lx ", (Mword)state_for_dbg, (Mword)jdb_get(&state_for_dbg->ts->epc));
}

IMPLEMENT
void
Vm::Vm_log::print(String_buffer *buf) const
{
  buf->printf("%s: epc:%08lx sr:%lx er:%lx exc:%lx gexc:%lx",
              is_entry ? "entry" : "exit ",
              pc, sr, exit_reason, exit_exccode, exit_gexccode);
}

PUBLIC static inline
void
Vm::log_vm(Vm_state *vm_state, bool is_entry)
{
  LOG_TRACE("VM entry/exit", "VM", current(), Vm_log,
      l->is_entry = is_entry;
      l->pc = vm_state->ts->epc;
      l->sr = vm_state->ts->status;
      l->exit_reason = vm_state->exit_reason;
      l->exit_exccode = MIPS_TRAPTYPE(vm_state->ts->cause);
      l->exit_gexccode = vm_state->exit_gexccode;
  );
}

// --------------------------------------------------------------------------
IMPLEMENTATION [!debug && mips_vz]:

PUBLIC static inline
void
Vm::log_vm(Vm_state *, bool)
{}

// ------------------------------------------------------------------------
INTERFACE [obj_space_virt]:

#include "obj_space_phys_util.h"

EXTENSION class Vm : Obj_space_phys<Vm>
{
public:
  typedef Obj_space_phys<Vm> Vm_obj_space;
  using Task::ram_quota;
  static Ram_quota *ram_quota(Vm_obj_space const *obj_sp)
  { return static_cast<Vm const *>(obj_sp)->ram_quota(); }

  bool v_lookup(Obj_space::V_pfn const &virt, Obj_space::Phys_addr *phys,
                Obj_space::Page_order *size, Obj_space::Attr *attribs);

  L4_fpage::Rights v_delete(Obj_space::V_pfn virt, Order size,
                            L4_fpage::Rights page_attribs);
  Obj_space::Status v_insert(Obj_space::Phys_addr phys,
                             Obj_space::V_pfn const &virt,
                             Order size,
                             Obj_space::Attr page_attribs);

  Obj_space::Capability lookup(Cap_index virt);
  Obj_space::V_pfn obj_map_max_address() const;
  void caps_free();
};

//----------------------------------------------------------------------------
IMPLEMENTATION [obj_space_virt]:

//
// Utilities for map<Generic_obj_space> and unmap<Generic_obj_space>
//

IMPLEMENT
void __attribute__((__flatten__))
Vm::caps_free()
{ Vm_obj_space::caps_free(); }

IMPLEMENT
inline
bool __attribute__((__flatten__))
Vm::v_lookup(Obj_space::V_pfn const &virt, Obj_space::Phys_addr *phys,
             Obj_space::Page_order *size, Obj_space::Attr *attribs)
{ return Vm_obj_space::v_lookup(virt, phys, size, attribs); }

IMPLEMENT
inline __attribute__((__flatten__))
Obj_space::Capability __attribute__((__flatten__))
Vm::lookup(Cap_index virt)
{ return Vm_obj_space::lookup(virt); }

IMPLEMENT
inline __attribute__((__flatten__))
L4_fpage::Rights __attribute__((__flatten__))
Vm::v_delete(Obj_space::V_pfn virt, Obj_space::Page_order size,
             L4_fpage::Rights page_attribs)
{ return Vm_obj_space::v_delete(virt, size, page_attribs); }

IMPLEMENT
inline __attribute__((__flatten__))
Obj_space::Status __attribute__((__flatten__))
Vm::v_insert(Obj_space::Phys_addr phys, Obj_space::V_pfn const &virt,
             Obj_space::Page_order size, Obj_space::Attr page_attribs)
{ return (Obj_space::Status)Vm_obj_space::v_insert(phys, virt, size, page_attribs); }

IMPLEMENT
inline __attribute__((__flatten__))
Obj_space::V_pfn
Vm::obj_map_max_address() const
{ return Vm_obj_space::obj_map_max_address(); }

// ------------------------------------------------------------------------------
IMPLEMENTATION [obj_space_virt && debug]:

PUBLIC static inline
Vm *
Vm::get_space(Vm_obj_space *base)
{ return static_cast<Vm*>(base); }

PUBLIC virtual
Vm::Vm_obj_space::Entry *
Vm::jdb_lookup_cap(Cap_index index)
{ return Vm_obj_space::jdb_lookup_cap(index); }

// ------------------------------------------------------------------------------
IMPLEMENTATION [obj_space_virt && !debug]:

PUBLIC static inline
Vm *
Vm::get_space(Vm_obj_space *)
{ return 0; }

// ------------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mips_vz]:

PUBLIC static inline
Vm::Vm_state*
Vm::get_vm_state(Vcpu_state *vcpu)
{
  return reinterpret_cast<Vm::Vm_state*>((char*)vcpu + Vm::L4_VCPU_OFFSET_VM_STATE);
}

PUBLIC static
void
Vm::readout_vmstate_at_reset(Vm_state *vm_state)
{
  if (!vmstate_at_reset.sizeof_vmstate)
  {
    auto guard = lock_guard(cpu_lock);

    if (!vmstate_at_reset.sizeof_vmstate)
    {
      vmstate_at_reset.ts = NULL;
      readout_guest_cp0_regs(&vmstate_at_reset);
      readout_host_cp0(&vmstate_at_reset);
      vmstate_at_reset.sizeof_vmstate = sizeof(Vm_state);
    }
  }

  memcpy(vm_state, &vmstate_at_reset, sizeof(Vm_state));
}

PUBLIC static
void
Vm::init_vm_state(Vcpu_state *vcpu)
{
  Vm_state *vm_state = Vm::get_vm_state(vcpu);

  vm_state->version = 0;
  if (!cpu_has_vz) {
    vm_state->sizeof_vmstate = -1;
    return;
  }

  if (!cpu_has_guestctl2)
    WARN("MIPS VZ guestctl2 register not present; required for guest interrupt injection");

  readout_vmstate_at_reset(vm_state);

  vm_state->ts = &vcpu->_ts;
  vm_state->version = Vm::L4_VM_STATE_VERSION;

  // VMM takes care of guestctl0 initialization

  // set the initial offset for the guest count
  vm_state->host_cp0_gtoffset = read_c0_count();
}

PRIVATE static
bool
Vm::set_cp0(unsigned reg, unsigned sel, unsigned long val)
{
#if 1
  (void)reg; (void)sel; (void)val;
  return false;
#else
  // Kyma: This API should be at vcpu thread level, here at task level the
  // set_cp0 changes can get lost by an inopportune context switch away from the
  // VMM after set_cp0 but before resume_vcpu is called; consider removing
  // altogether from Fiasco and L4Re if not required as it also brings security
  // concerns.
  // Another issue with this API is that vm_state->host_cp0_* is not updated.
  if ((reg == MIPS_CP0_GTOFFSET) && (sel == MIPS_CP0_GTOFFSET_SEL))
    write_c0_gtoffset(val);
  else if ((reg == MIPS_CP0_GUESTCTL0) && (sel == MIPS_CP0_GUESTCTL0_SEL))
    write_c0_guestctl0(val);
  else if ((reg == MIPS_CP0_GUESTCTL0EXT) && (sel == MIPS_CP0_GUESTCTL0EXT_SEL) && cpu_has_guestctl0ext)
    write_c0_guestctl0ext(val);
  else if ((reg == MIPS_CP0_GUESTCTL1) && (sel == MIPS_CP0_GUESTCTL1_SEL) && cpu_has_guestctl1)
    write_c0_guestctl1(val);
  else if ((reg == MIPS_CP0_GUESTCTL2) && (sel == MIPS_CP0_GUESTCTL2_SEL) && cpu_has_guestctl2)
    write_c0_guestctl2(val);
  else
    return false;

  return true;
#endif
}

PRIVATE static
bool
Vm::get_cp0(unsigned reg, unsigned sel, unsigned long *val)
{
  if ((reg == MIPS_CP0_PRID) && (sel == MIPS_CP0_PRID_SEL))
    *val = read_c0_prid();
#if 0
  else if ((reg == MIPS_CP0_GTOFFSET) && (sel == MIPS_CP0_GTOFFSET_SEL))
    *val = read_c0_gtoffset();
  else if ((reg == MIPS_CP0_GUESTCTL0) && (sel == MIPS_CP0_GUESTCTL0_SEL))
    *val = read_c0_guestctl0();
  else if ((reg == MIPS_CP0_GUESTCTL0EXT) && (sel == MIPS_CP0_GUESTCTL0EXT_SEL) && cpu_has_guestctl0ext)
    *val = read_c0_guestctl0ext();
  else if ((reg == MIPS_CP0_GUESTCTL1) && (sel == MIPS_CP0_GUESTCTL1_SEL) && cpu_has_guestctl1)
    *val = read_c0_guestctl1();
  else if ((reg == MIPS_CP0_GUESTCTL2) && (sel == MIPS_CP0_GUESTCTL2_SEL) && cpu_has_guestctl2)
    *val = read_c0_guestctl2();
#endif
  else
    return false;

  return true;
}

PRIVATE
bool
Vm::invoke_arch(L4_msg_tag &tag, Utcb *utcb)
{
  switch (utcb->values[0])
    {
    case Cp0_mips:
        {
          enum
          {
            Cp0_mips_set_op = 1,
            Cp0_mips_get_op = 2,
            Cp0_mips_op_max
          };

          if (EXPECT_FALSE(tag.words() != 5))
            {
              tag = commit_result(-L4_err::EInval);
              return true;
            }

          unsigned op  = utcb->values[1];
          unsigned reg = utcb->values[2];
          unsigned sel = utcb->values[3];

          Mword error = 0;

          if (reg > 31 || sel > 7)
            {
              error = -L4_err::ERange;
            }
          else if (op == Cp0_mips_set_op)
            {
              if (false == set_cp0(reg, sel, utcb->values[4]))
                error = -L4_err::ENosys;
            }
          else if (op == Cp0_mips_get_op)
            {
              if (false == get_cp0(reg, sel, &utcb->values[4]))
                error = -L4_err::ENosys;
            }
          else
            {
              error = -L4_err::EInval;
            }

          tag = commit_result(error);
        }
      return true;

    default:
      break;
    }
  return false;
}
