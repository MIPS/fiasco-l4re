/*
 * (c) 2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
#pragma once

#include <l4/sys/types.h>

/**
 * \brief vCPU registers.
 * \ingroup l4_vcpu_api
 */
typedef struct l4_vcpu_regs_t
{
  l4_umword_t pfa;
  l4_umword_t err;

  l4_umword_t reserved;
  l4_umword_t r[13];

  l4_umword_t sp;
  l4_umword_t lr;
  l4_umword_t _dummy;
  l4_umword_t ip;
  l4_umword_t flags;
} l4_vcpu_regs_t;

typedef struct l4_vcpu_arch_state_t
{
  l4_umword_t host_tpidruro;
  l4_umword_t user_tpidruro;
} l4_vcpu_arch_state_t;

/**
 * \brief vCPU message registers.
 * \ingroup l4_vcpu_api
 */
typedef struct l4_vcpu_ipc_regs_t
{
  l4_msgtag_t tag;
  l4_umword_t _d1[3];
  l4_umword_t label;
  l4_umword_t _d2[8];
} l4_vcpu_ipc_regs_t;
