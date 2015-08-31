/*
 * Included from linux arch/mips/include/asm/cpu-features.h
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2003, 2004 Ralf Baechle
 * Copyright (C) 2004  Maciej W. Rozycki
 */
#ifndef __ASM_CPU_FEATURES_H
#define __ASM_CPU_FEATURES_H

#define LINUX_ASM_CPU_FEATURES_FOR_FIASCO

#ifdef LINUX_ASM_CPU_FEATURES_FOR_FIASCO
#include "globalconfig.h"
#include "cpu.h"
#include "cpu-arch.h"

#define cpu_has_vzguestid (Cpu::options() & MIPS_CPU_VZGUESTID)
#define cpu_has_vzvirtirq (Cpu::options() & MIPS_CPU_VZVIRTIRQ)
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

#else
#include <asm/cpu.h>
#include <asm/cpu-info.h>
#include <cpu-feature-overrides.h>
#endif /* LINUX_ASM_CPU_FEATURES_FOR_FIASCO */

/*
 * SMP assumption: Options of CPU 0 are a superset of all processors.
 * This is true for all known MIPS systems.
 */
#ifndef cpu_has_tlb
#define cpu_has_tlb		(Cpu::options() & MIPS_CPU_TLB)
#endif
#ifndef cpu_has_tlbinv
#define cpu_has_tlbinv		(Cpu::options() & MIPS_CPU_TLBINV)
#endif
#ifndef cpu_has_segments
#define cpu_has_segments	(Cpu::options() & MIPS_CPU_SEGMENTS)
#endif
#ifndef cpu_has_eva
#define cpu_has_eva		(Cpu::options() & MIPS_CPU_EVA)
#endif
#ifndef cpu_has_htw
#define cpu_has_htw		(Cpu::options() & MIPS_CPU_HTW)
#endif
#ifndef cpu_has_rixiex
#define cpu_has_rixiex		(Cpu::options() & MIPS_CPU_RIXIEX)
#endif
#ifndef cpu_has_maar
#define cpu_has_maar		(Cpu::options() & MIPS_CPU_MAAR)
#endif
#ifndef cpu_has_rw_llb
#define cpu_has_rw_llb		(Cpu::options() & MIPS_CPU_RW_LLB)
#endif

/*
 * For the moment we don't consider R6000 and R8000 so we can assume that
 * anything that doesn't support R4000-style exceptions and interrupts is
 * R3000-like.  Users should still treat these two macro definitions as
 * opaque.
 */
#ifndef cpu_has_3kex
#define cpu_has_3kex		(!cpu_has_4kex)
#endif
#ifndef cpu_has_4kex
#define cpu_has_4kex		(Cpu::options() & MIPS_CPU_4KEX)
#endif
#ifndef cpu_has_3k_cache
#define cpu_has_3k_cache	(Cpu::options() & MIPS_CPU_3K_CACHE)
#endif
#define cpu_has_6k_cache	0
#define cpu_has_8k_cache	0
#ifndef cpu_has_4k_cache
#define cpu_has_4k_cache	(Cpu::options() & MIPS_CPU_4K_CACHE)
#endif
#ifndef cpu_has_tx39_cache
#define cpu_has_tx39_cache	(Cpu::options() & MIPS_CPU_TX39_CACHE)
#endif
#ifndef cpu_has_octeon_cache
#define cpu_has_octeon_cache	0
#endif
/* Don't override `cpu_has_fpu' to 1 or the "nofpu" option won't work.  */
#ifndef cpu_has_fpu
#ifdef LINUX_ASM_CPU_FEATURES_FOR_FIASCO
#define cpu_has_fpu	(Cpu::options() & MIPS_CPU_FPU)
#else
#define cpu_has_fpu		(current_cpu_data.options & MIPS_CPU_FPU)
#define raw_cpu_has_fpu		(raw_current_cpu_data.options & MIPS_CPU_FPU)
#endif /* LINUX_ASM_CPU_FEATURES_FOR_FIASCO */
#else
#define raw_cpu_has_fpu		cpu_has_fpu
#endif
#ifndef cpu_has_32fpr
#define cpu_has_32fpr		(Cpu::options() & MIPS_CPU_32FPR)
#endif
#ifndef cpu_has_counter
#define cpu_has_counter		(Cpu::options() & MIPS_CPU_COUNTER)
#endif
#ifndef cpu_has_watch
#define cpu_has_watch		(Cpu::options() & MIPS_CPU_WATCH)
#endif
#ifndef cpu_has_divec
#define cpu_has_divec		(Cpu::options() & MIPS_CPU_DIVEC)
#endif
#ifndef cpu_has_vce
#define cpu_has_vce		(Cpu::options() & MIPS_CPU_VCE)
#endif
#ifndef cpu_has_cache_cdex_p
#define cpu_has_cache_cdex_p	(Cpu::options() & MIPS_CPU_CACHE_CDEX_P)
#endif
#ifndef cpu_has_cache_cdex_s
#define cpu_has_cache_cdex_s	(Cpu::options() & MIPS_CPU_CACHE_CDEX_S)
#endif
#ifndef cpu_has_prefetch
#define cpu_has_prefetch	(Cpu::options() & MIPS_CPU_PREFETCH)
#endif
#ifndef cpu_has_mcheck
#define cpu_has_mcheck		(Cpu::options() & MIPS_CPU_MCHECK)
#endif
#ifndef cpu_has_ejtag
#define cpu_has_ejtag		(Cpu::options() & MIPS_CPU_EJTAG)
#endif
#ifndef cpu_has_llsc
#define cpu_has_llsc		(Cpu::options() & MIPS_CPU_LLSC)
#endif
#ifndef cpu_has_bp_ghist
#define cpu_has_bp_ghist	(Cpu::options() & MIPS_CPU_BP_GHIST)
#endif
#ifndef kernel_uses_llsc
#define kernel_uses_llsc	cpu_has_llsc
#endif
#ifndef cpu_has_mips16
#define cpu_has_mips16		(Cpu::ases() & MIPS_ASE_MIPS16)
#endif
#ifndef cpu_has_mdmx
#define cpu_has_mdmx		(Cpu::ases() & MIPS_ASE_MDMX)
#endif
#ifndef cpu_has_mips3d
#define cpu_has_mips3d		(Cpu::ases() & MIPS_ASE_MIPS3D)
#endif
#ifndef cpu_has_smartmips
#define cpu_has_smartmips	(Cpu::ases() & MIPS_ASE_SMARTMIPS)
#endif

#ifndef cpu_has_rixi
# ifdef CONFIG_64BIT
# define cpu_has_rixi		(Cpu::options() & MIPS_CPU_RIXI)
# else /* CONFIG_32BIT */
# define cpu_has_rixi		((Cpu::options() & MIPS_CPU_RIXI) && !cpu_has_64bits)
# endif
#endif

#ifndef cpu_has_mmips
# ifdef CONFIG_SYS_SUPPORTS_MICROMIPS
#  define cpu_has_mmips		(Cpu::options() & MIPS_CPU_MICROMIPS)
# else
#  define cpu_has_mmips		0
# endif
#endif

#ifndef cpu_has_xpa
#define cpu_has_xpa		(Cpu::options() & MIPS_CPU_XPA)
#endif
#ifndef cpu_has_vtag_icache
#define cpu_has_vtag_icache	(Cpu::icache().flags & MIPS_CACHE_VTAG)
#endif
#ifndef cpu_has_dc_aliases
#define cpu_has_dc_aliases	(Cpu::dcache().flags & MIPS_CACHE_ALIASES)
#endif
#ifndef cpu_has_ic_fills_f_dc
#define cpu_has_ic_fills_f_dc	(Cpu::icache().flags & MIPS_CACHE_IC_F_DC)
#endif
#ifndef cpu_has_pindexed_dcache
#define cpu_has_pindexed_dcache	(Cpu::dcache().flags & MIPS_CACHE_PINDEX)
#endif
#ifndef cpu_has_local_ebase
#define cpu_has_local_ebase	1
#endif

/*
 * I-Cache snoops remote store.	 This only matters on SMP.  Some multiprocessors
 * such as the R10000 have I-Caches that snoop local stores; the embedded ones
 * don't.  For maintaining I-cache coherency this means we need to flush the
 * D-cache all the way back to whever the I-cache does refills from, so the
 * I-cache has a chance to see the new data at all.  Then we have to flush the
 * I-cache also.
 * Note we may have been rescheduled and may no longer be running on the CPU
 * that did the store so we can't optimize this into only doing the flush on
 * the local CPU.
 */
#ifndef cpu_icache_snoops_remote_store
#ifdef CONFIG_SMP
#define cpu_icache_snoops_remote_store	(Cpu::icache().flags & MIPS_IC_SNOOPS_REMOTE)
#else
#define cpu_icache_snoops_remote_store	1
#endif
#endif

#ifndef cpu_has_mips_1
# define cpu_has_mips_1		(!cpu_has_mips_r6)
#endif
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
#ifndef cpu_has_mips32r1
# define cpu_has_mips32r1	(Cpu::isa_level() & MIPS_CPU_ISA_M32R1)
#endif
#ifndef cpu_has_mips32r2
# define cpu_has_mips32r2	(Cpu::isa_level() & MIPS_CPU_ISA_M32R2)
#endif
#ifndef cpu_has_mips32r6
# define cpu_has_mips32r6	(Cpu::isa_level() & MIPS_CPU_ISA_M32R6)
#endif
#ifndef cpu_has_mips64r1
# define cpu_has_mips64r1	(Cpu::isa_level() & MIPS_CPU_ISA_M64R1)
#endif
#ifndef cpu_has_mips64r2
# define cpu_has_mips64r2	(Cpu::isa_level() & MIPS_CPU_ISA_M64R2)
#endif
#ifndef cpu_has_mips64r6
# define cpu_has_mips64r6	(Cpu::isa_level() & MIPS_CPU_ISA_M64R6)
#endif

/*
 * Shortcuts ...
 */
#define cpu_has_mips_2_3_4_5	(cpu_has_mips_2 | cpu_has_mips_3_4_5)
#define cpu_has_mips_3_4_5	(cpu_has_mips_3 | cpu_has_mips_4_5)
#define cpu_has_mips_4_5	(cpu_has_mips_4 | cpu_has_mips_5)

#define cpu_has_mips_2_3_4_5_r	(cpu_has_mips_2 | cpu_has_mips_3_4_5_r)
#define cpu_has_mips_3_4_5_r	(cpu_has_mips_3 | cpu_has_mips_4_5_r)
#define cpu_has_mips_4_5_r	(cpu_has_mips_4 | cpu_has_mips_5_r)
#define cpu_has_mips_5_r	(cpu_has_mips_5 | cpu_has_mips_r)

#define cpu_has_mips_3_4_5_64_r2_r6					\
				(cpu_has_mips_3 | cpu_has_mips_4_5_64_r2_r6)
#define cpu_has_mips_4_5_64_r2_r6					\
				(cpu_has_mips_4_5 | cpu_has_mips64r1 |	\
				 cpu_has_mips_r2 | cpu_has_mips_r6)

#define cpu_has_mips32	(cpu_has_mips32r1 | cpu_has_mips32r2 | cpu_has_mips32r6)
#define cpu_has_mips64	(cpu_has_mips64r1 | cpu_has_mips64r2 | cpu_has_mips64r6)
#define cpu_has_mips_r1 (cpu_has_mips32r1 | cpu_has_mips64r1)
#define cpu_has_mips_r2 (cpu_has_mips32r2 | cpu_has_mips64r2)
#define cpu_has_mips_r6	(cpu_has_mips32r6 | cpu_has_mips64r6)
#define cpu_has_mips_r	(cpu_has_mips32r1 | cpu_has_mips32r2 | \
			 cpu_has_mips32r6 | cpu_has_mips64r1 | \
			 cpu_has_mips64r2 | cpu_has_mips64r6)

/* MIPSR2 and MIPSR6 have a lot of similarities */
#define cpu_has_mips_r2_r6	(cpu_has_mips_r2 | cpu_has_mips_r6)

/*
 * cpu_has_mips_r2_exec_hazard - return if IHB is required on current processor
 *
 * Returns non-zero value if the current processor implementation requires
 * an IHB instruction to deal with an instruction hazard as per MIPS R2
 * architecture specification, zero otherwise.
 */
#ifndef cpu_has_mips_r2_exec_hazard
#define cpu_has_mips_r2_exec_hazard					\
({									\
	int __res;							\
									\
	switch (current_cpu_type()) {					\
	case CPU_M14KC:							\
	case CPU_74K:							\
	case CPU_1074K:							\
	case CPU_PROAPTIV:						\
	case CPU_P5600:							\
	case CPU_M5150:							\
	case CPU_QEMU_GENERIC:						\
	case CPU_CAVIUM_OCTEON:						\
	case CPU_CAVIUM_OCTEON_PLUS:					\
	case CPU_CAVIUM_OCTEON2:					\
	case CPU_CAVIUM_OCTEON3:					\
		__res = 0;						\
		break;							\
									\
	default:							\
		__res = 1;						\
	}								\
									\
	__res;								\
})
#endif

/*
 * MIPS32, MIPS64, VR5500, IDT32332, IDT32334 and maybe a few other
 * pre-MIPS32/MIPS64 processors have CLO, CLZ.	The IDT RC64574 is 64-bit and
 * has CLO and CLZ but not DCLO nor DCLZ.  For 64-bit kernels
 * cpu_has_clo_clz also indicates the availability of DCLO and DCLZ.
 */
#ifndef cpu_has_clo_clz
#define cpu_has_clo_clz	cpu_has_mips_r
#endif

/*
 * MIPS32 R2, MIPS64 R2, Loongson 3A and Octeon have WSBH.
 * MIPS64 R2, Loongson 3A and Octeon have WSBH, DSBH and DSHD.
 * This indicates the availability of WSBH and in case of 64 bit CPUs also
 * DSBH and DSHD.
 */
#ifndef cpu_has_wsbh
#define cpu_has_wsbh		cpu_has_mips_r2
#endif

#ifndef cpu_has_dsp
#define cpu_has_dsp		(Cpu::ases() & MIPS_ASE_DSP)
#endif

#ifndef cpu_has_dsp2
#define cpu_has_dsp2		(Cpu::ases() & MIPS_ASE_DSP2P)
#endif

#ifndef cpu_has_mipsmt
#define cpu_has_mipsmt		(Cpu::ases() & MIPS_ASE_MIPSMT)
#endif

#ifndef cpu_has_userlocal
#define cpu_has_userlocal	(Cpu::options() & MIPS_CPU_ULRI)
#endif

#ifdef CONFIG_32BIT
# ifndef cpu_has_nofpuex
# define cpu_has_nofpuex	(Cpu::options() & MIPS_CPU_NOFPUEX)
# endif
# ifndef cpu_has_64bits
# define cpu_has_64bits		(Cpu::isa_level() & MIPS_CPU_ISA_64BIT)
# endif
# ifndef cpu_has_64bit_zero_reg
# define cpu_has_64bit_zero_reg	(Cpu::isa_level() & MIPS_CPU_ISA_64BIT)
# endif
# ifndef cpu_has_64bit_gp_regs
# define cpu_has_64bit_gp_regs		0
# endif
# ifndef cpu_has_64bit_addresses
# define cpu_has_64bit_addresses	0
# endif
# ifndef cpu_vmbits
# define cpu_vmbits 31
# endif
#endif

#ifdef CONFIG_64BIT
# ifndef cpu_has_nofpuex
# define cpu_has_nofpuex		0
# endif
# ifndef cpu_has_64bits
# define cpu_has_64bits			1
# endif
# ifndef cpu_has_64bit_zero_reg
# define cpu_has_64bit_zero_reg		1
# endif
# ifndef cpu_has_64bit_gp_regs
# define cpu_has_64bit_gp_regs		1
# endif
# ifndef cpu_has_64bit_addresses
# define cpu_has_64bit_addresses	1
# endif
# ifndef cpu_vmbits
# define cpu_vmbits Cpu::vmbits()
# define __NEED_VMBITS_PROBE
# endif
#endif

#if defined(CONFIG_CPU_MIPSR2_IRQ_VI) && !defined(cpu_has_vint)
# define cpu_has_vint		(Cpu::options() & MIPS_CPU_VINT)
#elif !defined(cpu_has_vint)
# define cpu_has_vint			0
#endif

#if defined(CONFIG_CPU_MIPSR2_IRQ_EI) && !defined(cpu_has_veic)
# define cpu_has_veic		(Cpu::options() & MIPS_CPU_VEIC)
#elif !defined(cpu_has_veic)
# define cpu_has_veic			0
#endif

#ifndef cpu_has_inclusive_pcaches
#define cpu_has_inclusive_pcaches	(Cpu::options() & MIPS_CPU_INCLUSIVE_CACHES)
#endif

#ifndef cpu_dcache_line_size
#define cpu_dcache_line_size()	Cpu::dcache().linesz
#endif
#ifndef cpu_icache_line_size
#define cpu_icache_line_size()	Cpu::icache().linesz
#endif
#ifndef cpu_scache_line_size
#define cpu_scache_line_size()	Cpu::scache().linesz
#endif

#ifndef cpu_hwrena_impl_bits
#define cpu_hwrena_impl_bits		0
#endif

#ifndef cpu_has_perf_cntr_intr_bit
#define cpu_has_perf_cntr_intr_bit	(Cpu::options() & MIPS_CPU_PCI)
#endif

#ifndef cpu_has_vz
#define cpu_has_vz		(Cpu::ases() & MIPS_ASE_VZ)
#endif

#if defined(CONFIG_CPU_HAS_MSA) && !defined(cpu_has_msa)
# define cpu_has_msa		(Cpu::ases() & MIPS_ASE_MSA)
#elif !defined(cpu_has_msa)
# define cpu_has_msa		0
#endif

#ifndef cpu_has_fre
# define cpu_has_fre		(Cpu::options() & MIPS_CPU_FRE)
#endif

#ifndef cpu_has_cdmm
# define cpu_has_cdmm		(Cpu::options() & MIPS_CPU_CDMM)
#endif

#endif /* __ASM_CPU_FEATURES_H */
