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
 * \brief MIPS virtualization interface.
 */
#ifndef __INCLUDE__ARCH_MIPS__VM_H__
#define __INCLUDE__ARCH_MIPS__VM_H__

#pragma once
#include <l4/sys/vcpu.h>

/**
 * \defgroup l4_vm_vz_api VM API for MIPS VZ
 * \brief Virtual Machine API for MIPS Virtualization Extension
 * \ingroup l4_vm_api
 */

enum l4_vcpu_consts_mips
{
  L4_VM_STATE_VERSION = 1,
  L4_N_MIPS_COPROC_REGS = 32,
  L4_N_MIPS_COPROC_SEL  = 8
};

/**
 * \brief state structure for MIPS VZ VMs
 * \ingroup l4_vm_vz_api
 *
 * Kept synchronized with fiasco kern/mips32/vm.cpp
 * bump L4_VM_STATE_VERSION in both files when changing l4_vm_vz_state
 */
typedef struct l4_vm_vz_state
{
  l4_vcpu_regs_t *r;          ///< Register state

  l4_umword_t version;        // initialized by kernel to L4_VM_STATE_VERSION
  l4_umword_t sizeof_vmstate; // initialized by kernel to sizeof(l4_vm_vz_state_t)

  l4_umword_t exit_reason;
  l4_umword_t exit_gexccode;  // contains GuestCtl0 GExcCode field

  // badinstr and badinstrp are only valid for exceptions which are synchronous
  // to an instruction: Coprocessor Unusable, Reserved Instruction, Integer
  // Overflow, Trap, System Call, Breakpoint, Floating Point, Coprocessor 2
  // exception, Address Error, TLB Refill, TLB Invalid, TLB Read Inhibit, TLB
  // Execute Inhibit, TLB Modified
  l4_umword_t exit_badinstr;  // contains faulting instruction
  l4_umword_t exit_badinstrp; // contains prior branch instr when faulting instr is in BD slot
  l4_umword_t host_cp0_gtoffset;
  l4_umword_t host_cp0_guestctl0;
  l4_umword_t host_cp0_guestctl0ext;
  l4_umword_t host_cp0_guestctl1;
  l4_umword_t host_cp0_guestctl2;

  l4_uint8_t set_cp0_guestctl0;
  l4_uint8_t set_cp0_guestctl0ext;

  l4_uint8_t set_injected_ipx;
  l4_umword_t injected_ipx;

  l4_umword_t gcp0[L4_N_MIPS_COPROC_REGS][L4_N_MIPS_COPROC_SEL];
} l4_vm_vz_state_t;

typedef enum L4_vm_exit_reason
{
  L4_vm_exit_reason_undef      = 0,
  L4_vm_exit_reason_none       = 1,
  L4_vm_exit_reason_vmm_exit   = 2,
  L4_vm_exit_reason_irq        = 3,
  L4_vm_exit_reason_tlb        = 4,
  L4_vm_exit_reason_ipc        = 5,
  L4_vm_exit_reason_max        = 6,
} L4_vm_exit_reason;

L4_INLINE const char*
L4_vm_exit_reason_to_str(L4_vm_exit_reason e) {

  const char* L4_vm_exit_reason_str[] = {
    "ER_undef",
    "ER_none",
    "ER_vmm_exit",
    "ER_irq",
    "ER_tlb",
    "ER_ipc",
    "ER_???",   // max
  };

  if (e > L4_vm_exit_reason_max)
    e = L4_vm_exit_reason_max;
 
  return L4_vm_exit_reason_str[e];
};

#endif /* ! __INCLUDE__ARCH_MIPS__VM_H__ */
