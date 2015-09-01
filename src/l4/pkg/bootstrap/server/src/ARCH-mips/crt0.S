/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/* -*- c -*- */

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

.section .text.init, "ax"
.type _start, @function
.globl _start
_start:
  .set push
  .set noreorder

  /* for bootstrap, map the first 0x20000000 of memory 1:1 in USEG address space */
  li    $8, 0x00000000
  mtc0  $8, $0, 0          // mtc0 t0, c0_index
  li    $8, 0x1ffff800     // li   t0, 0x1ffff800
  mtc0  $8, $5, 0          // mtc0 t0, c0_pagemask (256MB)
  li    $8, 0x0000001F     // even page @0MB, cached, dirty, valid, global
  mtc0  $8, $2, 0          // mtc0 t0, c0_entrylo0
  li    $8, 0x0080001F     // odd page @256MB, cached, dirty, valid, global
  mtc0  $8, $3, 0          // mtc0 t0, c0_entrylo1
  li    $8, 0x00000000
  mtc0  $8, $10, 0         // mtc0 t0, c0_entryhi (1:1 mapping)
  ehb
  tlbwi
  ehb
  la    $29, crt0_stack_high
  subu  $29, 6 * 4
  la    $25, __main        // jump to USEG address space
  jr    $25
        nop
1:
    b   1b
        nop
  .set pop

.section ".bss", "aw"
.align 12
.global crt0_stack_low
crt0_stack_low:
  .space 4096
.global crt0_stack_high
crt0_stack_high:
