/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#pragma once

inline void
switch_stack(unsigned long stack, void (*func)())
{
  register unsigned long v0 asm("v0") = 0;
  asm volatile ( ".set push          \n\t"
                 ".set noreorder     \n\t"
                 "move $sp, %[stack] \n\t"
                 "jalr %[func]       \n\t"
                 "nop                \n\t"
                 ".set pop           \n\t"
		 : : [stack] "r" (stack), [func] "r" (func), "r" (v0)
		 : "memory");
}

