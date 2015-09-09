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
 * \brief  Low-level kernel functions for MIPS
 */
#ifndef L4SYS_KERNEL_OBJECT_IMPL_H__
#define L4SYS_KERNEL_OBJECT_IMPL_H__

#include <l4/sys/syscall_defs.h>

L4_INLINE l4_msgtag_t
l4_invoke_debugger(l4_cap_idx_t obj, l4_msgtag_t tag, l4_utcb_t *utcb) L4_NOTHROW
{
  register void*         _a4      __asm__("s0") = utcb;
  register unsigned long _obj     __asm__("s1") = obj;
  register unsigned long _a1      __asm__("s3") = tag.raw;
  register unsigned long _sc_num  __asm__("s5") = L4_SYSCALL_DEBUGGER;

  __asm__ __volatile__
    ("syscall \n\t"
     :
     "+r" (_obj),
     "+r" (_a1),
     "+r" (_a4),
     "+r" (_sc_num)
     :
     :
     "memory");

  tag.raw = _a1; // gcc doesn't like the return out of registers variables
  return tag;
}

#endif
