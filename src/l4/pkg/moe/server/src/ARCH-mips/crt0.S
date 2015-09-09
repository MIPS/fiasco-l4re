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
 * \brief	Startup code
 */

#define __ASSEMBLY__
#include <l4/sys/compiler.h>

/*--- .text (program code) -------------------------*/

.section .text, "ax"
.type _real_start, @function
.globl _real_start
_real_start:
  la    $29, _stack_top
  subu  $29, 6 * 4
  j     __start
1:
    b   1b

/*--- my stack -------------------------------------*/
.section ".bss", "aw"
.global _stack_bottom
_stack_bottom:
	.space 0x4000
.global _stack_top
_stack_top:
