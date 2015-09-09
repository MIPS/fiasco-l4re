/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include "regdef.h"

# define _C_LABEL(x)  x

#define CALLFRAME_SIZ (4 * (4 + 2))
#define CALLFRAME_SP  (4 * 4)
#define CALLFRAME_RA  (4 * 5)

#define REG_S       sw
#define REG_L       lw

/*
 * NESTED
 *  A function calls other functions and needs
 *  therefore stack space to save/restore registers.
 */
#define NESTED(x, fsize, retpc)   \
  .globl  _C_LABEL(x);    \
  .ent  _C_LABEL(x), 0;   \
_C_LABEL(x): ;        \
  .frame  sp, fsize, retpc; \

/*
 * END
 *  Mark end of a procedure.
 */
#define END(x) \
  .end _C_LABEL(x)

/*
 *   save layout:
 *
 *          -----------
 *     +152 |  ra     | 
 *     +144 |  t9     | 
 *          -----------
 *     +136 |  fcr    | 
 *          -----------
 *     +128 |  f31    | 
 *     +120 |  f30    | 
 *     +112 |  f29    | 
 *     +104 |  f28    | 
 *     +96  |  f27    | 
 *     +88  |  f26    | 
 *     +80  |  f25    | 
 *     +72  |  f24    | 
 *          -----------
 *     +64  |  s8     | 
 *     +56  |  s7     | 
 *     +48  |  s6     | 
 *     +40  |  s5     | 
 *     +32  |  s4     | 
 *     +24  |  s3     | 
 *     +16  |  s2     | 
 *     +8   |  s1     | 
 *   sp ->  |  s0     |
 *          -----------
 */

#define REGSZ     4
#define SOFF(n)   ((n)*REGSZ)
#define FPOFF(n)  (SOFF(9) + (n)*8)
#define REGOFF(n) (FPOFF(9) + (n)*REGSZ)

#define FRAMESIZE	(REGOFF(2) + REGSZ*4+REGSZ+REGSZ)

/* void cpu_switchto (Context *old, Context *new, Mword **old_sp, Mword **new_sp); */

NESTED(cpu_switchto, CALLFRAME_SIZ, ra)
  .set push
  .set noreorder
  subu  sp, sp, FRAMESIZE
  REG_S s0, SOFF(0)(sp)
  REG_S s1, SOFF(1)(sp)
  REG_S s2, SOFF(2)(sp)
  REG_S s3, SOFF(3)(sp)
  REG_S s4, SOFF(4)(sp)
  REG_S s5, SOFF(5)(sp)
  REG_S s6, SOFF(6)(sp)
  REG_S s7, SOFF(7)(sp)
  REG_S s8, SOFF(8)(sp)
  REG_S t9, REGOFF(0)(sp)
  REG_S ra, REGOFF(1)(sp)

  /*
   * Setup address at which to resume after context switch
   * and push to top of old stack
   */
  la    ra, 1f
  addiu sp, sp, -4
  REG_S ra, 0(sp)

  /* Switch stacks */
  REG_S sp, 0(a2) # Save old SP
  REG_L sp, 0(a3) # Load new SP

/*
 * Call switchin_context_label
 */
  move  s0, a0    #Save old context
  move  s1, a1    #Save new context
 
  .mask 0x80000000, -4
  la    t9, switchin_context_label
  move  a0, s1
  move  a1, s0    #Pass old context to Context::switchin_context()
  jalr  t9
  subu  sp, sp, CALLFRAME_SIZ # branch delay slot - allocate callframe

  addu  sp, sp, CALLFRAME_SIZ
  move  a0, s0    #Restore old context
  move  a1, s1    #Restore new context

  REG_L t9, 0(sp)
  jr    t9
  nop

  /*
   * resume after returning from context switch
   */
1:
  addiu sp, sp, 4
  REG_L ra, REGOFF(1)(sp)
  REG_L t9, REGOFF(0)(sp)
  REG_L s8, SOFF(8)(sp)
  REG_L s7, SOFF(7)(sp)
  REG_L s6, SOFF(6)(sp)
  REG_L s5, SOFF(5)(sp)
  REG_L s4, SOFF(4)(sp)
  REG_L s3, SOFF(3)(sp)
  REG_L s2, SOFF(2)(sp)
  REG_L s1, SOFF(1)(sp)
  REG_L s0, SOFF(0)(sp)
  addu  sp, sp, FRAMESIZE

  /* New context is now active */
  j     ra
        nop
END(cpu_switchto)



/* This routine makes changes to the instruction stream effective to the hardware.
 * It should be called after the instruction stream is written.
 * On return, the new instructions are effective.
 * Inputs:
 * a0 = Start address of new instruction stream
 * a1 = Size, in bytes, of new instruction stream
 */

#define HW_SYNCI_Step	    $1
  .text
  .globl syncICache;
  .ent  syncICache, 0;
syncICache: ;
  .set push
  .set noreorder
  .set mips32r2

    beq     a1, 0, 20f
    nop
    addu    a1, a0, a1
    rdhwr   v0, HW_SYNCI_Step
    beq     v0, 0, 20f
    nop

10:
    synci   0(a0)
    addu    a0, a0, v0
    sltu    v1, a0, a1
    bne     v1, 0, 10b
    nop
    sync
20:
    jr.hb   ra
    nop
    .set pop
.end  syncICache
