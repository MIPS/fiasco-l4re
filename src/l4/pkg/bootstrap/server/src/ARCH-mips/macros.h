/*
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef __ARCH_MIPS_MACROS_H__
#define __ARCH_MIPS_MACROS_H__

#define L4_CHAR_PTR		(char*)
#define L4_CONST_CHAR_PTR	(const char*)
#define L4_VOID_PTR		(void*)
#define L4_MB_MOD_PTR		(l4util_mb_mod_t*)

/*
 * min()/max()/clamp() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })

#endif  /* ! __ARCH_MIPS_MACROS_H__ */
