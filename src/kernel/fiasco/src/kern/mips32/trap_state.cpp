/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE:

#include "l4_types.h"
#include "entry_frame.h"
#include "mipsdefs.h"
#include "processor.h"
#include "paging.h"

class Trap_state_regs
{
public:
  Mword pf_address;
  Mword error_code;     /* L4_trap_code | Jdb_trap_code | Pagefault_code | Exc_code */
};

/*
 * The Trap_state layout in memory matches the offsets in types/mips32/mipsdefs.h.
 * The Entry_frame (Syscall_frame + Return_frame) must be after Trap_state_regs
 * because of how Context::regs() works.
 */
class Trap_state : public Trap_state_regs, public Entry_frame
{

public:

  /* Trap codes = Exception cause codes (5 bits in Cause register) */
  enum Exc_code {
    Exc_code_Int      = 0,       // Interrupt
    Exc_code_Mod      = 1,       // TLB modification exception
    Exc_code_TLBL     = 2,       // TLB exception (load or fetch)
    Exc_code_TLBS     = 3,       // TLB exception (store)
    Exc_code_AdEL     = 4,       // Address error exception (load or fetch)
    Exc_code_AdES     = 5,       // Address error exception (store)
    Exc_code_IBE      = 6,       // Bus error exception (instruction bus)
    Exc_code_DBE      = 7,       // Bus error exception (data bus)
    Exc_code_Sys      = 8,       // Syscall exception
    Exc_code_Bp       = 9,       // Breakpoint exception
    Exc_code_RI       = 10,      // Reserved instruction exception
    Exc_code_CpU      = 11,      // Coprocessor unusable exception
    Exc_code_Ov       = 12,      // Arithmetic overflow exception
    Exc_code_Tr       = 13,      // Trap
    Exc_code_Res1     = 14,      // (reserved)
    Exc_code_FPE      = 15,      // Floating point exception
    Exc_code_Impl1    = 16,      // (implementation-dependent 1)
    Exc_code_Impl2    = 17,      // (implementation-dependent 2)
    Exc_code_C2E      = 18,      // (reserved for precise coprocessor 2)
    Exc_code_TLBRI    = 19,      // TLB exception (read)
    Exc_code_TLBXI    = 20,      // TLB exception (execute)
    Exc_code_Res2     = 21,      // (reserved)
    Exc_code_MDMX     = 22,      // (MIPS64 MDMX unusable)
    Exc_code_WATCH    = 23,      // Reference to watchHi/watchLo address
    Exc_code_MCheck   = 24,      // Machine check exception
    Exc_code_Thread   = 25,      // Thread exception
    Exc_code_DSPDis   = 26,      // DSP disabled exception
    Exc_code_GE       = 27,      // Guest Exit exception
    Exc_code_Res4     = 28,      // (reserved)
    Exc_code_Prot     = 29,      // Protection exception
    Exc_code_CacheErr = 30,      // Cache error exception
    Exc_code_Res6     = 31,      // (reserved)
    Exc_code_LAST     = 32,      // KEEP LAST: for sizing
    Exc_code_mask     = 0x1f
  };

  /* Pagefault trap codes */
  enum Pagefault_code {
    Pf_err_notpresent = PF::PF_ERR_NOTPRESENT, ///< PF: Page Is NOT Present In PTE
    Pf_err_writeprot  = PF::PF_ERR_WRITEPROT,  ///< PF: Page Is Write Protected
    Pf_err_usrmode    = PF::PF_ERR_USRMODE,    ///< PF: Caused By User Mode Code
    Pf_err_vzguest    = PF::PF_ERR_VZGUEST,    ///< PF: Caused By VZ Guest
    Pf_err_mask       = PF::PF_ERR_MASK        ///< PF: Pagefault Error Mask
  };

  /* JDB interface trap codes */
  enum Jdb_trap_code
  {
    Jdb_debug_trap         = 0x00010000,
    Jdb_debug_sequence     = 0x00020000,
    Jdb_debug_ipi          = 0x00030000,
    Jdb_trap_code_mask     = 0x000f0000, /* Mask for JDB interface trap codes */
  };

  /* Fiasco interface trap codes */
  enum L4_trap_code
  {
    L4_ipc_upcall         = 0x10000000, /* VCPU upcall */
    L4_trigger_exception  = 0x20000000, /* VCPU trigger exception */
    L4_trap_code_mask     = 0xf0000000, /* Mask for L4 interface trap codes */
  };

  enum Trap_state_reg_offset
  {
    /* Trap state register offsets */
    TS_REG_GPR_END    = FRAME_REG_RA + 1,
    TS_REG_SR         = FRAME_REG_SR,
    TS_REG_EPC        = FRAME_REG_EPC,
  };

public:
  typedef FIASCO_FASTCALL int (*Handler)(Trap_state*, Cpu_number::Value cpu, bool is_guestcontext);
  static Handler base_trap_handler asm ("BASE_TRAP_HANDLER");
  bool exclude_logging() { return false; }
};


IMPLEMENTATION:

#include <cstdio>
#include "static_assert.h"
#include "stringify.h"

static_assert(sizeof(Trap_state) == FRAME_SIZ, "Trap_state and asm FRAME_SIZ overlay mismatch");

Trap_state::Handler Trap_state::base_trap_handler FIASCO_FASTCALL;

PUBLIC inline
void
Trap_state::sanitize_user_state()
{
  /*
   * set user mode, enable interrupts and interrupt masks, and
   * set EXL to prepare for eret to user mode
   */
  status = Cp0_Status::ST_KSU_USER | Cp0_Status::ST_EXL |
    Cp0_Status::ST_IE | (read_c0_status() & Cp0_Status::ST_IP);
}

PUBLIC inline
unsigned long
Trap_state::trapno() const
{ return error_code & Exc_code_mask; }

PUBLIC inline
Mword
Trap_state::error() const
{ return error_code; }

PUBLIC inline
void
Trap_state::set_errorcode(Mword error)
{
  pf_address = 0;
  error_code = error;
}

PUBLIC inline
void
Trap_state::set_ipc_upcall()
{
  // Kyma: keep in synch with pkg/libvcpu/include/ARCH-mips/vcpu_arch.h
  pf_address = 0;
  error_code = L4_ipc_upcall;
}

PUBLIC inline
void
Trap_state::set_pagefault(Mword pfa, Mword error)
{
  pf_address = pfa;
  error_code = error;
}

PUBLIC inline
bool
Trap_state::is_debug_exception() const
{ return false; }

PUBLIC
void
Trap_state::dump()
{
  printf("EXCEPTION: pfa=%08lx, error=%08lx exc_code %s (%ld)\n",
          pf_address, error_code, exc_code_str(trapno()), trapno());

  Return_frame::dump();
  Syscall_frame::dump();

}

PUBLIC static
const char *
Trap_state::exc_code_str(Mword exc_code)
{
  switch(exc_code) {
    case Exc_code_Int:      return __stringify(Exc_code_Int);
    case Exc_code_Mod:      return __stringify(Exc_code_Mod);
    case Exc_code_TLBL:     return __stringify(Exc_code_TLBL);
    case Exc_code_TLBS:     return __stringify(Exc_code_TLBS);
    case Exc_code_AdEL:     return __stringify(Exc_code_AdEL);
    case Exc_code_AdES:     return __stringify(Exc_code_AdES);
    case Exc_code_IBE:      return __stringify(Exc_code_IBE);
    case Exc_code_DBE:      return __stringify(Exc_code_DBE);
    case Exc_code_Sys:      return __stringify(Exc_code_Sys);
    case Exc_code_Bp:       return __stringify(Exc_code_Bp);
    case Exc_code_RI:       return __stringify(Exc_code_RI);
    case Exc_code_CpU:      return __stringify(Exc_code_CpU);
    case Exc_code_Ov:       return __stringify(Exc_code_Ov);
    case Exc_code_Tr:       return __stringify(Exc_code_Tr);
    case Exc_code_Res1:     return __stringify(Exc_code_Res1);
    case Exc_code_FPE:      return __stringify(Exc_code_FPE);
    case Exc_code_Impl1:    return __stringify(Exc_code_Impl1);
    case Exc_code_Impl2:    return __stringify(Exc_code_Impl2);
    case Exc_code_C2E:      return __stringify(Exc_code_C2E);
    case Exc_code_TLBRI:    return __stringify(Exc_code_TLBRI);
    case Exc_code_TLBXI:    return __stringify(Exc_code_TLBXI);
    case Exc_code_Res2:     return __stringify(Exc_code_Res2);
    case Exc_code_MDMX:     return __stringify(Exc_code_MDMX);
    case Exc_code_WATCH:    return __stringify(Exc_code_WATCH);
    case Exc_code_MCheck:   return __stringify(Exc_code_MCheck);
    case Exc_code_Thread:   return __stringify(Exc_code_Thread);
    case Exc_code_DSPDis:   return __stringify(Exc_code_DSPDis);
    case Exc_code_GE:       return __stringify(Exc_code_GE);
    case Exc_code_Res4:     return __stringify(Exc_code_Res4);
    case Exc_code_Prot:     return __stringify(Exc_code_Prot);
    case Exc_code_CacheErr: return __stringify(Exc_code_CacheErr);
    case Exc_code_Res6:     return __stringify(Exc_code_Res6);
    default:                return __stringify(Exc_code_Invalid);
  }
}
