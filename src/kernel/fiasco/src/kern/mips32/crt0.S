/* -*- c -*- */

/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 */

#include "regdef.h"
#include "asm.h"

LEAF(_start)
    mtc0  $0, $12, 0
    ehb
    mtc0  $0, $13, 0
    ehb
    la	sp, __init_stack 
    subu sp, 6 * SZREG
    j   bootstrap_main
1:
    b   1b
END(_start)

.section .bss
.align 12
.space 4096
  __init_stack:
.previous


