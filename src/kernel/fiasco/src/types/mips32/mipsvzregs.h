/*
 * Derived in part from linux arch/mips/include/asm/mipsvzregs.h
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
* This file is subject to the terms and conditions of the GNU General Public
* License.  See the file "COPYING" in the main directory of this archive
* for more details.
*
* MIPS VZ-ASE  related register defines and helper macros
*
* Copyright (C) 2012  MIPS Technologies, Inc.  All rights reserved.
* Authors: Yann Le Du <ledu@kymasys.com>
*/


/*
 * VZ regs definitions, follows on from mipsregs.h
 */

#ifndef _ASM_MIPSVZREGS_H
#define _ASM_MIPSVZREGS_H

#include <mipsregs.h>
//#include <asm/war.h>

#ifndef __ASSEMBLY__

/*
 * C macros
 */

#define read_c0_guestctl0()	__read_32bit_c0_register($12, 6)
#define write_c0_guestctl0(val) __write_32bit_c0_register($12, 6, val)
#define read_c0_guestctl1()	__read_32bit_c0_register($10, 4)
#define write_c0_guestctl1(val)	__write_32bit_c0_register($10, 4, val)
#define read_c0_guestctl2()	__read_32bit_c0_register($10, 5)
#define write_c0_guestctl2(val)	__write_32bit_c0_register($10, 5, val)
#define read_c0_gtoffset()	__read_32bit_c0_register($12, 7)
#define write_c0_gtoffset(val) __write_32bit_c0_register($12, 7, val)
#define read_c0_badinstr()	__read_ulong_c0_register($8, 1)
#define write_c0_badinstr(val)	__write_ulong_c0_register($8, 1, val)
#define read_c0_badinstrp()	__read_ulong_c0_register($8, 2)
#define write_c0_badinstrp(val)	__write_ulong_c0_register($8, 2, val)
#define read_c0_guestctl0ext()	__read_32bit_c0_register($11, 4)
#define write_c0_guestctl0ext(val) __write_32bit_c0_register($11, 4, val)

__BUILD_SET_C0(guestctl1)
__BUILD_SET_C0(guestctl2)

#else /* Assembly */
/*
 * Macros for use in assembly language code
 */

#define CP0_GUESTCTL0		$12,6
#define CP0_GUESTCTL1		$10,4
#define CP0_GTOFFSET		$12,7

#endif

/* GuestCtl0 fields */
#define GUESTCTL0_GM_SHIFT	31
#define GUESTCTL0_GM		(_ULCAST_(1) << GUESTCTL0_GM_SHIFT)
#define GUESTCTL0_CP0_SHIFT	28
#define GUESTCTL0_CP0		(_ULCAST_(1) << GUESTCTL0_CP0_SHIFT)
#define GUESTCTL0_AT_SHIFT	26
#define GUESTCTL0_AT 		(_ULCAST_(0x3) << GUESTCTL0_AT_SHIFT)
#define GUESTCTL0_AT3		(_ULCAST_(3) << GUESTCTL0_AT_SHIFT)
#define GUESTCTL0_AT1		(_ULCAST_(1) << GUESTCTL0_AT_SHIFT)
#define GUESTCTL0_GT_SHIFT	25
#define GUESTCTL0_GT		(_ULCAST_(1) << GUESTCTL0_GT_SHIFT)
#define GUESTCTL0_CG_SHIFT	24
#define GUESTCTL0_CG		(_ULCAST_(1) << GUESTCTL0_CG_SHIFT)
#define GUESTCTL0_CF_SHIFT	23
#define GUESTCTL0_CF		(_ULCAST_(1) << GUESTCTL0_CF_SHIFT)
#define GUESTCTL0_G1_SHIFT	22
#define GUESTCTL0_G1		(_ULCAST_(1) << GUESTCTL0_G1_SHIFT)
#define GUESTCTL0_G0E_SHIFT	19
#define GUESTCTL0_G0E		(_ULCAST_(1) << GUESTCTL0_G0E_SHIFT)
#define GUESTCTL0_RAD_SHIFT	9
#define GUESTCTL0_RAD		(_ULCAST_(1) << GUESTCTL0_RAD_SHIFT)
#define GUESTCTL0_DRG_SHIFT	8
#define GUESTCTL0_DRG		(_ULCAST_(1) << GUESTCTL0_DRG_SHIFT)
#define GUESTCTL0_G2_SHIFT	7
#define GUESTCTL0_G2		(_ULCAST_(1) << GUESTCTL0_G2_SHIFT)

/* GuestCtl0.GExcCode Hypervisor exception cause code */
#define GUESTCTL0_GEXC_SHIFT	2
#define GUESTCTL0_GEXC		(_ULCAST_(0x1f) << GUESTCTL0_GEXC_SHIFT)
#define GUESTCTL0_GEXC_GPSI	0  /* Guest Privileged Sensitive Instruction */
#define GUESTCTL0_GEXC_GSFC 	1  /* Guest Software Field Change */
#define GUESTCTL0_GEXC_HC 	2  /* Hypercall */
#define GUESTCTL0_GEXC_GRR	3  /* Guest Reserved Instruction Redirect */
#define GUESTCTL0_GEXC_GVA	8  /* Guest Virtual Address available */
#define GUESTCTL0_GEXC_GHFC 	9  /* Guest Hardware Field Change */
#define GUESTCTL0_GEXC_GPA	10 /* Guest Physical Address available */

#define GUESTCTL0_SFC2_SHIFT	1
#define GUESTCTL0_SFC2		(_ULCAST_(1) << GUESTCTL0_SFC2_SHIFT)

#define GUESTCTL0_SFC1_SHIFT	0
#define GUESTCTL0_SFC1		(_ULCAST_(1) << GUESTCTL0_SFC1_SHIFT)

/* GuestCtl0Ext fields */
#define GUESTCTL0Ext_CGI_SHIFT	4
#define GUESTCTL0Ext_CGI	(_ULCAST_(1) << GUESTCTL0Ext_CGI_SHIFT)
#define GUESTCTL0Ext_FCD_SHIFT	3
#define GUESTCTL0Ext_FCD	(_ULCAST_(1) << GUESTCTL0Ext_FCD_SHIFT)
#define GUESTCTL0Ext_OG_SHIFT	2
#define GUESTCTL0Ext_OG		(_ULCAST_(1) << GUESTCTL0Ext_OG_SHIFT)
#define GUESTCTL0Ext_BG_SHIFT	1
#define GUESTCTL0Ext_BG		(_ULCAST_(1) << GUESTCTL0Ext_BG_SHIFT)
#define GUESTCTL0Ext_MG_SHIFT	0
#define GUESTCTL0Ext_MG		(_ULCAST_(1) << GUESTCTL0Ext_MG_SHIFT)

/* GuestCtl1 fields */
#define GUESTCTL1_ID_SHIFT	0
#define GUESTCTL1_ID_WIDTH	8
#define GUESTCTL1_ID		(_ULCAST_(0xff) << GUESTCTL1_ID_SHIFT)
#define GUESTCTL1_RID_SHIFT	16
#define GUESTCTL1_RID_WIDTH	8
#define GUESTCTL1_RID		(_ULCAST_(0xff) << GUESTCTL1_RID_SHIFT)

/* VZ GuestID reserved for root context */
#define GUESTCTL1_VZ_ROOT_GUESTID   0x00 

/* GuestCtl2 fields */
#define GUESTCTL2_VIP_SHIFT	10
#define GUESTCTL2_VIP		(_ULCAST_(0xff) << GUESTCTL2_VIP_SHIFT)

#ifndef __ASSEMBLY__

#define mfgc0(rd,sel)							\
({									\
	 unsigned long  __res;						\
									\
	__asm__ __volatile__(						\
	"	.set	push					\n"	\
	"	.set	mips32r2				\n"	\
	"	.set	noat					\n"	\
	"	# mfgc0	$1, $" #rd ", " #sel "			\n"	\
	"	.word	0x40600000 | (1<<16) | (" #rd "<<11) | " #sel "	\n"	\
	"	move	%0, $1					\n"	\
	"	.set	pop					\n"	\
	: "=r" (__res));						\
									\
	__res;								\
})

#define mtgc0(rd, sel, v)							\
({									\
	__asm__ __volatile__(						\
	"	.set	push					\n"	\
	"	.set	mips32r2				\n"	\
	"	.set	noat					\n"	\
	"	move	$1, %0					\n"	\
	"	# mtgc0 $1," #rd ", " #sel "			\n"	\
	"	.word	0x40600200 | (1<<16) | (" #rd "<<11) | " #sel "	\n"	\
	"	.set	pop					\n"	\
	:								\
	: "r" (v));							\
})

static inline void tlb_read_guest_indexed(void)
{
	__asm__ __volatile__(
	"	.set	push\n"
	"	.set	noreorder\n"
	"	.set	mips32r2\n"
	"	.word	0x42000009  # tlbgr ASM_TLBGR \n"
	"	.set	reorder\n"
	"	.set	pop\n");
}

static inline void tlb_write_guest_indexed(void)
{
	__asm__ __volatile__(
	"	.set	push\n"
	"	.set	noreorder\n"
	"	.set	mips32r2\n"
	"	.word	0x4200000a  # tlbgwi ASM_TLBGWI \n"
	"	.set	reorder\n"
	"	.set	pop\n");
}

static inline void tlb_invalidate_asid(void)
{
	__asm__ __volatile__(
	"	.set	push						\n"
	"	.set	mips32r2					\n"
	"	.set	noreorder					\n"
	"	.word	0x42000003  # tlbinv ASM_TLBINV		\n"
	"	.set	pop						\n"
	);
}

static inline void tlb_invalidate_flush(void)
{
	__asm__ __volatile__(
	"	.set	push						\n"
	"	.set	mips32r2					\n"
	"	.set	noreorder					\n"
	"	.word	0x42000004  # tlbinvf ASM_TLBINVF		\n"
	"	.set	pop						\n"
	);
}

static inline void tlb_guest_invalidate_flush(void)
{
	__asm__ __volatile__(
	"	.set	push						\n"
	"	.set	mips32r2					\n"
	"	.set	noreorder					\n"
	"	.word	0x4200000c  # tlbginvf ASM_TLBGINVF		\n"
	"	.set	pop						\n"
	);
}

#define read_c0_guest_index()           mfgc0(0, 0)
#define write_c0_guest_index(val)       mtgc0(0, 0, val)
#define read_c0_guest_random()          mfgc0(1, 0)
#define read_c0_guest_entrylo0()        mfgc0(2, 0)
#define write_c0_guest_entrylo0(val)    mtgc0(2, 0, val)
#define read_c0_guest_entrylo1()        mfgc0(3, 0)
#define write_c0_guest_entrylo1(val)    mtgc0(3, 0, val)
#define read_c0_guest_context()         mfgc0(4, 0)
#define write_c0_guest_context(val)     mtgc0(4, 0, val)
#define read_c0_guest_userlocal()       mfgc0(4, 2)
#define write_c0_guest_userlocal(val)   mtgc0(4, 2, val)
#define read_c0_guest_pagemask()        mfgc0(5, 0)
#define write_c0_guest_pagemask(val)    mtgc0(5, 0, val)
#define read_c0_guest_pagegrain()       mfgc0(5, 1)
#define write_c0_guest_pagegrain(val)   mtgc0(5, 1, val)
#define read_c0_guest_wired()           mfgc0(6, 0)
#define write_c0_guest_wired(val)       mtgc0(6, 0, val)
#define read_c0_guest_hwrena()          mfgc0(7, 0)
#define write_c0_guest_hwrena(val)      mtgc0(7, 0, val)
#define read_c0_guest_badvaddr()        mfgc0(8, 0)
#define write_c0_guest_badvaddr(val)    mtgc0(8, 0, val)
#define read_c0_guest_count()           mfgc0(9, 0)
#define write_c0_guest_count(val)       mtgc0(9, 0, val)
#define read_c0_guest_entryhi()         mfgc0(10, 0)
#define write_c0_guest_entryhi(val)     mtgc0(10, 0, val)
#define read_c0_guest_compare()         mfgc0(11, 0)
#define write_c0_guest_compare(val)     mtgc0(11, 0, val)
#define read_c0_guest_status()          mfgc0(12, 0)
#define write_c0_guest_status(val)      mtgc0(12, 0, val)
#define read_c0_guest_intctl()          mfgc0(12, 1)
#define write_c0_guest_intctl(val)      mtgc0(12, 1, val)
#define read_c0_guest_cause()           mfgc0(13, 0)
#define write_c0_guest_cause(val)       mtgc0(13, 0, val)
#define read_c0_guest_epc()             mfgc0(14, 0)
#define write_c0_guest_epc(val)         mtgc0(14, 0, val)
#define read_c0_guest_ebase()           mfgc0(15, 1)
#define write_c0_guest_ebase(val)       mtgc0(15, 1, val)
#define read_c0_guest_config()          mfgc0(16, 0)
#define read_c0_guest_config1()         mfgc0(16, 1)
#define read_c0_guest_config2()         mfgc0(16, 2)
#define read_c0_guest_config3()         mfgc0(16, 3)
#define read_c0_guest_config4()         mfgc0(16, 4)
#define read_c0_guest_config5()         mfgc0(16, 5)
#define read_c0_guest_config6()         mfgc0(16, 6)
#define read_c0_guest_config7()         mfgc0(16, 7)
#define write_c0_guest_config(val)      mtgc0(16, 0, val)
#define write_c0_guest_config1(val)     mtgc0(16, 1, val)
#define write_c0_guest_config2(val)     mtgc0(16, 2, val)
#define write_c0_guest_config3(val)     mtgc0(16, 3, val)
#define write_c0_guest_config4(val)     mtgc0(16, 4, val)
#define write_c0_guest_config5(val)     mtgc0(16, 5, val)
#define write_c0_guest_config6(val)     mtgc0(16, 6, val)
#define write_c0_guest_config7(val)     mtgc0(16, 7, val)
#define read_c0_guest_errorepc()        mfgc0(30, 0)
#define write_c0_guest_errorepc(val)    mtgc0(30, 0, val)

__BUILD_SET_C0(guest_status)
__BUILD_SET_C0(guest_cause)
__BUILD_SET_C0(guest_ebase)

#else /* end not __ASSEMBLY__ */

/*
 *************************************************************************
 *                S O F T W A R E   G P R   I N D I C E S                *
 *************************************************************************
 *
 * These definitions provide the index (number) of the GPR, as opposed
 * to the assembler register name ($n).
 */

#define R_zero                   0
#define R_AT                     1
#define R_v0                     2
#define R_v1                     3
#define R_a0                     4
#define R_a1                     5
#define R_a2                     6
#define R_a3                     7
#define R_t0                     8
#define R_t1                     9
#define R_t2                    10
#define R_t3                    11
#define R_t4                    12
#define R_t5                    13
#define R_t6                    14
#define R_t7                    15
#define R_s0                    16
#define R_s1                    17
#define R_s2                    18
#define R_s3                    19
#define R_s4                    20
#define R_s5                    21
#define R_s6                    22
#define R_s7                    23
#define R_t8                    24
#define R_t9                    25
#define R_repc                  25
#define R_k0                    26
#define R_k1                    27
#define R_gp                    28
#define R_sp                    29
#define R_fp                    30
#define R_s8                    30
#define R_tid                   30
#define R_ra                    31
#define R_hi                    32                      /* Hi register */
#define R_lo                    33                      /* Lo register */

/*
 *************************************************************************
 *             C P 0   R E G I S T E R   D E F I N I T I O N S           *
 *************************************************************************
 * Each register has the following definitions:
 *
 *      C0_rrr          The register number (as a $n value)
 *      R_C0_rrr        The register index (as an integer corresponding
 *                      to the register number)
 *      R_C0_Selrrr     The register select (as an integer corresponding
 *                      to the register select)
 *
 * Each field in a register has the following definitions:
 *
 *      S_rrrfff        The shift count required to right-justify
 *                      the field.  This corresponds to the bit
 *                      number of the right-most bit in the field.
 *      M_rrrfff        The Mask required to isolate the field.
 *
 * Register diagrams included below as comments correspond to the
 * MIPS32 and MIPS64 architecture specifications.  Refer to other
 * sources for register diagrams for older architectures.
 */
#define R_CP0_INDEX              0
#define R_CP0_SELINDEX           0
#define R_CP0_RANDOM             1
#define R_CP0_SELRANDOM          0
#define R_CP0_ENTRYLO0           2
#define R_CP0_SELENTRYLO0        0
#define R_CP0_ENTRYLO1           3
#define R_CP0_SELENTRYLO1        0
#define R_CP0_CONTEXT            4
#define R_CP0_SELCONTEXT         0
#define R_CP0_CONTEXTCONFIG      4       /* Overload */
#define R_CP0_SELCONTEXTCONFIG   1
#define R_CP0_XCONTEXTCONFIG     4
#define R_CP0_SELXCONTEXTCONFIG  3
#define R_CP0_USERLOCAL          4
#define R_CP0_SELUSERLOCAL       2
#define R_CP0_PAGEMASK           5                       /* Mask (R/W) */
#define R_CP0_SELPAGEMASK        0
#define R_CP0_PAGEGRAIN          5                       /* Mask (R/W) */
#define R_CP0_SELPAGEGRAIN       1
#define R_CP0_WIRED              6
#define R_CP0_SELWIRED           0
#define R_CP0_HWRENA             7
#define R_CP0_SELHWRENA          0
#define R_CP0_BADVADDR           8
#define R_CP0_SELBADVADDR        0
#define R_CP0_BADINSTR           8
#define R_CP0_SELBADINSTR        1
#define R_CP0_BADINSTRP          8
#define R_CP0_SELBADINSTRP       2
#define R_CP0_COUNT              9
#define R_CP0_SELCOUNT           0
#define R_CP0_ENTRYHI            10
#define R_CP0_SELENTRYHI         0
#define R_CP0_COMPARE            11
#define R_CP0_SELCOMPARE         0
#define R_CP0_STATUS             12
#define R_CP0_SELSTATUS          0
#define R_CP0_INTCTL             12
#define R_CP0_SELINTCTL          1
#define R_CP0_SRSCTL             12
#define R_CP0_SELSRSCTL          2
#define R_CP0_SRSMAP             12
#define R_CP0_SELSRSMAP          3
#define R_CP0_VIEW_IPL           12
#define R_CP0_SELVIEW_IPL        4
#define R_CP0_SRSMAP2            12
#define R_CP0_SELSRSMAP2         5
#define R_CP0_CAUSE              13
#define R_CP0_SELCAUSE           0
#define R_CP0_VIEW_RIPL          13
#define R_CP0_SELVIEW_RIPL       4
#define R_CP0_EPC                14
#define R_CP0_SELEPC             0
#define R_CP0_PRID               15
#define R_CP0_SELPRID            0
#define R_CP0_EBASE              15
#define R_CP0_SELEBASE           1
#define R_CP0_CDMMBASE           15
#define R_CP0_SELCDMMBASE        2
#define R_CP0_CMGCRBASE          15
#define R_CP0_SELCMGCRBASE       3
#define R_CP0_CONFIG             16
#define R_CP0_SELCONFIG          0
#define R_CP0_CONFIG1            16
#define R_CP0_SELCONFIG1         1
#define R_CP0_CONFIG2            16
#define R_CP0_SELCONFIG2         2
#define R_CP0_CONFIG3            16
#define R_CP0_SELCONFIG3         3
#define R_CP0_CONFIG4            16
#define R_CP0_SELCONFIG4         4
#define R_CP0_CONFIG6            16
#define R_CP0_SELCONFIG6         6
#define R_CP0_CONFIG7            16
#define R_CP0_SELCONFIG7         7
#define R_CP0_LLADDR             17
#define R_CP0_SELLLADDR          0
#define R_CP0_WATCHLO            18
#define R_CP0_SELWATCHLO         0
#define R_CP0_WATCHHI            19
#define R_CP0_SELWATCHHI         0
#define R_CP0_XCONTEXT           20
#define R_CP0_SELXCONTEXT        0
#define R_CP0_DEBUG              23
#define R_CP0_SELDEBUG           0
#define R_CP0_TRACECONTROL       23
#define R_CP0_SELTRACECONTROL    1
#define R_CP0_TRACECONTROL2      23
#define R_CP0_SELTRACECONTROL2   2
#define R_CP0_USERTRACEDATA      23
#define R_CP0_SELUSERTRACEDATA   3
#define R_CP0_USERTRACEDATA2     24
#define R_CP0_SELUSERTRACEDATA2  3
#define R_CP0_TRACEBPC           23
#define R_CP0_SELTRACEBPC        4
#define R_CP0_TRACEIBPC          23
#define R_CP0_SELTRACEIBPC       4
#define R_CP0_TRACEDBPC          23
#define R_CP0_SELTRACEDBPC       5
#define R_CP0_DEBUG2             23
#define R_CP0_SELDEBUG2          6
#define R_CP0_TRACECONTROL3      24
#define R_CP0_SELTRACECONTROL3   2
#define R_CP0_DEPC               24
#define R_CP0_SELDEPC            0
#define R_CP0_PERFCNT            25
#define R_CP0_SELPERFCNT         0
#define R_CP0_SELPERFCNT0        1
#define R_CP0_SELPERFCNT1        3
#define R_CP0_SELPERFCNT2        5
#define R_CP0_SELPERFCNT3        7
#define R_CP0_PERFCTRL           25
#define R_CP0_SELPERFCTRL0       0
#define R_CP0_SELPERFCTRL1       2
#define R_CP0_SELPERFCTRL2       4
#define R_CP0_SELPERFCTRL3       6
#define R_CP0_ERRCTL             26
#define R_CP0_SELERRCTL          0
#define R_CP0_CACHEERR           27
#define R_CP0_SELCACHEERR        0
#define R_CP0_TAGLO              28
#define R_CP0_SELTAGLO           0
#define R_CP0_ITAGLO             28
#define R_CP0_SELITAGLO          0
#define R_CP0_DTAGLO             28
#define R_CP0_SELDTAGLO          2
#define R_CP0_STAGLO             28
#define R_CP0_SELSTAGLO          4
#define R_CP0_DATALO             28
#define R_CP0_SELDATALO          1
#define R_CP0_IDATALO            28
#define R_CP0_SELIDATALO         1
#define R_CP0_DDATALO            28
#define R_CP0_SELDDATALO         3
#define R_CP0_SDATALO            28
#define R_CP0_SELSDATALO         5
#define R_CP0_TAGHI              29
#define R_CP0_SELTAGHI           0
#define R_CP0_ITAGHI             29
#define R_CP0_SELITAGHI          0
#define R_CP0_DTAGHI             29
#define R_CP0_SELDTAGHI          2
#define R_CP0_STAGHI             29
#define R_CP0_SELSTAGHI          4
#define R_CP0_DATAHI             29
#define R_CP0_SELDATAHI          1
#define R_CP0_IDATAHI            29
#define R_CP0_SELIDATAHI         1
#define R_CP0_DDATAHI            29
#define R_CP0_SELDDATAHI         3
#define R_CP0_SDATAHI            29
#define R_CP0_SELSDATAHI         5
#define R_CP0_ERROREPC           30
#define R_CP0_SELERROREPC        0
#define R_CP0_DESAVE             31
#define R_CP0_SELDESAVE          0

#define R_CP0_GUESTCTL0          12
#define R_CP0_SELGUESTCTL0       6
#define R_CP0_GUESTCTL1          10
#define R_CP0_SELGUESTCTL1       4
#define R_CP0_GTOFFSET           12
#define R_CP0_SELGTOFFSET        7
#define R_CP0_SEGCTL0            5
#define R_CP0_SELSEGCTL0         2
#define R_CP0_SEGCTL1            5
#define R_CP0_SELSEGCTL1         3
#define R_CP0_SEGCTL2            5
#define R_CP0_SELSEGCTL2         4

#define R_CP0_KSCRATCH1          31
#define R_CP0_SELKSCRATCH1       2
#define R_CP0_KSCRATCH2          31
#define R_CP0_SELKSCRATCH2       3

/* Use these ASM_* macros with the above R_* register constants for rt,rd */

// VZ ASE Extensions
#define ASM_HYPCALL(code)    .word( 0x42000028 | (code<<11) )
#define ASM_MFGC0(rt,rd,sel) .word( 0x40600000 | (rt<<16) | (rd<<11) | sel )
#define ASM_MTGC0(rt,rd,sel) .word( 0x40600200 | (rt<<16) | (rd<<11) | sel )
#define ASM_TLBGP            .word( 0x42000010 )
#define ASM_TLBGR            .word( 0x42000009 )
#define ASM_TLBGWI           .word( 0x4200000a )
#define ASM_TLBGINV          .word( 0x4200000b )
#define ASM_TLBGINVF         .word( 0x4200000c )
#define ASM_TLBGWR           .word( 0x4200000e )

// Config4.IE TLB invalidate instructions
#define ASM_TLBINV           .word( 0x42000003 )
#define ASM_TLBINVF          .word( 0x42000004 )

#endif
#endif
