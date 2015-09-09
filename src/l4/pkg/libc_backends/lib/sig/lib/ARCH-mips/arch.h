/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * (c) 2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */
#pragma once

#include <ucontext.h>
#include <sys/ucontext.h>
#include <stdio.h>

static inline
void fill_ucontext_frame(ucontext_t *ucf, l4_exc_regs_t *ue)
{
#warning KYMA fill_ucontext_frame needs to be implemented for mips
  ucf->uc_mcontext.gregs[0]        = ue->r[0];
  asm volatile ("break\n");
  // ...
}

static inline
void fill_utcb_exc(l4_exc_regs_t *ue, ucontext_t *ucf)
{
#warning KYMA fill_utcb_exc needs to be implemented for mips
  ue->r[0] = ucf->uc_mcontext.gregs[0];
  asm volatile ("break\n");
  // ...
}

static inline
void show_regs(l4_exc_regs_t *u)
{
  printf("Exception: State (tbd):\n");
  (void)u;
}
