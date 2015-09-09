/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/**
 * \file
 * \brief   L4 IPC System Calls, MIPS
 * \ingroup api_calls
 */
/*
 * (c) 2010    Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *             Björn Döbel <doebel@os.inf.tu-dresden.de>
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

#include_next <l4/sys/ipc.h>

#ifdef __GNUC__

#include <l4/sys/compiler.h>
#include <l4/sys/syscall_defs.h>

L4_INLINE l4_msgtag_t
l4_ipc(l4_cap_idx_t dest, l4_utcb_t *utcb,
       l4_umword_t flags,
       l4_umword_t slabel,
       l4_msgtag_t tag,
       l4_umword_t *rlabel,
       l4_timeout_t timeout) L4_NOTHROW
{
  register void*       _utcb     __asm__("s0") = utcb;
  register l4_umword_t _dest     __asm__("s1") = dest | flags;
  register l4_umword_t _timeout  __asm__("s2") = timeout.raw;
  register l4_umword_t _tag      __asm__("s3") = tag.raw;
  register l4_umword_t _label    __asm__("s4") = slabel;
  register l4_umword_t _sc_num   __asm__("s5") = L4_SYSCALL_INVOKE;

  __asm__ __volatile__
    ("syscall \n\t"
     :
     "+r" (_dest),
     "+r" (_timeout),
     "+r" (_label),
     "+r" (_tag),
     "+r" (_utcb),
     "+r" (_sc_num)
     :
     :
     "memory");

  if (rlabel)
    *rlabel = _label;
  tag.raw = _tag;

  return tag;
}

L4_INLINE l4_msgtag_t
l4_ipc_call(l4_cap_idx_t dest, l4_utcb_t *utcb,
            l4_msgtag_t tag,
            l4_timeout_t timeout) L4_NOTHROW
{
  return l4_ipc(dest, utcb, L4_SYSF_CALL, 0, tag, 0, timeout);
}

L4_INLINE l4_msgtag_t
l4_ipc_reply_and_wait(l4_utcb_t *utcb, l4_msgtag_t tag,
                      l4_umword_t *label,
                      l4_timeout_t timeout) L4_NOTHROW
{
  return l4_ipc(L4_INVALID_CAP, utcb, L4_SYSF_REPLY_AND_WAIT, 0, tag, label, timeout);
}

L4_INLINE l4_msgtag_t
l4_ipc_send_and_wait(l4_cap_idx_t dest, l4_utcb_t *utcb,
                     l4_msgtag_t tag,
                     l4_umword_t *src,
                     l4_timeout_t timeout) L4_NOTHROW
{
  return l4_ipc(dest, utcb, L4_SYSF_SEND_AND_WAIT, 0, tag, src, timeout);
}

L4_INLINE l4_msgtag_t
l4_ipc_send(l4_cap_idx_t dest, l4_utcb_t *utcb,
            l4_msgtag_t tag,
            l4_timeout_t timeout) L4_NOTHROW
{
  return l4_ipc(dest, utcb, L4_SYSF_SEND, 0, tag, 0, timeout);
}

L4_INLINE l4_msgtag_t
l4_ipc_wait(l4_utcb_t *utcb, l4_umword_t *src,
            l4_timeout_t timeout) L4_NOTHROW
{
  l4_msgtag_t t;
  t.raw = 0;
  return l4_ipc(L4_INVALID_CAP, utcb, L4_SYSF_WAIT, 0, t, src, timeout);
}

L4_INLINE l4_msgtag_t
l4_ipc_receive(l4_cap_idx_t src, l4_utcb_t *utcb,
               l4_timeout_t timeout) L4_NOTHROW
{
  l4_msgtag_t t;
  t.raw = 0;
  return l4_ipc(src, utcb, L4_SYSF_RECV, 0, t, 0, timeout);
}

#include <l4/sys/ipc-impl.h>

#endif //__GNUC__