/*
 * Derived in part from linux arch/mips/kernel/mips-cpc.c
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

class Cpc
{
public:
  static inline bool mips_cpc_present(void);

  static Static_object<Cpc> cpc;
};

//-------------------------------------------------------------------
IMPLEMENTATION [mips32]:

Static_object<Cpc> Cpc::cpc;

//-------------------------------------------------------------------
IMPLEMENTATION [mips32 && !cps]:

IMPLEMENT
static inline
bool Cpc::mips_cpc_present(void)
{
  return false;
}

//-------------------------------------------------------------------
INTERFACE [mips32 && cps]:

#include "linux_asm_mips-cpc.h"
#include "per_cpu_data.h"
#include "spin_lock.h"

//-------------------------------------------------------------------
IMPLEMENTATION [mips32 && cps]:

#include "mem_layout.h"
#include "mipsregs.h"
#include "cm.h"
#include "mem_op-mips32.h"


EXTENSION class Cpc
{
public:
  static void * &mips_cpc_base;

private:
  static Per_cpu<Spin_lock<>> cpc_core_lock;
  static Per_cpu<unsigned long> cpc_core_lock_flags;
};

#define DEFINE_PER_CPU_ALIGNED DEFINE_PER_CPU

void *mips_cpc_base; // alias used in linux_asm-mips-cpc.h macros

void * &Cpc::mips_cpc_base = ::mips_cpc_base;

DEFINE_PER_CPU_ALIGNED Per_cpu<Spin_lock<>> Cpc::cpc_core_lock;

DEFINE_PER_CPU_ALIGNED Per_cpu<unsigned long> Cpc::cpc_core_lock_flags;

PUBLIC
Cpc::Cpc()
{
  mips_cpc_probe();
}

IMPLEMENT
static inline
bool Cpc::mips_cpc_present(void)
{
  return mips_cpc_base != NULL;
}

PRIVATE
phys_t Cpc::mips_cpc_phys_base(void)
{
	u32 cpc_base;

	if (!Cm::mips_cm_present())
		return 0;

	if (!(read_gcr_cpc_status() & CM_GCR_CPC_STATUS_EX_MSK))
		return 0;

	/* If the CPC is already enabled, leave it so */
	cpc_base = read_gcr_cpc_base();
	if (cpc_base & CM_GCR_CPC_BASE_CPCEN_MSK)
		return cpc_base & CM_GCR_CPC_BASE_CPCBASE_MSK;

	/* Otherwise, give it the default address & enable it */
	cpc_base = mips_cpc_default_phys_base();
	write_gcr_cpc_base(cpc_base | CM_GCR_CPC_BASE_CPCEN_MSK);
	return cpc_base;
}

/* linux to fiasco adaptor methods */
PRIVATE
void *
Cpc::ioremap_nocache(unsigned long addr, unsigned long size)
{
  return reinterpret_cast<void*>(
        Mem_layout::ioremap_nocache((Address)addr, (Address)size)
      );
}

PUBLIC
int Cpc::mips_cpc_probe(void)
{
	phys_t addr;
#ifndef LINUX_CPC_FOR_FIASCO
	unsigned cpu;

	for_each_possible_cpu(cpu)
		spin_lock_init(&per_cpu(cpc_core_lock, cpu));
#endif

	addr = mips_cpc_phys_base();
	if (!addr)
		return 1;

	mips_cpc_base = ioremap_nocache(addr, 0x8000);
	if (!mips_cpc_base)
		return 1;

	return 0;
}

PUBLIC
void Cpc::mips_cpc_lock_other(unsigned int core)
{
	unsigned curr_core;
	preempt_disable();
#ifdef LINUX_CPC_FOR_FIASCO
        (void)curr_core;
        cpc_core_lock_flags.current() = cpc_core_lock.current().test_and_set();
#else
	curr_core = current_cpu_data.core;
	spin_lock_irqsave(&per_cpu(cpc_core_lock, curr_core),
			  per_cpu(cpc_core_lock_flags, curr_core));
#endif /* LINUX_CPC_FOR_FIASCO */
	write_cpc_cl_other(core << CPC_Cx_OTHER_CORENUM_SHF);
}

PUBLIC
void Cpc::mips_cpc_unlock_other(void)
{
#ifdef LINUX_CPC_FOR_FIASCO
        cpc_core_lock.current().set(cpc_core_lock_flags.current());
#else
	unsigned curr_core = current_cpu_data.core;
	spin_unlock_irqrestore(&per_cpu(cpc_core_lock, curr_core),
			       per_cpu(cpc_core_lock_flags, curr_core));
#endif /* LINUX_CPC_FOR_FIASCO */
	preempt_enable();
}
