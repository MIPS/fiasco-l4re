/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#pragma once

#include <l4/sys/utcb.h>

L4_CV L4_INLINE
int
l4vcpu_is_irq_entry(l4_vcpu_state_t *vcpu) L4_NOTHROW
{
  return vcpu->r.err == L4_ipc_upcall;
}

L4_CV L4_INLINE
int
l4vcpu_is_page_fault_entry(l4_vcpu_state_t *vcpu) L4_NOTHROW
{
  return vcpu->r.err & Pf_err_usrmode;
}
