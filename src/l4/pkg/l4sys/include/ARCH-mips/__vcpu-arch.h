/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#pragma once

#include <l4/sys/types.h>

/**
 * \brief vCPU registers.
 * \ingroup l4_vcpu_api
 * 
 * l4_exc_regs_t matches l4_vcpu_regs_t and corresponds to
 * fiasco/src/kern/mips32/trap_state.cpp: Trap_state_regs and
 * entry_frame-mips32.cpp: Syscall_frame and Return_frame
 */
typedef struct l4_vcpu_regs_t
{
  l4_umword_t pfa;
  l4_umword_t err;

  l4_umword_t r[32];

  l4_umword_t lo, hi;
  l4_umword_t status;
  l4_umword_t ip;      /**< epc/pc */
  l4_umword_t cause;
  l4_umword_t badvaddr;
  l4_umword_t sp;      /**< usp/stack pointer */
} l4_vcpu_regs_t;

typedef struct l4_vcpu_arch_state_t {} l4_vcpu_arch_state_t;

/**
 * \brief vCPU message registers.
 * \ingroup l4_vcpu_api
 *
 * l4_vcpu_ipc_regs_t register usage matches the implementation of l4_ipc() in
 * l4sys/include/ARCH-mips/L4API-l4f/ipc.h
 */
typedef struct l4_vcpu_ipc_regs_t
{
  l4_umword_t _d0[16];
  void*       utcb;     /* s0 */
  l4_umword_t dest;     /* s1 */
  l4_umword_t timeout;  /* s2 */
  l4_msgtag_t tag;      /* s3 */
  l4_umword_t label;    /* s4 */
  l4_umword_t sc_num;   /* s5 */
  l4_umword_t _d1[10];
} l4_vcpu_ipc_regs_t;
