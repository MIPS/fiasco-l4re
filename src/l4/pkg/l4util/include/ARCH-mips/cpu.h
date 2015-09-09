/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/**
 * \file
 * \brief  CPU related functions
 *
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */

/*
 * (c) 2004-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */

#ifndef __L4_UTIL__ARCH_MIPS__CPU_H__
#define __L4_UTIL__ARCH_MIPS__CPU_H__

EXTERN_C_BEGIN

/**
 * \defgroup l4util_cpu CPU related functions
 * \ingroup l4util_api
 */
/*@{*/

/**
 * Check whether the CPU supports the "cpuid" instruction.
 *
 * \return 1 if it has, 0 if it has not
 */
L4_INLINE int          l4util_cpu_has_cpuid(void);

/**
 * Generic CPUID access function.
 */
L4_INLINE void
l4util_cpu_cpuid(unsigned long mode,
                 unsigned long *eax, unsigned long *ebx,
                 unsigned long *ecx, unsigned long *edx);

/*@}*/
L4_INLINE int
l4util_cpu_has_cpuid(void)
{
  return 0;
}

L4_INLINE void
l4util_cpu_cpuid(unsigned long mode,
                 unsigned long *eax, unsigned long *ebx,
                 unsigned long *ecx, unsigned long *edx)
{
  (void)mode; (void)eax; (void)ebx; (void)ecx; (void)edx;
}

EXTERN_C_END

#endif /* __L4_UTIL__ARCH_MIPS__CPU_H__ */
