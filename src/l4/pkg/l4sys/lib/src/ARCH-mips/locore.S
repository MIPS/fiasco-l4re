/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 */

/* This routine makes changes to the instruction stream effective to the hardware.
 * It should be called after the instruction stream is written.
 * On return, the new instructions are effective.
 * Inputs:
 * a0 = Start address of new instruction stream
 * a1 = Size, in bytes, of new instruction stream
 */

#define HW_SYNCI_Step       $1
  .text
  .globl syncICache;
  .ent  syncICache, 0;
syncICache: ;
  .set push
  .set noreorder
  .set mips32r2

    beq     $a1, $0, 20f
    nop
    addu    $a1, $a0, $a1
    rdhwr   $v0, HW_SYNCI_Step
    beq     $v0, $0, 20f
    nop

10:
    synci   0($a0)
    addu    $a0, $a0, $v0
    sltu    $v1, $a0, $a1
    bne     $v1, $0, 10b
    nop
    sync
20:
    jr.hb   $ra
    nop
    .set pop
.end  syncICache

