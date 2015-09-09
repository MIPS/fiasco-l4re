/*****************************************************************************/
/**
 * \file   drivers/fb/src/emul_time.c
 * \brief  L4DRIVERS: Linux time emulation
 *
 * \date   2007-05-29
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/*
 * (c) 2007-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

/* L4 */
#if !defined(ARCH_arm) && !defined(ARCH_sparc) && !defined(ARCH_mips)
#include <l4/util/rdtsc.h>     /* XXX x86 specific */
#endif
#include <l4/util/util.h>

/* Linux */
#if 1 //KYMA
#include "linux/delay.h"
#else
#include <asm/delay.h>
#endif // KYMA

/* UDELAY */
void udelay(unsigned long usecs)
{
#ifdef ARCH_arm
  l4_sleep(usecs/1000); // XXX
#elif defined(ARCH_mips)
#warning KYMA ARCH_mips l4_busy_wait_us() not implemented, using l4_usleep instead.
  // this gives up control of cpu; it's not always what's wanted...
  l4_usleep(usecs);
#else
  l4_busy_wait_us(usecs);
#endif
}
