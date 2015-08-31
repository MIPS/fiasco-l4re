/* -*- asm -*- */

/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include "globalconfig.h"
#include "tcboffset.h"
#include "config_tcbsize.h"

#include "asm.h"
#include "regdef.h"
#include "mipsregs.h"
#include "mipsvzregs.h"
#include "mipsdefs.h"

#define COP0_SYNC   ehb

.section .exceptcommon, "ax"

/*******************************************************************************
 * System-call table
 */
.global syscall_table
syscall_table:
    .word sys_kdb_ke                # (0)
    .word sys_kdb_ke                # (1)
    .word sys_ipc_wrapper           # (2)
    .word sys_kdb_ke                # (3) mips mem_op syscall not implemented
    .word sys_invoke_debug_wrapper  # (4)
    .word sys_kdb_ke                # (5)
    .word sys_kdb_ke                # (6)
    .word sys_kdb_ke                # (7)
.space 4096

/*
 * Make sure the assembler does not reorder the exception
 * handling code, nor make changes to the $at register
 */

.section .text
    .set noreorder
    .set noat

/*
 * Main Exception Handler
 *
 * Vector code for the general exception vector 0x80000180
 *
 * Note : This code should be relocatable and have a max of 32
 * instructions since it is copied to the general exception vector
 * address to handle all general exceptions (hypervisor, guest and user)
 */

    .globl  _exceptionhandler
    .type   _exceptionhandler, function
    .ent    _exceptionhandler

_exceptionhandler:
/*
 * Find out what mode we came from and jump to the proper handler.
 */

#ifdef CONFIG_MIPS_VZ
    /* 
     * Save k0 and k1 in the scratch registers since they are
     * shared between root and guest contexts 
     */
    mtc0 k0, CP0_KSCRATCH1                    /* 00 : save k0 */
    mtc0 k1, CP0_KSCRATCH2                    /* 01 : save K1 */
#endif
    mfc0 k1, CP0_CAUSE                        /* 02: get the Cause Register */
    and k1, k1, CAUSEF_EXCCODE                /* 03: mask out the ExcCode bits */

#ifdef CONFIG_MIPS_VZ
    /* TBD : Need to handle the case for guest kernel too. Currently
     * just handling root and user exceptions */
    /*
     * Figure out whether we jumped from guest context or were
     * already in root context by using the GuestCtrl0_GM bit
     * If GuestCtrl0_GM bit is set, we choose the guest kernel
     * exception handlers from the exception handler table.
     * If GuestCtrl0_GM bit is not set, then use the Status_UM
     * bit to figure out whether to use the root(kernel) exception
     * handlers or the user exception handlers.
     * Note : The first 32 handlers in the table are for root
     * exceptions, the second 32 handlers are for user exceptions
     * and the third 32 handlers are for guest exceptions
     */
    mfc0 k0, CP0_GUESTCTL0                    /* 04: check GuestCtrl0_GM bit */
    bgtz k0, 1f                               /* 05: */
    nop                                       /* 06: TBD : Can replce the nop with 03 and */
    addiu k1, k1, 256                         /* 07: need the guest exception handlers */
    j 2f                                      /* 08: */
    nop                                       /* 09: branch delay slot */
1:
#endif
    mfc0 k0, CP0_STATUS                       /* 10: get the Status Register */
    and k0, k0, KSU_USER                      /* 11: test for User Mode */
    sll k0, k0, 3                             /* 12: */
    or  k1, k1, k0                            /* 13: pick correct index to exception table */
2:
    la k0, _exceptionhandler_base    /* 14: load base of the exception handler table */
    addu k0, k0, k1                  /* 15: get the address of the function entry. Note that
                                      *     the bits are already shifted left by 2 bits, so it
                                      *     forms the perfect index into the base table */
    lw k0, 0(k0)                     /* 16: get the handler function address */


    j k0                             /* 17: jump to the appropriate handler function */
    nop                              /* 18: Branch delay slot */


    .globl _exceptionhandlerEnd
_exceptionhandlerEnd:
    .end _exceptionhandler

/*
 **** Macros used for Exception Handling
 */

#define STATMASK 0x1f /* (ST0_KSU | ST0_ERL | ST0_EXL | ST0_IE) */

  /*
   * Move to kernel mode and disable interrupts, preserve interrupt mask.
   */
  .macro CLI reg
    mfc0  \reg, CP0_STATUS
    ori   \reg, \reg, STATMASK
    xori  \reg, \reg, STATMASK
    mtc0  \reg, CP0_STATUS
    ehb
  .endm

  /*
   * Copy FPU enable bit from status register to stack frame
   * clobbers reg1 and reg2
   */
  .macro COPY_FPU_STATUS_TO_TRAPFRAME fp reg0 reg1
#ifdef CONFIG_FPU
    mfc0  \reg0, CP0_STATUS
    ext   \reg0, \reg0, _ST0_CU1, 1
    REG_L \reg1, FRAME_SR(\fp)
    ins   \reg1, \reg0, _ST0_CU1, 1
    REG_S \reg1, FRAME_SR(\fp)
#endif
  .endm

  /*
   * SAVE_K_CONTEXT_IN_TRAPFRAME : Saves the relevant cpu regs and COP0 regs
   * (SR & EPC) onto the stack pointed to by fp
   * The compiler saves s0 - s8, sp and gp so as long as we 
   * don't use them, we're fine
   */
  .macro SAVE_K_CONTEXT_IN_TRAPFRAME fp
    REG_S zero, FRAME_ZERO(\fp)
    REG_S   AT, FRAME_AST(\fp)
    REG_S   v0, FRAME_V0(\fp)
    REG_S   v1, FRAME_V1(\fp)
    REG_S   a0, FRAME_A0(\fp)
    REG_S   a1, FRAME_A1(\fp)
    REG_S   a2, FRAME_A2(\fp)
    REG_S   a3, FRAME_A3(\fp)
    REG_S   t0, FRAME_T0(\fp)
    REG_S   t1, FRAME_T1(\fp)
    REG_S   t2, FRAME_T2(\fp)
    REG_S   t3, FRAME_T3(\fp)
    REG_S   t4, FRAME_T4(\fp)
    REG_S   t5, FRAME_T5(\fp)
    REG_S   t6, FRAME_T6(\fp)
    REG_S   t7, FRAME_T7(\fp)
    REG_S   t8, FRAME_T8(\fp)
    REG_S   t9, FRAME_T9(\fp)
    REG_S   ra, FRAME_RA(\fp)
#ifndef CONFIG_CPU_MIPSR6
    mflo    t0
    REG_S   t0, FRAME_MULLO(\fp)
    mfhi    t0
    REG_S   t0, FRAME_MULHI(\fp)
#endif
    mfc0    t0, CP0_STATUS
#ifdef CONFIG_FPU
    ins     t0, zero, _ST0_CU1, 1   # disable FPU in trap frame on kernel
                                    # entry: worst case, if a code path doesn't
                                    # call COPY_FPU_STATUS_TO_TRAPFRAME then
                                    # an FPU trap occurs instead of stomping
                                    # another thread's FPU registers.
#endif
    REG_S   t0, FRAME_SR(\fp)
    mfc0    t0, CP0_EPC
    REG_S   t0, FRAME_EPC(\fp)
    mfc0    t0, CP0_CAUSE
    REG_S   t0, FRAME_CAUSE(\fp)
    mfc0    t0, CP0_BADVADDR
    REG_S   t0, FRAME_BADVADDR(\fp)
  .endm

  /*
   * RESTORE_K_CONTEXT_FROM_TRAPFRAME : Restores saved cpu and COP0
   * regs from the stack pointed to by fp
   * Must be called with k0-k1, or s0-s8
   */
  .macro RESTORE_K_CONTEXT_FROM_TRAPFRAME fp
    mtc0  zero, CP0_STATUS          #zero SR just in case it has been changed
    COP0_SYNC
#ifndef CONFIG_CPU_MIPSR6
    REG_L   t0, FRAME_MULLO(\fp)
    mtlo    t0
    REG_L   t0, FRAME_MULHI(\fp)
    mthi    t0
#endif
    REG_L   t0, FRAME_EPC(\fp)
    mtc0    t0, CP0_EPC             #restore EPC and set return address
    REG_L   t0, FRAME_SR(\fp)
    mtc0    t0, CP0_STATUS          #restore SR
    
    REG_L   AT, FRAME_AST(\fp)
    REG_L   v0, FRAME_V0(\fp)
    REG_L   v1, FRAME_V1(\fp)
    REG_L   a0, FRAME_A0(\fp)
    REG_L   a1, FRAME_A1(\fp)
    REG_L   a2, FRAME_A2(\fp)
    REG_L   a3, FRAME_A3(\fp)
    REG_L   t0, FRAME_T0(\fp)
    REG_L   t1, FRAME_T1(\fp)
    REG_L   t2, FRAME_T2(\fp)
    REG_L   t3, FRAME_T3(\fp)
    REG_L   t4, FRAME_T4(\fp)
    REG_L   t5, FRAME_T5(\fp)
    REG_L   t6, FRAME_T6(\fp)
    REG_L   t7, FRAME_T7(\fp)
    REG_L   t8, FRAME_T8(\fp)
    REG_L   t9, FRAME_T9(\fp)
    REG_L   ra, FRAME_RA(\fp)
  .endm

 /*
  * SAVE_U_CONTEXT_IN_TRAPFRAME : Saves the relevant cpu regs and COP0 regs
  * (SR & EPC) onto the stack pointer at by fp
  * Do not save s0 - s8 as the compiler saves them but save gp and sp
  */
  .macro SAVE_U_CONTEXT_IN_TRAPFRAME fp
    SAVE_K_CONTEXT_IN_TRAPFRAME \fp
    REG_S   sp, FRAME_SP(\fp)
    REG_S   sp, FRAME_USP(\fp)
    REG_S   gp, FRAME_GP(\fp)
  .endm

 /*
  * RESTORE_U_CONTEXT_FROM_TRAPFRAME : Restores the relevant cpu regs and COP0 regs
  * (SR & EPC) from the stack pointed at by fp in preparation for jumping back
  * to user mode
  * Must be called with k1 or s0-s8
  */
  .macro RESTORE_U_CONTEXT_FROM_TRAPFRAME fp
    mtc0  zero, CP0_STATUS
    COP0_SYNC
    li    k0, ST0_EXL
    mtc0  k0, CP0_STATUS            # set exception level bit
    COP0_SYNC

#ifndef CONFIG_CPU_MIPSR6
    REG_L k0, FRAME_MULLO(\fp)
    mtlo  k0
    REG_L k0, FRAME_MULHI(\fp)
    mthi  k0
#endif
    REG_L k0, FRAME_EPC(\fp)
    mtc0  k0, CP0_EPC               # set return address

    REG_L AT, FRAME_AST(\fp)
    REG_L v0, FRAME_V0(\fp)
    REG_L v1, FRAME_V1(\fp)
    REG_L a0, FRAME_A0(\fp)
    REG_L a1, FRAME_A1(\fp)
    REG_L a2, FRAME_A2(\fp)
    REG_L a3, FRAME_A3(\fp)
    REG_L t0, FRAME_T0(\fp)
    REG_L t1, FRAME_T1(\fp)
    REG_L t2, FRAME_T2(\fp)
    REG_L t3, FRAME_T3(\fp)
    REG_L t4, FRAME_T4(\fp)
    REG_L t5, FRAME_T5(\fp)
    REG_L t6, FRAME_T6(\fp)
    REG_L t7, FRAME_T7(\fp)
    REG_L t8, FRAME_T8(\fp)
    REG_L t9, FRAME_T9(\fp)
    REG_L gp, FRAME_GP(\fp)
    REG_L sp, FRAME_SP(\fp)
    REG_L ra, FRAME_RA(\fp)

    REG_L k0, FRAME_SR(\fp)
    mtc0  k0, CP0_STATUS            # restore the SR
    COP0_SYNC
  .endm

  .macro RESTORE_S0_S8_FROM_TRAPFRAME fp
    REG_L s0, FRAME_S0(\fp)
    REG_L s1, FRAME_S1(\fp)
    REG_L s2, FRAME_S2(\fp)
    REG_L s3, FRAME_S3(\fp)
    REG_L s4, FRAME_S4(\fp)
    REG_L s5, FRAME_S5(\fp)
    REG_L s6, FRAME_S6(\fp)
    REG_L s7, FRAME_S7(\fp)
    REG_L s8, FRAME_S8(\fp)
  .endm

  .macro SAVE_S0_S8_IN_TRAPFRAME fp
    REG_S s0, FRAME_S0(\fp)
    REG_S s1, FRAME_S1(\fp)
    REG_S s2, FRAME_S2(\fp)
    REG_S s3, FRAME_S3(\fp)
    REG_S s4, FRAME_S4(\fp)
    REG_S s5, FRAME_S5(\fp)
    REG_S s6, FRAME_S6(\fp)
    REG_S s7, FRAME_S7(\fp)
    REG_S s8, FRAME_S8(\fp)
  .endm

/*
 *** Root Exception Handlers ***
 */

/*
 * Root General Exception Handler
 */
  .globl  _root_gen_exceptionhandler
  .globl  _root_gen_exceptionhandler_exit .text
  .type   _root_gen_exceptionhandler, function
  .ent    _root_gen_exceptionhandler
_root_gen_exceptionhandler:

  /*
   * Reserve space for the trap frame, set up the call frame args,
   * disable interrupts/exceptions and jump to the C handler
   * On returning from the C exception handler, restore all
   * regs and enable interrupts/exceptions. The EPC will have been
   * updated by the C handler if appropriate.
   * Note : Need to allocate a callframe for any code that calls a
   * C function that could potentially call other functions
   */
  addu  sp, sp, -VAL__SIZEOF_TRAP_STATE
  SAVE_K_CONTEXT_IN_TRAPFRAME sp
  move  a0, sp                          # 1st arg is trap state pointer
  mfc0  a1, CP0_PRID,1                  # 2nd arg is CPU
  and   a1, a1, 0x03FF
  li    a2, 0                           # 3rd arg indicates whether raised from guest context
  CLI   t0
  jal   thread_handle_trap
  addu  sp, sp, -CALLFRAME_SIZ          # use branch delay - allocate standard callframe
  addu  sp, sp, CALLFRAME_SIZ

_root_gen_exceptionhandler_exit:
  j     _root_exceptionhandler_exit
  nop

  .end _root_gen_exceptionhandler


/*
 * Root Intr Handler
 */
  .globl  _root_intr_exceptionhandler
  .globl  _root_intr_exceptionhandler_exit .text
  .type   _root_intr_exceptionhandler, function
  .ent    _root_intr_exceptionhandler
_root_intr_exceptionhandler:

  /*
   * Reserve space for the trap frame, set up the call frame args,
   * disable interrupts/exceptions and jump to the C handler
   * On returning from the C exception handler, restore all
   * regs and enable interrupts/exceptions. The EPC will have been
   * updated by the C handler if appropriate.
   * Note : Need to allocate a callframe for any code that calls a
   * C function that could potentially call other functions
   */
  addu  sp, sp, -VAL__SIZEOF_TRAP_STATE
  SAVE_K_CONTEXT_IN_TRAPFRAME sp
  mfc0  a0, CP0_STATUS                  # 1st arg is STATUS
  mfc0  a1, CP0_CAUSE                   # 2nd arg is CAUSE
  li    a2, 0                           # 3rd arg indicates whether raised by guest context
  CLI   t0
  jal   irq_handler
  addu  sp, sp, -CALLFRAME_SIZ          # use branch delay - allocate standard callframe
  addu  sp, sp, CALLFRAME_SIZ

_root_intr_exceptionhandler_exit:
  j     _root_exceptionhandler_exit
  nop

  .end _root_intr_exceptionhandler


/*
 * Root TLB Exception Handler
 */
  .globl  _root_tlb_exceptionhandler
  .globl  _root_tlb_exceptionhandler_exit .text
  .type   _root_tlb_exceptionhandler, function
  .ent    _root_tlb_exceptionhandler
_root_tlb_exceptionhandler:

  /*
   * Reserve space for the trap frame, set up the call frame args,
   * disable interrupts/exceptions and jump to the C handler
   * On returning from the C exception handler, restore all
   * regs and enable interrupts/exceptions. The EPC will have been
   * updated by the C handler if appropriate.
   * Note : Need to allocate a callframe for any code that calls a
   * C function that could potentially call other functions
   */
  addu  sp, sp, -VAL__SIZEOF_TRAP_STATE
  SAVE_K_CONTEXT_IN_TRAPFRAME sp
  move  a0, sp                          # 1st arg is trap state pointer
  mfc0  a1, CP0_PRID,1                  # 2nd arg is CPU
  and   a1, a1, 0x03FF
  li    a2, 0                           # 3rd arg indicates whether raised by guest context
  CLI   t0
  jal   handle_tlb_exceptions
  addu  sp, sp, -CALLFRAME_SIZ          # use branch delay - allocate standard callframe
  addu  sp, sp, CALLFRAME_SIZ

_root_tlb_exceptionhandler_exit:
  j     _root_exceptionhandler_exit
  nop

  .end _root_tlb_exceptionhandler


/*
 * Exit point for Root Exception Handler
 */
  .globl  _root_exceptionhandler_exit
  .type   _root_exceptionhandler_exit, function
  .ent    _root_exceptionhandler_exit
_root_exceptionhandler_exit:

  /* Return from handler. Make sure interrupts are disabled */
  di
  ehb

  COPY_FPU_STATUS_TO_TRAPFRAME sp t0 t1
  RESTORE_K_CONTEXT_FROM_TRAPFRAME sp
  addu  sp, sp, VAL__SIZEOF_TRAP_STATE  # restore kernel stack pointer
  eret                                  # return to interrupted point

  .end _root_exceptionhandler_exit


/*
 *** User Mode Exception Handlers ***
 */

/*
 * Entry point to user mode task
 * On entry:
 * k1: trap frame containing entry conditions
 * FRAME_USP: contains the user thread sp set by the kernel
 */
  .globl  user_invoke_entrypoint
  .type   user_invoke_entrypoint, function
  .ent    user_invoke_entrypoint
user_invoke_entrypoint:
  COPY_FPU_STATUS_TO_TRAPFRAME k1 t0 t1
  RESTORE_U_CONTEXT_FROM_TRAPFRAME k1
  REG_L sp, FRAME_USP(k1)               # restore sp from trap frame usp
  eret                                  # return to interrupted point
  .end user_invoke_entrypoint

/*
 * User Coprocessor Unusable Exception Handler
 * Fast path handler which only uses k0, k1 when handling read_c0_ddatalo user
 * exception.
 */
  .globl  _user_copu_exceptionhandler
  .globl  _user_copu_exceptionhandler_exit .text
  .type   _user_copu_exceptionhandler, function
  .ent    _user_copu_exceptionhandler
_user_copu_exceptionhandler:
  /* skip if in BD slot */
  mfc0    k0,CP0_CAUSE
  bgez    k0,2f
  nop
1:
  la      k0, _user_gen_exceptionhandler
  j       k0
  nop

  /* skip if badinstr register not implemented */
2:
  mfc0    k0,CP0_CONFIG,3
  ext     k0,k0,0x1a,0x1
  beqz    k0,1b
  nop

  /* check whether this is mfc0 CP0_DATALO1 instruction */
  mfc0    k0,CP0_BADINSTR
  la      k1,_user_copu_read_c0_ddatalo
  REG_L   k1,0(k1)
  xor     k0,k0,k1
  lui     k1,0xffe0
  ori     k1,k1,0xffff
  and     k0,k0,k1
  bne     k0,zero,1b
  nop

  /* extract rt register and index into table to set rt <- CP0_DATALO1 */
  mfc0    k0,CP0_BADINSTR
  ext     k0,k0,0x10,0x5
  sll     k0,k0,0x3                     # index into table
  la      k1,_user_copu_setrt_table
  addu    k0,k0,k1

_user_copu_read_c0_ddatalo:
  mfc0    k1,CP0_DATALO1
  j       k0
  nop

_user_copu_setrt_table:
  b       3f
  nop     # move    zero,k1
  b       3f
  move    AT,k1
  b       3f
  move    v0,k1
  b       3f
  move    v1,k1
  b       3f
  move    a0,k1
  b       3f
  move    a1,k1
  b       3f
  move    a2,k1
  b       3f
  move    a3,k1
  b       3f
  move    t0,k1
  b       3f
  move    t1,k1
  b       3f
  move    t2,k1
  b       3f
  move    t3,k1
  b       3f
  move    t4,k1
  b       3f
  move    t5,k1
  b       3f
  move    t6,k1
  b       3f
  move    t7,k1
  b       3f
  move    s0,k1
  b       3f
  move    s1,k1
  b       3f
  move    s2,k1
  b       3f
  move    s3,k1
  b       3f
  move    s4,k1
  b       3f
  move    s5,k1
  b       3f
  move    s6,k1
  b       3f
  move    s7,k1
  b       3f
  move    t8,k1
  b       3f
  move    t9,k1
  b       3f
  move    k0,k1
  b       3f
  move    k1,k1
  b       3f
  move    gp,k1
  b       3f
  move    sp,k1
  b       3f
  move    s8,k1
  b       3f
  move    ra,k1
  b       3f

3:
_user_copu_exceptionhandler_exit:
  mfc0    k1, CP0_EPC
  addiu   k1, k1, 4                     # assumption: user mfc0 CP0_DATALO1 not in BD slot
  mtc0    k1, CP0_EPC
  COP0_SYNC
  eret                                  # return to interrupted point

  .end _user_copu_exceptionhandler

/*
 * User GenException Handler
 */
  .globl  _user_gen_exceptionhandler
  .globl  _user_gen_exceptionhandler_exit .text
  .type   _user_gen_exceptionhandler, function
  .ent    _user_gen_exceptionhandler
_user_gen_exceptionhandler:

  /*
   * Get the saved kernel stack pointer from ErrorEPC and allocate
   * a trap_state object and use it to save context including the
   * current sp.
   * Set up the callframe args and reserve callframe space
   */
  mfc0  k1, CP0_ERROREPC                # switch to kernel stack
  addu  k1, k1, -VAL__SIZEOF_TRAP_STATE
  SAVE_U_CONTEXT_IN_TRAPFRAME k1
  move  a0, k1                          # 1st arg is trap state pointer
  mfc0  a1, CP0_PRID,1                  # 2nd arg is CPU
  and   a1, a1, 0x03FF
  li    a2, 0                           # 3rd arg indicates whether raised from guest context

  /*
   * Switch to kernel mode and jump to the C handler
   */
  .set  at
  mfc0  t0, CP0_STATUS
#ifdef CONFIG_FPU
  and   t0, t0, ~(ST0_EXL | ST0_IE | ST0_KSU)
#else
  and   t0, t0, ~(ST0_CU1 | ST0_EXL | ST0_IE | ST0_KSU)
#endif
  mtc0  t0, CP0_STATUS
  .set  noat
  COP0_SYNC
  jal   thread_handle_trap
  addu  sp, k1, -CALLFRAME_SIZ          # branch delay slot - allocate callframe

  /*
   * Return from handler. Make sure interrupts are disabled
   */
  di
  ehb

_user_gen_exceptionhandler_exit:
  /*
   * On return, restore context and return to the interrupted point
   */
  addu  k1, sp, CALLFRAME_SIZ
  COPY_FPU_STATUS_TO_TRAPFRAME k1 t0 t1
  RESTORE_U_CONTEXT_FROM_TRAPFRAME k1
  eret                                  # return to interrupted point

  .end _user_gen_exceptionhandler


/*
 * User IntrException Handler
 */
  .globl  _user_intr_exceptionhandler
  .globl  _user_intr_exceptionhandler_exit .text
  .type   _user_intr_exceptionhandler, function
  .ent    _user_intr_exceptionhandler
_user_intr_exceptionhandler:
  /*
   * Get the saved kernel stack pointer from ErrorEPC and allocate
   * a trap_state object and use it to save context including the
   * current sp.
   * Set up the callframe args and reserve callframe space
   */
  mfc0  k1, CP0_ERROREPC                # switch to kernel stack
  addu  k1, k1, -VAL__SIZEOF_TRAP_STATE
  SAVE_U_CONTEXT_IN_TRAPFRAME k1
  mfc0  a0, CP0_STATUS                  # 1st arg is STATUS
  mfc0  a1, CP0_CAUSE                   # 2nd arg is CAUSE
  li    a2, 0                           # 3rd arg indicates whether raised from guest context

  /*
   * Switch to kernel mode and jump to the C handler
   */
  .set  at
  mfc0  t0, CP0_STATUS
#ifdef CONFIG_FPU
  and   t0, t0, ~(ST0_EXL | ST0_IE | ST0_KSU)
#else
  and   t0, t0, ~(ST0_CU1 | ST0_EXL | ST0_IE | ST0_KSU)
#endif
  mtc0  t0, CP0_STATUS
  .set  noat
  COP0_SYNC
  jal   irq_handler
  addu  sp, k1, -CALLFRAME_SIZ          # switch to kernel SP

  /*
   * Return from handler. Make sure interrupts are disabled
   */
  di
  ehb

_user_intr_exceptionhandler_exit:
  /*
   * On return, restore context and return to the interrupted point
   */
  addu  k1, sp, CALLFRAME_SIZ
  COPY_FPU_STATUS_TO_TRAPFRAME k1 t0 t1
  RESTORE_U_CONTEXT_FROM_TRAPFRAME k1
  eret                                  # return to interrupted point

  .end _user_intr_exceptionhandler

/*
 * reset the Thread_cancel flag
 * _thread and _scr are scratched, _thread contains the thread state
 */
  .macro RESET_THREAD_CANCEL _thread _scr
  REG_L \_scr, OFS__THREAD__STATE (\_thread)
  ins   \_scr, zero, 8, 1 # Clear Thread_cancel 0x100 // KYMAXXX this doesn't match thread_state.cpp 0x80
  REG_S \_scr, OFS__THREAD__STATE (\_thread)
  .endm

  /* _ksp must be within the TCB i.e. not at the upper limit of the kernel
   * stack */
  .macro KERNEL_SP_TO_TCB _ksp _thread
  li    \_thread, ~(THREAD_BLOCK_SIZE - 1)
  and   \_thread, \_thread, \_ksp
  .endm

  /* Callers depend on only _tcb register being modified */
  .macro THREAD_TCB _tcb
  mfc0  \_tcb, CP0_ERROREPC             # get ksp
  addiu \_tcb, -4                       # make sure it is not at upper limit
  ori   \_tcb, (THREAD_BLOCK_SIZE - 1)
  xori  \_tcb, (THREAD_BLOCK_SIZE - 1)
  .endm

  /* Callers depend on only _reg register being modified */
  .macro THREAD_VCPU _reg
  THREAD_TCB \_reg
  REG_L \_reg, OFS__THREAD__VCPU_STATE(\_reg)
  .endm

/*
 * System Call Handler
 */
  .globl  _user_syscall_exceptionhandler
  .globl  _user_syscall_exceptionhandler_exit .text
  .type   _user_syscall_exceptionhandler, function
  .ent    _user_syscall_exceptionhandler
_user_syscall_exceptionhandler:

  /*
   * Get the saved kernel stack pointer from ErrorEPC and allocate
   * a trap_state object and use it to save context including the
   * current sp.
   * Set up the callframe args and reserve callframe space
   */
  mfc0  k1, CP0_ERROREPC                # switch to kernel stack
  addu  k1, k1, -VAL__SIZEOF_TRAP_STATE
  SAVE_U_CONTEXT_IN_TRAPFRAME k1
  SAVE_S0_S8_IN_TRAPFRAME k1            # save syscall parameters

  la    t9, syscall_table
  li    t0, 0x1c                        # limit syscall offset to syscall_table size (0x7*4)
  and   s5, s5, t0                      # s5 contains syscall number
  addu  t9, t9, s5                      # use syscall as offset into table
  REG_L t9, 0(t9)                       # load syscall handler address

  KERNEL_SP_TO_TCB    k1 t0             # get tcb context and reset thread cancel state
  RESET_THREAD_CANCEL t0 t1

  /*
   * Switch to kernel mode and jump to the C handler
   */
  .set  at
  mfc0  t0, CP0_STATUS
#ifdef CONFIG_FPU
  and   t0, t0, ~(ST0_EXL | ST0_IE | ST0_KSU)
#else
  and   t0, t0, ~(ST0_CU1 | ST0_EXL | ST0_IE | ST0_KSU)
#endif
  mtc0  t0, CP0_STATUS
  .set  noat
  COP0_SYNC

  jalr  t9
  addu  sp, k1, -CALLFRAME_SIZ          # branch delay slot - allocate callframe

  /*
   * Return from handler. Make sure interrupts are disabled
   */
  di
  ehb

_user_syscall_exceptionhandler_exit:
  addu  k1, sp, CALLFRAME_SIZ

  /* Figure out if we are returning to Kernel context, due to a continuation
   * happens in Thread:kill()
   */
  REG_L k0, FRAME_SR(k1)
  andi  k0, k0, KSU_USER
  bnez  k0, 1f
  nop

_user_syscall_exceptionhandler_exit_to_kernel:
  /* Returning to kernel context, set the EPC, STATUS and ERET */
  REG_L k0, FRAME_EPC(k1)
  mtc0  k0, CP0_EPC

  COPY_FPU_STATUS_TO_TRAPFRAME k1 t0 t1 # stomp t0/t1
  REG_L k0, FRAME_SR(k1)
  mtc0  k0, CP0_STATUS
  COP0_SYNC

  addu  sp, k1, -CALLFRAME_SIZ
  eret
  nop

  .globl  fast_ret_from_irq
fast_ret_from_irq:
  di
  ehb
  addu  k1, sp, -4                      # skip addr of fast_ret_from_irq on sp
                                        # fall through to 1: to restore context

1:
_user_syscall_exceptionhandler_exit_to_user:
  COPY_FPU_STATUS_TO_TRAPFRAME k1 t0 t1
  RESTORE_U_CONTEXT_FROM_TRAPFRAME k1
  RESTORE_S0_S8_FROM_TRAPFRAME k1       # restore syscall parameters

  mfc0  k1, CP0_EPC
  addiu k1, k1, 4                       # assumption: syscall is not in BD slot
  mtc0  k1, CP0_EPC                     # increment past syscall instruction
  COP0_SYNC
  eret                                  # return to interrupted point

  .end _user_syscall_exceptionhandler


/*
 * restore saved vcpu state
 * must be called with interrupts disabled
 * On entry:
 * a0: trap frame
 */
LEAF(vcpu_resume)
  move  k1, a0                          # move trap frame to k1
  COPY_FPU_STATUS_TO_TRAPFRAME k1 t0 t1
  RESTORE_U_CONTEXT_FROM_TRAPFRAME k1
  RESTORE_S0_S8_FROM_TRAPFRAME k1
  eret                                  # return to interrupted point
END(vcpu_resume)

/*
 * fast return to user space without restoring user state
 * On entry:
 * k1: trap frame to restore
 * a0: arg, vcpu state (user space addr)
 * FRAME_USP: contains the vcpu sp set by the kernel
 */
LEAF(fast_vcpu_resume)
  COPY_FPU_STATUS_TO_TRAPFRAME k1 t0 t1 # stomp t0/t1
  REG_L sp, FRAME_USP(k1)  # restore sp from trap frame usp
  REG_L k0, FRAME_SR(k1)
  mtc0  k0, CP0_STATUS     # restore the SR
  REG_L k0, FRAME_EPC(k1)
  mtc0  k0, CP0_EPC        # set return address
  COP0_SYNC
  eret                     # return to interrupted point
END(fast_vcpu_resume)

LEAF(leave_by_trigger_exception)
  // makes use of Trap_state::L4_trigger_exception and enters slowtrap
  break // Kyma: Not implemented - leave_by_trigger_exception
  j ra
  nop
END(leave_by_trigger_exception)

LEAF(leave_by_vcpu_upcall)
  mfc0  k1, CP0_ERROREPC                # switch to kernel stack
  addu  k1, k1, -VAL__SIZEOF_TRAP_STATE # k1 = kernel stack trap frame

  /* save vcpu state */
  THREAD_VCPU k0                        # k0 = vcpu_state->ts
  SAVE_U_CONTEXT_IN_TRAPFRAME k0

  /* now that state is saved, save current exception to vcpu and return to upcall handler */

  THREAD_TCB t1                         # t1 = tcb context

  /* t1 = current() tcb context */
  /* k0 = addr of vcpu_state trap frame */
  /* k1 = addr of kernel stack trap frame */

  /* restore exception state from exception preceeding the upcall */
  REG_L t0, OFS__THREAD__EXCEPTION_IP(t1)
  REG_S t0, FRAME_EPC(k0)
  li    t0, ~0
  REG_S t0, OFS__THREAD__EXCEPTION_IP(t1)

  REG_L t0, OFS__THREAD__EXCEPTION_SR(t1)
  REG_S t0, FRAME_SR(k0)

  .set  at
  or    t0, t0, ST0_IE
  .set  noat
  REG_S t0, FRAME_SR(k1)

  /* copy return state from kernel trap frame to vcpu_state trap frame */
  REG_L t0, FRAME_USP(k1)
  REG_S t0, FRAME_USP(k0)

  /* set return sp to upcall entry sp saved in vcpu state */
  REG_L t0, OFS__VCPU_STATE__ENTRY_SP(k0)
  REG_S t0, FRAME_USP(k1)

  /* set return epc to upcall entry ip saved in vcpu state */
  REG_L t0, OFS__VCPU_STATE__ENTRY_IP(k0)
  REG_S t0, FRAME_EPC(k1)

  /* k1 = addr of kernel stack trap frame */
  b     fast_vcpu_resume
  nop
END(leave_by_vcpu_upcall)


/*
 *** Guest Kernel Exception Handlers ***
 */

/*
 * Guest General Exception Handler
 */
  .globl  _guest_gen_exceptionhandler
  .type   _guest_gen_exceptionhandler, function
  .ent    _guest_gen_exceptionhandler
_guest_gen_exceptionhandler:
  j     _guest_exceptionhandler
  nop
  .end _guest_gen_exceptionhandler

/*
 * Guest Intr Handler
 */
  .globl  _guest_intr_exceptionhandler
  .type   _guest_intr_exceptionhandler, function
  .ent    _guest_intr_exceptionhandler
_guest_intr_exceptionhandler:
  j     _guest_exceptionhandler
  nop
  .end _guest_intr_exceptionhandler

/*
 * Guest TLB Exception Handler
 */
  .globl  _guest_tlb_exceptionhandler
  .type   _guest_tlb_exceptionhandler, function
  .ent    _guest_tlb_exceptionhandler
_guest_tlb_exceptionhandler:
  j     _guest_exceptionhandler
  nop
  .end _guest_tlb_exceptionhandler

/*
 * Generic Guest Exception Handler
 */
  .globl  _guest_exceptionhandler
  .globl  _guest_exceptionhandler_exit .text
  .type   _guest_exceptionhandler, function
  .ent    _guest_exceptionhandler
_guest_exceptionhandler:
  /*
   * Reserve space for the trap frame, set up the call frame args,
   * disable interrupts/exceptions and jump to the C handler
   * On returning from the C exception handler, restore all
   * regs and enable interrupts/exceptions. The EPC will have been
   * updated by the C handler if appropriate.
   * Note : Need to allocate a callframe for any code that calls a
   * C function that could potentially call other functions
   */
  THREAD_VCPU k1                        # k1 = vcpu_state->ts

  /* save vz guest state */
  SAVE_U_CONTEXT_IN_TRAPFRAME k1
  SAVE_S0_S8_IN_TRAPFRAME k1

  /* save vz guest k0/k1 registers */
  mfc0  t0, CP0_KSCRATCH1
  mfc0  t1, CP0_KSCRATCH2
  REG_S t0, FRAME_K0(k1)                # save guest k0
  REG_S t1, FRAME_K1(k1)                # save guest k1

#ifdef CONFIG_MIPS_VZ
  /* Clear GM bit to avoid switching to VZ guest context when EXL is cleared */
  mfc0  t0, CP0_GUESTCTL0
  ins   t0, zero, GUESTCTL0_GM_SHIFT, 1
  mtc0  t0, CP0_GUESTCTL0

  /* check GuestCtl0.G1 */
  ext   t0, t0, GUESTCTL0_G1_SHIFT, 1
  beq   t0, zero, 1f    /* no GuestCtl1 register */
  nop

  /* see ClearGuestRID. Handles both GuestCtl0.DRG mode enabled or
   * disabled */
  mfc0  t0, CP0_GUESTCTL1
  addiu t1, zero, GUESTCTL1_VZ_ROOT_GUESTID
  ins   t0, t1, GUESTCTL1_RID_SHIFT, GUESTCTL1_RID_WIDTH
  mtc0  t0, CP0_GUESTCTL1 /* Set GuestCtl1.RID = GUESTCTL1_VZ_ROOT_GUESTID */
  ehb
1:
#endif

  .globl _resume_to_host .text
_resume_to_host:
  /* restore and unwind kernel call stack */
  THREAD_TCB k0
  REG_L k1, OFS__THREAD__SAVED_KSP(k0)  # restore ksp
  li    t0, ~0
  REG_S t0, OFS__THREAD__SAVED_KSP(k0)  # invalidate saved ksp

  COPY_FPU_STATUS_TO_TRAPFRAME k1 t0 t1
  RESTORE_U_CONTEXT_FROM_TRAPFRAME k1   # our convention is to leave ksp before ts frame
  RESTORE_S0_S8_FROM_TRAPFRAME k1

  .globl _resume_to_host_exit .text
_resume_to_host_exit:
_guest_exceptionhandler_exit:
  jr    ra
  nop

  .end _guest_exceptionhandler


/*
 * Cause Register ExcCode Exception Handler Table
 */
_exceptionhandler_base:
    /*
     * Handlers to handle the root(hypervisor) generated exceptions
     */
    .word _root_intr_exceptionhandler     #  0 external interrupt
    .word _root_tlb_exceptionhandler      #  1 TLB modification
    .word _root_tlb_exceptionhandler      #  2 TLB miss (Load/I-fetch)
    .word _root_tlb_exceptionhandler      #  3 TLB miss (Store)
    .word _root_gen_exceptionhandler      #  4 address error (Load/I-fetch)
    .word _root_gen_exceptionhandler      #  5 address error (Store)
    .word _root_gen_exceptionhandler      #  6 bus error (I-fetch)
    .word _root_gen_exceptionhandler      #  7 bus error (data reference : load or store)
    .word _root_gen_exceptionhandler      #  8 system call
    .word _root_gen_exceptionhandler      #  9 breakpoint
    .word _root_gen_exceptionhandler      # 10 reserved instruction
    .word _root_gen_exceptionhandler      # 11 coprocessor unusable
    .word _root_gen_exceptionhandler      # 12 arithmetic overflow
    .word _root_gen_exceptionhandler      # 13 trap exception
    .word _root_gen_exceptionhandler      # 14 reserved
    .word _root_gen_exceptionhandler      # 15 FP exception
    .word _root_gen_exceptionhandler      # 16 34K :: timer interrupt
    .word _root_gen_exceptionhandler      # 17 reserved
    .word _root_gen_exceptionhandler      # 18 reserved for precise Coprocessor 2 exceptions
    .word _root_gen_exceptionhandler      # 19 reserved
    .word _root_gen_exceptionhandler      # 20 reserved
    .word _root_gen_exceptionhandler      # 21 reserved
    .word _root_gen_exceptionhandler      # 22 reserved
    .word _root_gen_exceptionhandler      # 23 watch exception
    .word _root_gen_exceptionhandler      # 24 reserved
    .word _root_gen_exceptionhandler      # 25 reserved
    .word _root_gen_exceptionhandler      # 26 reserved
    .word _root_gen_exceptionhandler      # 27 hypercall (defined by VZ ASE)
    .word _root_gen_exceptionhandler      # 28 reserved
    .word _root_gen_exceptionhandler      # 29 reserved
    .word _root_gen_exceptionhandler      # 30 reserved
    .word _root_gen_exceptionhandler      # 31 reserved


    /*
     * Handlers to handle the user generated exceptions
     */
    .word _user_intr_exceptionhandler     #  0 External interrupt
    .word _user_gen_exceptionhandler      #  1 TLB modification
    .word _user_gen_exceptionhandler      #  2 TLB miss (Load/I-fetch)
    .word _user_gen_exceptionhandler      #  3 TLB miss (Store)
    .word _user_gen_exceptionhandler      #  4 address error (Load/I-fetch)
    .word _user_gen_exceptionhandler      #  5 address error (Store)
    .word _user_gen_exceptionhandler      #  6 bus error (I-fetch)
    .word _user_gen_exceptionhandler      #  7 bus error (data reference : load or store)
    .word _user_syscall_exceptionhandler  #  8 system call
    .word _user_gen_exceptionhandler      #  9 breakpoint
    .word _user_gen_exceptionhandler      # 10 reserved instruction
    .word _user_copu_exceptionhandler      # 11 coprocessor unusable
    .word _user_gen_exceptionhandler      # 12 arithmetic overflow
    .word _user_gen_exceptionhandler      # 13 trap exception
    .word _user_gen_exceptionhandler      # 14 reserved
    .word _user_gen_exceptionhandler      # 15 FP exception
    .word _user_gen_exceptionhandler      # 16 34K :: timer interrupt
    .word _user_gen_exceptionhandler      # 17 reserved
    .word _user_gen_exceptionhandler      # 18 reserved for precise Coprocessor 2 exceptions
    .word _user_gen_exceptionhandler      # 19 reserved
    .word _user_gen_exceptionhandler      # 20 reserved
    .word _user_gen_exceptionhandler      # 21 reserved
    .word _user_gen_exceptionhandler      # 22 reserved
    .word _user_gen_exceptionhandler      # 23 watch exception
    .word _user_gen_exceptionhandler      # 24 reserved
    .word _user_gen_exceptionhandler      # 25 reserved
    .word _user_gen_exceptionhandler      # 26 reserved
    .word _user_gen_exceptionhandler      # 27 hypercall (defined by VZ ASE)
    .word _user_gen_exceptionhandler      # 28 reserved
    .word _user_gen_exceptionhandler      # 29 reserved
    .word _user_gen_exceptionhandler      # 30 reserved
    .word _user_gen_exceptionhandler      # 31 reserved


    /*
     * Handlers to handle the guest kernel generated exceptions
     */
    .word _guest_intr_exceptionhandler    #  0 external interrupt
    .word _guest_tlb_exceptionhandler     #  1 TLB modification
    .word _guest_tlb_exceptionhandler     #  2 TLB miss (Load/I-fetch)
    .word _guest_tlb_exceptionhandler     #  3 TLB miss (Store)
    .word _guest_gen_exceptionhandler     #  4 address error (Load/I-fetch)
    .word _guest_gen_exceptionhandler     #  5 address error (Store)
    .word _guest_gen_exceptionhandler     #  6 bus error (I-fetch)
    .word _guest_gen_exceptionhandler     #  7 bus error (data reference : load or store)
    .word _guest_gen_exceptionhandler     #  8 system call
    .word _guest_gen_exceptionhandler     #  9 breakpoint
    .word _guest_gen_exceptionhandler     # 10 reserved instruction
    .word _guest_gen_exceptionhandler     # 11 coprocessor unusable
    .word _guest_gen_exceptionhandler     # 12 arithmetic overflow
    .word _guest_gen_exceptionhandler     # 13 trap exception
    .word _guest_gen_exceptionhandler     # 14 reserved
    .word _guest_gen_exceptionhandler     # 15 FP exception
    .word _guest_gen_exceptionhandler     # 16 reserved
    .word _guest_gen_exceptionhandler     # 17 reserved
    .word _guest_gen_exceptionhandler     # 18 reserved for precise Coprocessor 2 exceptions
    .word _guest_gen_exceptionhandler     # 19 reserved
    .word _guest_gen_exceptionhandler     # 20 reserved
    .word _guest_gen_exceptionhandler     # 21 reserved
    .word _guest_gen_exceptionhandler     # 22 reserved
    .word _guest_gen_exceptionhandler     # 23 watch exception
    .word _guest_gen_exceptionhandler     # 24 reserved
    .word _guest_gen_exceptionhandler     # 25 reserved
    .word _guest_gen_exceptionhandler     # 26 reserved
    .word _guest_gen_exceptionhandler     # 27 hypercall (defined by VZ ASE)
    .word _guest_gen_exceptionhandler     # 28 reserved
    .word _guest_gen_exceptionhandler     # 29 reserved
    .word _guest_gen_exceptionhandler     # 30 reserved
    .word _guest_gen_exceptionhandler     # 31 reserved


/*
 * resume_vm_vz: entry point to the guest
 * must be called with interrupts disabled
 * On entry:
 * a0: vcpu
 * a1: vm_state
 */
  .globl  resume_vm_vz
  .type   resume_vm_vz, function
  .ent    resume_vm_vz
resume_vm_vz:

  /* save host context at current ksp */
  move  k1, sp
  addu  k1, k1, -VAL__SIZEOF_TRAP_STATE
  SAVE_U_CONTEXT_IN_TRAPFRAME k1
  SAVE_S0_S8_IN_TRAPFRAME k1

  /* stash ksp for resuming host state in _saved_ksp */
  THREAD_TCB k0
  REG_S k1, OFS__THREAD__SAVED_KSP(k0)  # our convention is to leave ksp before ts frame

  b     resume_to_vz_guest              # arguements: a0, a1
  nop
  .end resume_vm_vz

/*
 * resume_to_vz_guest: restore guest context and return to guest
 * must be called with interrupts disabled
 * a0: vcpu
 * a1: vm_state
 */
  .globl  resume_to_vz_guest
  .globl  resume_to_vz_guest_exit .text
  .type   resume_to_vz_guest, function
  .ent    resume_to_vz_guest
resume_to_vz_guest:

  /* restore guest context from saved trap frame */
  move  k1, a0                          # a0 = vcpu_state->ts
  COPY_FPU_STATUS_TO_TRAPFRAME k1 t0 t1
  RESTORE_U_CONTEXT_FROM_TRAPFRAME k1

  /* only touch s0-s8, k0 from this point */

#ifdef CONFIG_MIPS_VZ
  /* Set GM bit to setup eret to VZ guest context */
  mfc0  k0, CP0_GUESTCTL0
  addiu s0, zero, 1
  ins   k0, s0, GUESTCTL0_GM_SHIFT, 1
  mtc0  k0, CP0_GUESTCTL0

  /* check GuestCtl0.G1 */
  ext   k0, k0, GUESTCTL0_G1_SHIFT, 1
  beq   k0, zero, 1f    /* no GuestCtl1 register */
  nop

  /* see SetGuestRIDtoGuestID. Handles both GuestCtl0.DRG mode enabled or
   * disabled */
  mfc0  k0, CP0_GUESTCTL1 /* Get current GuestID */
  ext   s0, k0, GUESTCTL1_ID_SHIFT, GUESTCTL1_ID_WIDTH
  ins   k0, s0, GUESTCTL1_RID_SHIFT, GUESTCTL1_RID_WIDTH
  mtc0  k0, CP0_GUESTCTL1 /* Set GuestCtl1.RID = GuestCtl1.ID */
  ehb
1:
#endif

  RESTORE_S0_S8_FROM_TRAPFRAME k1

  /* Restore the guest's k0/k1 registers */
  REG_L k0, FRAME_K0(k1)                # restore guest k0
  REG_L k1, FRAME_K1(k1)                # restore guest k1

resume_to_vz_guest_exit:
  eret                                  # return to guest
  .end resume_to_vz_guest
