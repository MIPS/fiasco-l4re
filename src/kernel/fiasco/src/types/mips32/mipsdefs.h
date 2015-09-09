/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef _MIPSDEFS_H_
#define _MIPSDEFS_H_

/*
 * Location of MIPS Exception Vectors and Offsets
 */
#define MIPS_UTLBMISS_EXC_VECTOR_OFFSET 0x0 
#define MIPS_CACHEERR_EXC_VECTOR_OFFSET 0x100
#define MIPS_GEN_EXC_VECTOR_OFFSET      0x180
#define MIPS_INTR_EXC_VECTOR_OFFSET     0x200

#define MIPS_RESET_EXC_VECTOR       0xBFC00000
#define MIPS_DEFAULT_EBASE          0x80000000
#define MIPS_UTLB_MISS_EXC_VECTOR   (MIPS_DEFAULT_EBASE + MIPS_UTLBMISS_EXC_VECTOR_OFFSET) 
#define MIPS_CACHEERR_EXC_VECTOR    (MIPS_DEFAULT_EBASE + MIPS_CACHEERR_EXC_VECTOR_OFFSET) 
#define MIPS_GENERAL_EXC_VECTOR     (MIPS_DEFAULT_EBASE + MIPS_GEN_EXC_VECTOR_OFFSET) 
#define MIPS_INTR_EXC_VECTOR        (MIPS_DEFAULT_EBASE + MIPS_INTR_EXC_VECTOR_OFFSET)

/* 
 * Call Frame Definitions
 *
 * standard callframe {
 *  register_t cf_args[4];    arg0 - arg3
 *  register_t cf_sp;   frame pointer
 *  register_t cf_ra;   return address
 * };
 */
#define CALLFRAME_SIZ (4 * (4 + 2))
#define CALLFRAME_SP  (4 * 4)
#define CALLFRAME_RA  (4 * 5)

/*
 * Trap Frame register definitions
 * - keep synced with mips32/trap_state.cpp and mips32/entry_frame-mips32.cpp
 *
 * Trap_state = Trap_state_regs + Syscall_frame + Return_frame
 * Entry_frame = Syscall_frame + Return_frame
 */

/*
 * Trap_frame register indices
 */
#define FRAME_REG_PFA 0       // Matches Trap_state_regs
#define FRAME_REG_ERR 1       // Matches Trap_state_regs
#define FRAME_REG_ZERO 2      // Matches Syscall_frame
#define FRAME_REG_AST 3       // ..
#define FRAME_REG_V0 4

#define FRAME_REG_V1 5
#define FRAME_REG_A0 6
#define FRAME_REG_A1 7
#define FRAME_REG_A2 8
#define FRAME_REG_A3 9

#define FRAME_REG_T0 10
#define FRAME_REG_T1 11
#define FRAME_REG_T2 12
#define FRAME_REG_T3 13
#define FRAME_REG_T4 14
#define FRAME_REG_T5 15
#define FRAME_REG_T6 16
#define FRAME_REG_T7 17

#define FRAME_REG_S0 18
#define FRAME_REG_S1 19
#define FRAME_REG_S2 20
#define FRAME_REG_S3 21
#define FRAME_REG_S4 22
#define FRAME_REG_S5 23
#define FRAME_REG_S6 24
#define FRAME_REG_S7 25
#define FRAME_REG_T8 26
#define FRAME_REG_T9 27

#define FRAME_REG_K0 28
#define FRAME_REG_K1 29
#define FRAME_REG_GP 30
#define FRAME_REG_SP 31
#define FRAME_REG_S8 32       // ..
#define FRAME_REG_RA 33       // Matches Syscall_frame

#define FRAME_REG_MULLO 34    // Matches Return_frame
#define FRAME_REG_MULHI 35    // Matches Return_frame
#define FRAME_REG_SR 36       // Matches Return_frame
#define FRAME_REG_EPC 37      // Matches Return_frame
#define FRAME_REG_CAUSE 38    // Matches Return_frame
#define FRAME_REG_BADVADDR 39 // Matches Return_frame

#define FRAME_REG_USP 40      // Matches Return_frame

#define FRAME_REGS_NR (FRAME_REG_USP + 1)
#define FRAME_SIZ (FRAME_REGS_NR * 4)

#define FRAME_TRAP_REGS_NR (FRAME_REG_ERR + 1)

#ifdef __ASSEMBLY__
/*
 * Trap_frame register offsets
 */
#define FRAME_PFA 0
#define FRAME_ERR 4
#define FRAME_ZERO 8
#define FRAME_AST 12
#define FRAME_V0 16
#define FRAME_V1 20

#define FRAME_A0 24
#define FRAME_A1 28
#define FRAME_A2 32
#define FRAME_A3 36

#define FRAME_T0 40
#define FRAME_T1 44
#define FRAME_T2 48
#define FRAME_T3 52
#define FRAME_T4 56
#define FRAME_T5 60
#define FRAME_T6 64
#define FRAME_T7 68

#define FRAME_S0 72
#define FRAME_S1 76
#define FRAME_S2 80
#define FRAME_S3 84
#define FRAME_S4 88
#define FRAME_S5 92
#define FRAME_S6 96
#define FRAME_S7 100
#define FRAME_T8 104
#define FRAME_T9 108

#define FRAME_K0 112
#define FRAME_K1 116
#define FRAME_GP 120
#define FRAME_SP 124
#define FRAME_S8 128
#define FRAME_RA 132

#define FRAME_MULLO 136
#define FRAME_MULHI 140
#define FRAME_SR 144
#define FRAME_EPC 148
#define FRAME_CAUSE 152
#define FRAME_BADVADDR 156

#define FRAME_USP 160
#endif /* __ASSEMBLY__ */

#ifndef __ASSEMBLY__

extern char _exceptionhandler[], _exceptionhandlerEnd[];

#endif /* __ASSEMBLY__ */

#endif /* _MIPSDEFS_H_ */
