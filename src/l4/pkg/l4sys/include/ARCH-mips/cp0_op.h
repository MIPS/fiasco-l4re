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
 * \brief   MIPS CP0 operations.
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_SYS__ARCH_MIPS__CP0_OP_H__
#define __L4_SYS__ARCH_MIPS__CP0_OP_H__

#ifndef L4API_l4f
#error This header file can only be used with a L4API version!
#endif

#include <l4/sys/ipc.h>

/**
 * Set a privileged cp0 register
 * \ingroup api_calls_fiasco
 *
 * \param       task                    Task to set the register for.
 * \param       reg                     CP0 Register.
 * \param       sel                     CP0 Select.
 * \param       val                     Value to write.
 * \param       utcb                    UTCB of the caller.
 * \return System call error
 */
L4_INLINE long
fiasco_mips_cp0_set(l4_cap_idx_t task, unsigned int reg, unsigned int sel,
               unsigned long val, l4_utcb_t *utcb);

/**
 * Get a privileged cp0 register
 * \ingroup api_calls_fiasco
 *
 * \param       task                    Task to set the register for.
 * \param       reg                     CP0 Register.
 * \param       sel                     CP0 Select.
 * \param       val                     Value read.
 * \param       utcb                    UTCB of the caller.
 * \return System call error
 */
L4_INLINE long
fiasco_mips_cp0_get(l4_cap_idx_t task, unsigned int reg, unsigned int sel,
               unsigned long *val, l4_utcb_t *utcb);

/**
 * \brief Contants for MIPS cp0 handling.
 */
enum L4_task_cp0_mips_consts
{
  L4_TASK_CP0_MIPS_SET_OP = 1,
  L4_TASK_CP0_MIPS_GET_OP = 2,
};


/*****************************************************************************
 *** Implementation
 *****************************************************************************/

#include <l4/sys/task.h>
#include <l4/sys/thread.h>

L4_INLINE long
fiasco_mips_cp0_set(l4_cap_idx_t task, unsigned int reg, unsigned int sel,
               unsigned long val, l4_utcb_t *utcb)
{
  if (reg > 31)
    return -L4_EINVAL;
  if (sel > 7)
    return -L4_EINVAL;
  l4_utcb_mr_u(utcb)->mr[0] = L4_TASK_CP0_MIPS_OP;
  l4_utcb_mr_u(utcb)->mr[1] = L4_TASK_CP0_MIPS_SET_OP;
  l4_utcb_mr_u(utcb)->mr[2] = reg;
  l4_utcb_mr_u(utcb)->mr[3] = sel;
  l4_utcb_mr_u(utcb)->mr[4] = val;
  return l4_error_u(l4_ipc_call(task, utcb, l4_msgtag(L4_PROTO_TASK, 5, 0, 0), L4_IPC_NEVER), utcb);
}

L4_INLINE long
fiasco_mips_cp0_get(l4_cap_idx_t task, unsigned int reg, unsigned int sel,
               unsigned long *val, l4_utcb_t *utcb)
{
  long result;

  if (reg > 31)
    return -L4_EINVAL;
  if (sel > 7)
    return -L4_EINVAL;
  l4_utcb_mr_u(utcb)->mr[0] = L4_TASK_CP0_MIPS_OP;
  l4_utcb_mr_u(utcb)->mr[1] = L4_TASK_CP0_MIPS_GET_OP;
  l4_utcb_mr_u(utcb)->mr[2] = reg;
  l4_utcb_mr_u(utcb)->mr[3] = sel;
  l4_utcb_mr_u(utcb)->mr[4] = 0;
  result = l4_error_u(l4_ipc_call(task, utcb, l4_msgtag(L4_PROTO_TASK, 5, 0, 0), L4_IPC_NEVER), utcb);
  if (!result)
    *val = l4_utcb_mr_u(utcb)->mr[4];

  return result;
}

#endif /* ! __L4_SYS__ARCH_MIPS__CP0_OP_H__ */

