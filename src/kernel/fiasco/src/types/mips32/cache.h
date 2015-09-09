/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 */

#ifndef _MIPS_CACHE_
#define _MIPS_CACHE_

#define SYNC  __asm volatile("sync")

#define _MIPS_CACHECFG_SHIFT(reg)  __MIPS_CACHECFG_SHIFT(reg)
#define __MIPS_CACHECFG_SHIFT(reg) MIPS_CACHECFG_ ## reg ## _SHIFT
#define _MIPS_CACHECFG_MASK(reg) __MIPS_CACHECFG_MASK(reg)
#define __MIPS_CACHECFG_MASK(reg)  MIPS_CACHECFG_ ## reg ## _MASK

#define MIPS_CACHECFG_GET(reg, x)            \
    ((unsigned)((x) & _MIPS_CACHECFG_MASK(reg)) >> _MIPS_CACHECFG_SHIFT(reg))
#define MIPS_CACHECFG_PUT(reg, val)            \
    (((x) << _MIPS_CACHECFG_SHIFT(reg)) & _MIPS_CACHECFG_MASK(reg))

/* "IS" (R): (Primary) I-cache sets per way. */
#define MIPS_CACHECFG_CFG1_IS_MASK 0x01c00000
#define MIPS_CACHECFG_CFG1_IS_SHIFT  22

#define MIPS_CACHECFG_CFG1_IS_RSVD 7   /* rsvd value, otherwise: */
#define MIPS_CACHECFG_CFG1_IS(x) (64 << MIPS_CACHECFG_GET(CFG1_IS, (x)))

/* "IL" (R): (Primary) I-cache line size. */
#define MIPS_CACHECFG_CFG1_IL_MASK 0x00380000
#define MIPS_CACHECFG_CFG1_IL_SHIFT  19

#define MIPS_CACHECFG_CFG1_IL_NONE 0   /* No I-cache, */
#define MIPS_CACHECFG_CFG1_IL_RSVD 7   /* rsvd value, otherwise: */
#define MIPS_CACHECFG_CFG1_IL(x) (2 << MIPS_CACHECFG_GET(CFG1_IL, (x)))

/* "IA" (R): (Primary) I-cache associativity (ways - 1). */
#define MIPS_CACHECFG_CFG1_IA_MASK 0x00070000
#define MIPS_CACHECFG_CFG1_IA_SHIFT  16

#define MIPS_CACHECFG_CFG1_IA(x) MIPS_CACHECFG_GET(CFG1_IA, (x))

/* "DS" (R): (Primary) D-cache sets per way. */
#define MIPS_CACHECFG_CFG1_DS_MASK 0x0000e000
#define MIPS_CACHECFG_CFG1_DS_SHIFT  13

#define MIPS_CACHECFG_CFG1_DS_RSVD 7   /* rsvd value, otherwise: */
#define MIPS_CACHECFG_CFG1_DS(x) (64 << MIPS_CACHECFG_GET(CFG1_DS, (x)))

/* "DL" (R): (Primary) D-cache line size. */
#define MIPS_CACHECFG_CFG1_DL_MASK   0x00001c00
#define MIPS_CACHECFG_CFG1_DL_SHIFT  10

#define MIPS_CACHECFG_CFG1_DL_NONE 0   /* No D-cache, */
#define MIPS_CACHECFG_CFG1_DL_RSVD 7   /* rsvd value, otherwise: */
#define MIPS_CACHECFG_CFG1_DL(x) (2 << MIPS_CACHECFG_GET(CFG1_DL, (x)))

/* "DA" (R): (Primary) D-cache associativity (ways - 1). */
#define MIPS_CACHECFG_CFG1_DA_MASK 0x00000380
#define MIPS_CACHECFG_CFG1_DA_SHIFT  7

#define MIPS_CACHECFG_CFG1_DA(x) MIPS_CACHECFG_GET(CFG1_DA, (x))

#endif /* _MIPS_CACHE_ */
