/*
 * Derived in part from linux arch/mips/kernel/cpu-probe.c
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * Processor capabilities determination functions.
 *
 * Copyright (C) xxxx  the Anonymous
 * Copyright (C) 1994 - 2006 Ralf Baechle
 * Copyright (C) 2003, 2004  Maciej W. Rozycki
 * Copyright (C) 2001, 2004, 2011, 2012	 MIPS Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

INTERFACE [mips32]:

#include "per_cpu_data.h"
#include "initcalls.h"
#include "types.h"
#include "cpu-arch.h"

EXTENSION class Cpu
{
public:
  void init(bool is_boot_cpu = false);
  static void early_init();

  Unsigned64 frequency() const { return _frequency; }
  static Per_cpu<Cpu> cpus;
  static Cpu *boot_cpu() { return _boot_cpu; }

  Cpu(Cpu_number cpu) { set_id(cpu); }

  static inline Signed32 tlbsize() { return Cpu::_cpuinfo.tlbsize; }
  static inline Signed32 vztlbsize() { return Cpu::_cpuinfo.vz.tlbsize; }

  void set_isa(struct cpuinfo_mips *c, unsigned int isa);

  static inline Mword options() { return Cpu::_cpuinfo.options; }
  static inline Mword extensions() { return Cpu::_cpuinfo.ases; }
  static inline Mword isa_level() { return Cpu::_cpuinfo.isa_level; }

  static inline Mword vzoptions() { return Cpu::_cpuinfo.vz.options; }
  static inline Mword vzconfig0() { return Cpu::_cpuinfo.vz.config0; }
  static inline Mword vzconfig1() { return Cpu::_cpuinfo.vz.config1; }
  static inline Mword vzconfig2() { return Cpu::_cpuinfo.vz.config2; }
  static inline Mword vzconfig3() { return Cpu::_cpuinfo.vz.config3; }
  static inline Mword vzconfig4() { return Cpu::_cpuinfo.vz.config4; }
  static inline Mword vzconfig5() { return Cpu::_cpuinfo.vz.config5; }
  static inline Mword vzconfig6() { return Cpu::_cpuinfo.vz.config6; }
  static inline Mword vzconfig7() { return Cpu::_cpuinfo.vz.config7; }
  static inline Mword vzguestid_mask() { return Cpu::_cpuinfo.vzguestid_mask; }
  static inline Mword has_guestctl0ext() { return Cpu::_cpuinfo.has_guestctl0ext; }
  static inline Mword has_guestctl1() { return Cpu::_cpuinfo.has_guestctl1; }
  static inline Mword has_guestctl2() { return Cpu::_cpuinfo.has_guestctl2; }

private:

  static const Unsigned64 _frequency = Config::cpu_frequency * 1000000;
  static Cpu *_boot_cpu;
  Cpu_phys_id _phys_id;
  static unsigned long _ns_per_cycle;
  Unsigned32 processor_id;
  const char *name;
  
  struct vzase_info {
    unsigned long   options;
    int             tlbsize;
    unsigned long   config0;
    unsigned long   config1;
    unsigned long   config2;
    unsigned long   config3;
    unsigned long   config4;
    unsigned long   config5;
    unsigned long   config6;
    unsigned long   config7;
  };

  struct cpuinfo_mips {
    Mword options;
    Mword ases;
    Signed32 isa_level;
    Signed32 tlbsize;
    Mword core;
    struct vzase_info	vz;
    unsigned long vzguestid_mask;
    Mword has_guestctl0ext;
    Mword has_guestctl1;
    Mword has_guestctl2;
    Mword kscratch_mask; /* Usable KScratch mask. */
    Mword gtoffset_size;
  };

  static int mips_fpu_disabled;
  static int mips_dsp_disabled;
  static cpuinfo_mips _cpuinfo;
};

namespace Segment
{
  enum Attribs_enum
  {
    Ks = 1UL << 30,
    Kp = 1UL << 29,
    N  = 1UL << 28,
    Default_attribs = Kp //Ks | Kp,
  };
};

# define cpu_has_mips_1		(Cpu::isa_level() & MIPS_CPU_ISA_I)
#ifndef cpu_has_mips_2
# define cpu_has_mips_2		(Cpu::isa_level() & MIPS_CPU_ISA_II)
#endif
#ifndef cpu_has_mips_3
# define cpu_has_mips_3		(Cpu::isa_level() & MIPS_CPU_ISA_III)
#endif
#ifndef cpu_has_mips_4
# define cpu_has_mips_4		(Cpu::isa_level() & MIPS_CPU_ISA_IV)
#endif
#ifndef cpu_has_mips_5
# define cpu_has_mips_5		(Cpu::isa_level() & MIPS_CPU_ISA_V)
#endif
# ifndef cpu_has_mips32r1
# define cpu_has_mips32r1	(Cpu::isa_level() & MIPS_CPU_ISA_M32R1)
# endif
# ifndef cpu_has_mips32r2
# define cpu_has_mips32r2	(Cpu::isa_level() & MIPS_CPU_ISA_M32R2)
# endif
# ifndef cpu_has_mips64r1
# define cpu_has_mips64r1	(Cpu::isa_level() & MIPS_CPU_ISA_M64R1)
# endif
# ifndef cpu_has_mips64r2
# define cpu_has_mips64r2	(Cpu::isa_level() & MIPS_CPU_ISA_M64R2)
# endif

/*
 * Shortcuts ...
 */
#define cpu_has_mips32	(cpu_has_mips32r1 | cpu_has_mips32r2)
#define cpu_has_mips64	(cpu_has_mips64r1 | cpu_has_mips64r2)
#define cpu_has_mips_r1 (cpu_has_mips32r1 | cpu_has_mips64r1)
#define cpu_has_mips_r2 (cpu_has_mips32r2 | cpu_has_mips64r2)
#define cpu_has_mips_r	(cpu_has_mips32r1 | cpu_has_mips32r2 | \
			 cpu_has_mips64r1 | cpu_has_mips64r2)

#ifndef cpu_has_mips_r2_exec_hazard
#define cpu_has_mips_r2_exec_hazard cpu_has_mips_r2
#endif

#define cpu_has_fpu       (Cpu::options() & MIPS_CPU_FPU)
#define cpu_has_tlb       (Cpu::options() & MIPS_CPU_TLB)
#define cpu_has_dsp       (Cpu::extensions() & MIPS_ASE_DSP)
#define cpu_has_dsp2      (Cpu::extensions() & MIPS_ASE_DSP2P)
#define cpu_has_vz        (Cpu::extensions() & MIPS_ASE_VZ)
#define cpu_has_vzguestid (Cpu::options() & MIPS_CPU_VZGUESTID)
#define cpu_has_vzvirtirq (Cpu::options() & MIPS_CPU_VZVIRTIRQ)
#define cpu_has_tlbinv    (Cpu::options() & MIPS_CPU_TLBINV)
#define cpu_has_badinstr  (Cpu::options() & MIPS_CPU_BADINSTR)
#define cpu_has_badinstrp (Cpu::options() & MIPS_CPU_BADINSTRP)
#define cpu_has_guestctl0ext (Cpu::has_guestctl0ext())
#define cpu_has_guestctl1 (Cpu::has_guestctl1())
#define cpu_has_guestctl2 (Cpu::has_guestctl2())

#define cpu_vz_has_tlb      (Cpu::vzoptions() & MIPS_CPU_TLB)
#define cpu_vz_has_config1  (Cpu::vzconfig0() & MIPS_CONF_M)
#define cpu_vz_has_config2  (Cpu::vzconfig1() & MIPS_CONF_M)
#define cpu_vz_has_config3  (Cpu::vzconfig2() & MIPS_CONF_M)
#define cpu_vz_has_config4  (Cpu::vzconfig3() & MIPS_CONF_M)
#define cpu_vz_has_config5  (Cpu::vzconfig4() & MIPS_CONF_M)
#define cpu_vz_has_config6  (0)
#define cpu_vz_has_config7  (1)

#ifndef cpu_has_clo_clz
#define cpu_has_clo_clz cpu_has_mips_r
#endif

#define cpu_has_veic        0
#ifndef cpu_has_veic
# define cpu_has_veic       (Cpu::options() & MIPS_CPU_VEIC)
#endif

#define cpu_has_vint        0
#ifndef cpu_has_vint
# define cpu_has_vint       (Cpu::options() & MIPS_CPU_VINT)
#endif

//------------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "panic.h"
#include "boot_info.h"
#include "div32.h"
#include "regdef.h"
#include "mipsregs.h"
#include "mipsvzregs.h"
#include "cpu-arch.h"
#include "processor.h"
#include "config.h"
#include "mem_unit.h"
#include "fpu.h"

#include <cstdio>

DEFINE_PER_CPU_P(0) Per_cpu<Cpu> Cpu::cpus(Per_cpu_data::Cpu_num);
Cpu *Cpu::_boot_cpu;
unsigned long Cpu::_ns_per_cycle;
int Cpu::mips_fpu_disabled = 0;
int Cpu::mips_dsp_disabled = 1;
Cpu::cpuinfo_mips Cpu::_cpuinfo;
PUBLIC static inline unsigned Cpu::phys_bits() { return 32; }

PRIVATE static
void Cpu::set_isa(struct cpuinfo_mips *c, unsigned int isa)
{
  switch (isa) {
  case MIPS_CPU_ISA_M64R2:
    c->isa_level |= MIPS_CPU_ISA_M32R2 | MIPS_CPU_ISA_M64R2;
  case MIPS_CPU_ISA_M64R1:
    c->isa_level |= MIPS_CPU_ISA_M32R1 | MIPS_CPU_ISA_M64R1;
  case MIPS_CPU_ISA_V:
    c->isa_level |= MIPS_CPU_ISA_V;
  case MIPS_CPU_ISA_IV:
    c->isa_level |= MIPS_CPU_ISA_IV;
  case MIPS_CPU_ISA_III:
    c->isa_level |= MIPS_CPU_ISA_I | MIPS_CPU_ISA_II |
        MIPS_CPU_ISA_III;
    break;

  case MIPS_CPU_ISA_M32R2:
    c->isa_level |= MIPS_CPU_ISA_M32R2;
  case MIPS_CPU_ISA_M32R1:
    c->isa_level |= MIPS_CPU_ISA_M32R1;
  case MIPS_CPU_ISA_II:
    c->isa_level |= MIPS_CPU_ISA_II;
  case MIPS_CPU_ISA_I:
    c->isa_level |= MIPS_CPU_ISA_I;
    break;
  }
}

PRIVATE
void
Cpu::identify()
{
  processor_id = read_c0_prid();

  switch (processor_id & 0xff0000) {
    case PRID_COMP_MIPS:
      switch (processor_id & 0xff00) {
        case PRID_IMP_4KC:
                name = "MIPS 4Kc";
                break;
        case PRID_IMP_4KEC:
        case PRID_IMP_4KECR2:
                name = "MIPS 4KEc";
                break;
        case PRID_IMP_4KSC:
        case PRID_IMP_4KSD:
                name = "MIPS 4KSc";
                break;
        case PRID_IMP_5KC:
                name = "MIPS 5Kc";
                break;
        case PRID_IMP_5KE:
                name = "MIPS 5KE";
                break;
        case PRID_IMP_20KC:
                name = "MIPS 20Kc";
                break;
        case PRID_IMP_24K:
                name = "MIPS 24Kc";
                break;
        case PRID_IMP_24KE:
                name = "MIPS 24KEc";
                break;
        case PRID_IMP_25KF:
                name = "MIPS 25Kc";
                break;
        case PRID_IMP_34K:
                name = "MIPS 34Kc";
                break;
        case PRID_IMP_74K:
                name = "MIPS 74Kc";
                break;
        case PRID_IMP_M14KC:
                name = "MIPS M14Kc";
                break;
        case PRID_IMP_M14KEC:
                name = "MIPS M14KEc";
                break;
        case PRID_IMP_1004K:
                name = "MIPS 1004Kc";
                break;
        case PRID_IMP_1074K:
                name = "MIPS 1074Kc";
                break;
        case PRID_IMP_INTERAPTIV_UP:
                name = "MIPS interAptiv";
                break;
        case PRID_IMP_INTERAPTIV_MP:
                name = "MIPS interAptiv (multi)";
                break;
        case PRID_IMP_PROAPTIV_UP:
                name = "MIPS proAptiv";
                break;
        case PRID_IMP_PROAPTIV_MP:
                name = "MIPS proAptiv (multi)";
                break;
        case PRID_IMP_P5600:
                name = "MIPS P5600";
                break;
        case PRID_IMP_M5150:
                name = "MIPS M5150";
                break;
	}
      break;
    default:
      panic("CPU not identified!");
  }
}

PRIVATE static
unsigned int Cpu::decode_config0(struct cpuinfo_mips *c)
{
	unsigned int config0;
	int isa;

	config0 = read_c0_config();

	if (((config0 & MIPS_CONF_MT) >> 7) == 1)
		c->options |= MIPS_CPU_TLB;

	isa = (config0 & MIPS_CONF_AT) >> 13;
	switch (isa) {
	case 0:
		switch ((config0 & MIPS_CONF_AR) >> 10) {
		case 0:
			set_isa(c, MIPS_CPU_ISA_M32R1);
			break;
		case 1:
			set_isa(c, MIPS_CPU_ISA_M32R2);
			break;
		default:
			goto unknown;
		}
		break;
	case 2:
		switch ((config0 & MIPS_CONF_AR) >> 10) {
		case 0:
			set_isa(c, MIPS_CPU_ISA_M64R1);
			break;
		case 1:
			set_isa(c, MIPS_CPU_ISA_M64R2);
			break;
		default:
			goto unknown;
		}
		break;
	default:
		goto unknown;
	}

	return config0 & MIPS_CONF_M;

unknown:
	panic("Unsupported ISA type");
}

PRIVATE static
unsigned int Cpu::decode_config1(struct cpuinfo_mips *c)
{
	unsigned int config1;

	config1 = read_c0_config1();

	if (config1 & MIPS_CONF1_MD)
		c->ases |= MIPS_ASE_MDMX;
	if (config1 & MIPS_CONF1_WR)
		c->options |= MIPS_CPU_WATCH;
	if (config1 & MIPS_CONF1_CA)
		c->ases |= MIPS_ASE_MIPS16;
	if (config1 & MIPS_CONF1_EP)
		c->options |= MIPS_CPU_EJTAG;
	if (config1 & MIPS_CONF1_FP) {
		c->options |= MIPS_CPU_FPU;
		c->options |= MIPS_CPU_32FPR;
	}
	if (cpu_has_tlb)
		c->tlbsize = ((config1 & MIPS_CONF1_TLBS) >> 25) + 1;

	return config1 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_config2(struct cpuinfo_mips *c)
{
	unsigned int config2;
	(void)c;

	config2 = read_c0_config2();

#if 0
	if (config2 & MIPS_CONF2_SL)
		c->scache.flags &= ~MIPS_CACHE_NOT_PRESENT;
#endif

	return config2 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_config3(struct cpuinfo_mips *c)
{
	unsigned int config3;

	config3 = read_c0_config3();

	if (config3 & MIPS_CONF3_SM) {
		c->ases |= MIPS_ASE_SMARTMIPS;
		c->options |= MIPS_CPU_RIXI;
	}
	if (config3 & MIPS_CONF3_RXI)
		c->options |= MIPS_CPU_RIXI;
	if (config3 & MIPS_CONF3_DSP)
		c->ases |= MIPS_ASE_DSP;
	if (config3 & MIPS_CONF3_DSP2P)
		c->ases |= MIPS_ASE_DSP2P;
	if (config3 & MIPS_CONF3_VINT)
		c->options |= MIPS_CPU_VINT;
	if (config3 & MIPS_CONF3_VEIC)
		c->options |= MIPS_CPU_VEIC;
	if (config3 & MIPS_CONF3_MT)
		c->ases |= MIPS_ASE_MIPSMT;
	if (config3 & MIPS_CONF3_ULRI)
		c->options |= MIPS_CPU_ULRI;
	if (config3 & MIPS_CONF3_ISA)
		c->options |= MIPS_CPU_MICROMIPS;
	if (config3 & MIPS_CONF3_BI)
		c->options |= MIPS_CPU_BADINSTR;
	if (config3 & MIPS_CONF3_BP)
		c->options |= MIPS_CPU_BADINSTRP;
#if 0
#ifdef CONFIG_CPU_MICROMIPS
	write_c0_config3(read_c0_config3() | MIPS_CONF3_ISA_OE);
#endif
#endif
	if (config3 & MIPS_CONF3_VZ)
		c->ases |= MIPS_ASE_VZ;

	return config3 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_config4(struct cpuinfo_mips *c)
{
	unsigned int config4;

	config4 = read_c0_config4();

	if ((config4 & MIPS_CONF4_MMUEXTDEF) == MIPS_CONF4_MMUEXTDEF_MMUSIZEEXT
	    && cpu_has_tlb)
		c->tlbsize += (config4 & MIPS_CONF4_MMUSIZEEXT) * 0x40;

	c->kscratch_mask = (config4 >> 16) & 0xff;

	if (config4 & MIPS_CONF4_IE)
		if (config4 & MIPS_CONF4_TLBINV)
			c->options |= MIPS_CPU_TLBINV;

	return config4 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_config5(struct cpuinfo_mips *c)
{
	unsigned int config5;
	(void)c;

	config5 = read_c0_config5();
        // disable user mode control of Status.FR 64bit FPU mode
	config5 &= ~(MIPS_CONF5_UFR | MIPS_CONF5_UFE | MIPS_CONF5_FRE);
	write_c0_config5(config5);

	return config5 & MIPS_CONF_M;
}

PRIVATE static
void Cpu::decode_configs(struct cpuinfo_mips *c)
{
	int ok;

	/* MIPS32 or MIPS64 compliant CPU.  */
	c->options = MIPS_CPU_4KEX | MIPS_CPU_4K_CACHE | MIPS_CPU_COUNTER |
		     MIPS_CPU_DIVEC | MIPS_CPU_LLSC | MIPS_CPU_MCHECK;

#if 0
	c->scache.flags = MIPS_CACHE_NOT_PRESENT;
#endif

	ok = decode_config0(c);			/* Read Config registers.  */
	//BUG_ON(!ok);				      /* Arch spec violation!	 */
	if (ok)
		ok = decode_config1(c);
	if (ok)
		ok = decode_config2(c);
	if (ok)
		ok = decode_config3(c);
	if (ok)
		ok = decode_config4(c);
	if (ok)
		ok = decode_config5(c);

	if (cpu_has_mips_r2)
		c->core = read_c0_ebase() & 0x3ff;

	probe_vz_ase(c);

	if (mips_fpu_disabled)
		c->options &= ~MIPS_CPU_FPU;

	if (mips_dsp_disabled)
		c->ases &= ~(MIPS_ASE_DSP | MIPS_ASE_DSP2P);
}

PRIVATE static
unsigned int Cpu::decode_vz_config0(struct cpuinfo_mips *c)
{
	unsigned int config0;

	config0 = read_c0_guest_config();

	if (((config0 & MIPS_CONF_MT) >> 7) == 1)
		c->vz.options |= MIPS_CPU_TLB;

	c->vz.config0 = config0;
	return config0 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_vz_config1(struct cpuinfo_mips *c)
{
	unsigned int config1;

	config1 = read_c0_guest_config1();

	if (cpu_vz_has_tlb)
		c->vz.tlbsize = ((config1 & MIPS_CONF1_TLBS) >> 25) + 1;

	c->vz.config1 = config1;
	return config1 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_vz_config2(struct cpuinfo_mips *c)
{
	c->vz.config2 = read_c0_guest_config2();
	return c->vz.config2 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_vz_config3(struct cpuinfo_mips *c)
{
	c->vz.config3 = read_c0_guest_config3();
	return c->vz.config3 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_vz_config4(struct cpuinfo_mips *c)
{
	unsigned int config4;

	config4 = read_c0_guest_config4();

	if ((config4 & MIPS_CONF4_MMUEXTDEF) == MIPS_CONF4_MMUEXTDEF_MMUSIZEEXT
	    && cpu_vz_has_tlb)
		c->vz.tlbsize += (config4 & MIPS_CONF4_MMUSIZEEXT) * 0x40;

	c->vz.config4 = config4;
	return config4 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_vz_config5(struct cpuinfo_mips *c)
{
	unsigned int config5;

	config5 = read_c0_guest_config5();
        // disable user mode control of Status.FR 64bit FPU mode
	config5 &= ~(MIPS_CONF5_UFR | MIPS_CONF5_UFE | MIPS_CONF5_FRE);
        // no cop0 MAAR or MAARI support (MRP is optionally root-writable)
        config5 &= ~MIPS_CONF5_MRP;
        write_c0_guest_config5(config5);

	c->vz.config5 = config5;
	return config5 & MIPS_CONF_M;
}

PRIVATE static
unsigned int Cpu::decode_vz_config6(struct cpuinfo_mips *c)
{
	c->vz.config6 = read_c0_guest_config6();
	return 1;
}

PRIVATE static
unsigned int Cpu::decode_vz_config7(struct cpuinfo_mips *c)
{
	c->vz.config7 = read_c0_guest_config7();
	return 1;
}

PRIVATE static
void Cpu::decode_vz_configs(struct cpuinfo_mips *c)
{
	int ok;

	ok = decode_vz_config0(c);		/* Read Config registers.  */
	//BUG_ON(!ok);			/* Arch spec violation!  */
	if (ok)
		ok = decode_vz_config1(c);
	if (ok)
		ok = decode_vz_config2(c);
	if (ok)
		ok = decode_vz_config3(c);
	if (ok)
		ok = decode_vz_config4(c);
	if (ok)
		ok = decode_vz_config5(c);

	decode_vz_config6(c);
	decode_vz_config7(c);
}

PRIVATE static
void Cpu::probe_vz_guestid(struct cpuinfo_mips *c)
{
	if ((read_c0_guestctl0() & (GUESTCTL0_G1 | GUESTCTL0_RAD))
	    == GUESTCTL0_G1)
	{
		c->options |= MIPS_CPU_VZGUESTID;

		write_c0_guestctl1(0xff);
		back_to_back_c0_hazard();
		c->vzguestid_mask = read_c0_guestctl1() & 0xff;
		write_c0_guestctl1(0);
	}
}

//------------------------------------------------------------------------------
IMPLEMENTATION [mips32 && !mips_vz]:

PRIVATE static
void Cpu::validate_vz_config(struct cpuinfo_mips *)
{
}

//------------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mips_vz]:

PRIVATE static
void Cpu::validate_vz_config(struct cpuinfo_mips *c)
{
  if (!cpu_has_vz)
    panic("Running kernel with CONFIG_MIPS_VZ requires hw virtualization support.\n");

  if ((c->kscratch_mask & 0xc) != 0xc)
    panic("Running kernel with CONFIG_MIPS_VZ requires KScratch1 and KScratch2 support.\n");

}

//------------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

int
clo(int x)
{
    int ones;

    __asm__ __volatile("clo    %0, %1":"=r"(ones)
                       :"r"(x)
        );
    return ones;
}

PRIVATE static
void Cpu::probe_vz_ase(struct cpuinfo_mips *c)
{
        Mword guestctl0 = 0;

        validate_vz_config(c);

	if (!cpu_has_vz)
		return;

	probe_vz_guestid(c);

        guestctl0 = read_c0_guestctl0();
        c->has_guestctl0ext = (guestctl0 & GUESTCTL0_G0E) != 0;
        c->has_guestctl1 = (guestctl0 & GUESTCTL0_G1) != 0;
        c->has_guestctl2 = (guestctl0 & GUESTCTL0_G2) != 0;

	if (guestctl0 & GUESTCTL0_G2)
		c->options |= MIPS_CPU_VZVIRTIRQ;

	decode_vz_configs(c);

    write_c0_gtoffset(0xffffffff);
    back_to_back_c0_hazard();
    Mword gtoffset = read_c0_gtoffset();
    c->gtoffset_size = clo(gtoffset);
    printf("GTOffset Size: %lu bits\n", c->gtoffset_size);
    
}

PUBLIC
bool
Cpu::if_show_infos() const
{
  return id() == Cpu_number::boot_cpu() || !boot_cpu();
}

PUBLIC
void
Cpu::print_infos() const
{
  if (if_show_infos())
    printf("CPU[%u]: %s at %lluMHz (%d TLBs)\n\n",
      cxx::int_value<Cpu_number>(id()),
      name,
      div32(frequency(), 1000000),
      tlbsize());

  if (!(_cpuinfo.options &  MIPS_CPU_ULRI))
    panic("XXX UserLocal register not supported, used for TLS XXX");
}

PUBLIC static inline
const char *
Cpu::vz_str()
{
  return cpu_has_vz ? "R" : "";
}

IMPLEMENT
void
Cpu::init(bool is_boot_cpu)
{
  identify();
  if (is_boot_cpu)
    {
      _boot_cpu = this;
      set_present(1);
      set_online(1);
    }
  _phys_id = Proc::cpu_id();
  _ns_per_cycle = 1000000000 / (frequency());
  print_infos();

  /* Set the HW Enable register to allow rdhwr access from UM */
  write_c0_hwrena(0x2000000F);
}

PUBLIC inline
Cpu_phys_id
Cpu::phys_id() const
{ return _phys_id; }

IMPLEMENT
void
Cpu::early_init()
{
#ifndef CONFIG_FPU
  mips_fpu_disabled = 1;
#endif
  decode_configs(&_cpuinfo);

  /* Flush TLBs */
  Mem_unit::tlb_flush_slow();
}

PUBLIC static inline
Mword
Cpu::stack_align(Mword stack)
{ return stack & ~0xf; }

//------------------------------------------------------------------------------
/* Time functions */

/**
 * Read time base registers 
 */
PUBLIC static inline
Unsigned64
Cpu::rdtsc()
{
  Unsigned64 tb = 0x0ULL;
  return tb;
}

PUBLIC static inline
void
Cpu::busy_wait_ns(Unsigned64 ns)
{
  Unsigned64 stop = rdtsc() + ns_to_tsc(ns);

  while(rdtsc() <  stop) 
    ;
}

PUBLIC static inline
Unsigned64
Cpu::ns_to_tsc(Unsigned64 ns)
{
  return ns / _ns_per_cycle;
}

PUBLIC static inline
Unsigned64
Cpu::tsc_to_ns(Unsigned64 tsc)
{
  return tsc * _ns_per_cycle;
}

PUBLIC static inline
Unsigned32
Cpu::get_scaler_tsc_to_ns()
{ return 0; }

PUBLIC static inline
Unsigned32
Cpu::get_scaler_tsc_to_us() 
{ return 0; }

PUBLIC static inline
Unsigned32 
Cpu::get_scaler_ns_to_tsc() 
{ return 0; }

PUBLIC static inline
bool
Cpu::tsc()
{ return 0; }

//------------------------------------------------------------------------------
/* Unimplemented */

PUBLIC static inline
void
Cpu::debugctl_enable()
{}

PUBLIC static inline
void
Cpu::debugctl_disable()
{}

