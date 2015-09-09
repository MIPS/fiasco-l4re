/* -*- asm -*- */

/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include "globalconfig.h"

        /* PAGE_SIZE 16K */
	.section .mp_tramp, "awx"
	.p2align 14

	.global _tramp_mp_entry
_tramp_mp_entry:
	j     BOOT_AP_CPU
        nop

.macro defvar name
.global \name
\name: .long 0
.endm

defvar _tramp_mp_spinlock

	.align 16
_tramp_mp_init_stack:
	.space 1024
.global _tramp_mp_init_stack_top
_tramp_mp_init_stack_top:

