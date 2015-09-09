/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/**
 * \file
 * \brief   UTCB definitions for MIPS.
 * \ingroup l4_utcb_api
 */
#ifndef __L4_SYS__INCLUDE__ARCH_MIPS__UTCB_H__
#define __L4_SYS__INCLUDE__ARCH_MIPS__UTCB_H__

#include <l4/sys/types.h>

/**
 * \defgroup l4_utcb_api_mips MIPS Virtual Registers (UTCB)
 * \ingroup  l4_utcb_api
 */

/**
 * \brief UTCB structure for exceptions.
 * \ingroup l4_utcb_api_mips
 * 
 * l4_exc_regs_t matches l4_vcpu_regs_t and corresponds to
 * fiasco/src/kern/mips32/trap_state.cpp: Trap_state_regs and
 * entry_frame-mips32.cpp: Syscall_frame and Return_frame
 */
typedef struct l4_exc_regs_t
{
  l4_umword_t pfa;     /**< page fault address */
  l4_umword_t err;     /**< errorcode: L4_trap_code_mips | Pagefault_code_mips | Exc_code_mips */

  l4_umword_t r[32];   /**< registers */
  l4_umword_t lo, hi;
  l4_umword_t status;
  l4_umword_t epc;     /**< epc/pc */
  l4_umword_t cause;
  l4_umword_t badvaddr;
  l4_umword_t sp;      /**< usp/stack pointer */
} l4_exc_regs_t;

/**
 * \brief UTCB Exception trap codes.
 * \ingroup l4_utcb_api_mips
 *
 * errorcode: L4_trap_code_mips | Pagefault_code_mips | Exc_code_mips
 *
 * Exception cause codes (5 bits in Cause register)
 */
enum Exc_code_mips {
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

/**
 * \brief UTCB Pagefault trap codes.
 * \ingroup l4_utcb_api_mips
 *
 * errorcode: L4_trap_code_mips | Pagefault_code_mips | Exc_code_mips
 */
enum Pagefault_code_mips {
  Pf_err_notpresent = 0x00000100, ///< PF: Page Is NOT Present In PTE
  Pf_err_writeprot  = 0x00000200, ///< PF: Page Is Write Protected
  Pf_err_usrmode    = 0x00000400, ///< PF: Caused By User Mode Code
  Pf_err_vzguest    = 0x00000800, ///< PF: Caused By VZ Guest
  Pf_err_mask       = 0x0000ff00, ///< PF: Pagefault Error Mask
};

/**
 * \brief UTCB Fiasco L4 interface trap codes.
 * \ingroup l4_utcb_api_mips
 *
 * errorcode: L4_trap_code_mips | Pagefault_code_mips | Exc_code_mips
 */
enum L4_trap_code_mips
{
  L4_ipc_upcall         = 0x10000000, /* VCPU upcall */
  L4_trigger_exception  = 0x20000000, /* VCPU trigger exception */
  L4_trap_code_mask     = 0xf0000000, /* Mask for L4 interface trap codes */
};

/**
 * \brief UTCB constants for MIPS
 * \ingroup l4_utcb_api_mips
 * \hideinitializer
 */
enum L4_utcb_consts_mips
{
  L4_UTCB_EXCEPTION_REGS_SIZE    = sizeof(l4_exc_regs_t)/sizeof(l4_umword_t), /* 41 */
  L4_UTCB_GENERIC_DATA_SIZE      = 63,
  L4_UTCB_GENERIC_BUFFERS_SIZE   = 58,

  L4_UTCB_MSG_REGS_OFFSET        = 0,
  L4_UTCB_BUF_REGS_OFFSET        = 64 * sizeof(l4_umword_t),
  L4_UTCB_THREAD_REGS_OFFSET     = 123 * sizeof(l4_umword_t),

  L4_UTCB_INHERIT_FPU            = 1UL << 24,

  L4_UTCB_OFFSET                 = 512,
};

#include_next <l4/sys/utcb.h>

/*
 * ==================================================================
 * Implementations.
 */

#ifdef __GNUC__
L4_INLINE l4_utcb_t *l4_utcb_direct(void) L4_NOTHROW
{
  register l4_utcb_t *utcb __asm__ ("v0");
  asm volatile (".set push; .set noreorder; mfc0   %0,$28,3; .set pop" : "=r" (utcb));
  return utcb;
}
#endif

L4_INLINE l4_umword_t l4_utcb_exc_pc(l4_exc_regs_t *u) L4_NOTHROW
{
  return u->epc;
}

L4_INLINE void l4_utcb_exc_pc_set(l4_exc_regs_t *u, l4_addr_t pc) L4_NOTHROW
{
  u->epc = pc;
}

L4_INLINE l4_umword_t l4_utcb_exc_typeval(l4_exc_regs_t *u) L4_NOTHROW
{
  return u->err;
}

L4_INLINE int l4_utcb_exc_is_pf(l4_exc_regs_t *u) L4_NOTHROW
{
  return u->err & Pf_err_usrmode;
}

L4_INLINE l4_addr_t l4_utcb_exc_pfa(l4_exc_regs_t *u) L4_NOTHROW
{
  return (u->pfa & ~3) | (!(u->err & Pf_err_writeprot) << 1);
}

#endif /* ! __L4_SYS__INCLUDE__ARCH_MIPS__UTCB_H__ */
