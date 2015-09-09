/*
 * Derived in part from linux arch/mips/kernel/mips-cm.c
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * Copyright (C) 2013 Imagination Technologies
 * Author: Paul Burton <paul.burton@imgtec.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

INTERFACE [mips32]:

#include "types.h"

class Cm
{
public:
  static inline bool mips_cm_present(void);

  static Static_object<Cm> cm;
};

//-------------------------------------------------------------------
IMPLEMENTATION [mips32]:

Static_object<Cm> Cm::cm;

//-------------------------------------------------------------------
IMPLEMENTATION [mips32 && !cps]:

IMPLEMENT
static inline
bool Cm::mips_cm_present(void)
{
  return false;
}

//-------------------------------------------------------------------
INTERFACE [mips32 && cps]:

#include "linux_asm_mips-cm.h"

//-------------------------------------------------------------------
IMPLEMENTATION [mips32 && cps]:

#include "mem_layout.h"
#include "mipsregs.h"
#include "kdb_ke.h"

#define BUG_ON(x) assert_kdb(!(x))

EXTENSION class Cm
{
public:
  static void * &mips_cm_base;
};

void *mips_cm_base; // alias used in linux_asm-mips-cm.h macros

void * &Cm::mips_cm_base = ::mips_cm_base;

PUBLIC
Cm::Cm()
{
  mips_cm_probe();
}

IMPLEMENT
static inline
bool Cm::mips_cm_present(void)
{
  return mips_cm_base != NULL;
}

/**
 * mips_cm_numcores - return the number of cores present in the system
 *
 * Returns the value of the PCORES field of the GCR_CONFIG register plus 1, or
 * zero if no Coherence Manager is present.
 */
PUBLIC static inline NEEDS["mipsregs.h"]
unsigned Cm::mips_cm_numcores(void)
{
	if (!mips_cm_present())
		return 0;

	return ((read_gcr_config() & CM_GCR_CONFIG_PCORES_MSK)
		>> CM_GCR_CONFIG_PCORES_SHF) + 1;
}

PUBLIC
phys_t Cm::mips_cm_phys_base(void)
{
	u32 config3 = read_c0_config3();
	u32 cmgcr;

	/* Check the CMGCRBase register is implemented */
	if (!(config3 & MIPS_CONF3_CMGCR))
		return 0;

	/* Read the phys_t from CMGCRBase */
	cmgcr = read_c0_cmgcrbase();
	return (cmgcr & MIPS_CMGCRF_BASE) << (36 - 32);
}

//-------------------------------------------------------------------
// hide #ifndef'd functions from Fiasco C++ 'preprocess' tool using && 0
IMPLEMENTATION [mips32 && cps && 0]:

#ifndef LINUX_CM_FOR_FIASCO
phys_t mips_cm_phys_base(void)
	__attribute__((weak, alias("__mips_cm_phys_base")));

phys_t __mips_cm_l2sync_phys_base(void)
{
	u32 base_reg;

	/*
	 * If the L2-only sync region is already enabled then leave it at it's
	 * current location.
	 */
	base_reg = read_gcr_l2_only_sync_base();
	if (base_reg & CM_GCR_L2_ONLY_SYNC_BASE_SYNCEN_MSK)
		return base_reg & CM_GCR_L2_ONLY_SYNC_BASE_SYNCBASE_MSK;

	/* Default to following the CM */
	return mips_cm_phys_base() + MIPS_CM_GCR_SIZE;
}

phys_t mips_cm_l2sync_phys_base(void)
	__attribute__((weak, alias("__mips_cm_l2sync_phys_base")));

static void mips_cm_probe_l2sync(void)
{
	unsigned major_rev;
	phys_t addr;

	/* L2-only sync was introduced with CM major revision 6 */
	major_rev = (read_gcr_rev() & CM_GCR_REV_MAJOR_MSK) >>
		CM_GCR_REV_MAJOR_SHF;
	if (major_rev < 6)
		return;

	/* Find a location for the L2 sync region */
	addr = mips_cm_l2sync_phys_base();
	BUG_ON((addr & CM_GCR_L2_ONLY_SYNC_BASE_SYNCBASE_MSK) != addr);
	if (!addr)
		return;

	/* Set the region base phys_t & enable it */
	write_gcr_l2_only_sync_base(addr | CM_GCR_L2_ONLY_SYNC_BASE_SYNCEN_MSK);

	/* Map the region */
	mips_cm_l2sync_base = ioremap_nocache(addr, MIPS_CM_L2SYNC_SIZE);
}
#endif /* LINUX_CM_FOR_FIASCO */

//-------------------------------------------------------------------
IMPLEMENTATION [mips32 && cps]:

/* linux to fiasco adaptor methods */
PRIVATE
void *
Cm::ioremap_nocache(unsigned long addr, unsigned long size)
{
  return reinterpret_cast<void*>(
        Mem_layout::ioremap_nocache((Address)addr, (Address)size)
      );
}

PRIVATE
int Cm::mips_cm_probe(void)
{
	phys_t addr;
	u32 base_reg;

	addr = mips_cm_phys_base();
	BUG_ON((addr & CM_GCR_BASE_GCRBASE_MSK) != addr);
	if (!addr)
		return 1;

	mips_cm_base = ioremap_nocache(addr, MIPS_CM_GCR_SIZE);
	if (!mips_cm_base)
		return 1;

	/* sanity check that we're looking at a CM */
	base_reg = read_gcr_base();
	if ((base_reg & CM_GCR_BASE_GCRBASE_MSK) != addr) {
		printf("GCRs appear to have been moved (expected them at 0x%08lx)!\n",
		       (unsigned long)addr);
		mips_cm_base = NULL;
		return 1;
	}

	/* set default target to memory */
	base_reg &= ~CM_GCR_BASE_CMDEFTGT_MSK;
	base_reg |= CM_GCR_BASE_CMDEFTGT_MEM;
	write_gcr_base(base_reg);

	/* disable CM regions */
	write_gcr_reg0_base(CM_GCR_REGn_BASE_BASEADDR_MSK);
	write_gcr_reg0_mask(CM_GCR_REGn_MASK_ADDRMASK_MSK);
	write_gcr_reg1_base(CM_GCR_REGn_BASE_BASEADDR_MSK);
	write_gcr_reg1_mask(CM_GCR_REGn_MASK_ADDRMASK_MSK);
	write_gcr_reg2_base(CM_GCR_REGn_BASE_BASEADDR_MSK);
	write_gcr_reg2_mask(CM_GCR_REGn_MASK_ADDRMASK_MSK);
	write_gcr_reg3_base(CM_GCR_REGn_BASE_BASEADDR_MSK);
	write_gcr_reg3_mask(CM_GCR_REGn_MASK_ADDRMASK_MSK);

#ifndef LINUX_CM_FOR_FIASCO
	/* probe for an L2-only sync region */
	mips_cm_probe_l2sync();
#endif /* LINUX_CM_FOR_FIASCO */

	return 0;
}
