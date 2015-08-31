/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "mem_layout.h"
#include "tlb_entry.h"
#include "mipsvzregs.h"
#include "mmu.h"

extern "C" void mips32_ClearGuestRID(void);
extern "C" void mips32_SetGuestRID(Mword guestRID);
extern "C" void mips32_SetGuestID(Mword guestID);

class Mem_unit : public Mmu< Mem_layout::Cache_flush_area >
{
public:
  enum : Mword
  {
    Asid_invalid = ~0UL,
    Vz_root_guestid = GUESTCTL1_VZ_ROOT_GUESTID,
    FLUSH_ALL_GUESTIDS = 0,
  };
};


//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "globalconfig.h"
#include "mipsregs.h"
#include "mipsvzregs.h"
#include "processor.h"
#include "cpu.h"
#include "mem_layout.h"
#include "warn.h"

#define UNIQUE_ENTRYHI(idx) (cpu_has_tlbinv ? ((CKSEG0 + ((idx) << (Tlb_entry::PageShift + 1))) | MIPS_ENTRYHI_EHINV) : \
            (CKSEG0 + ((idx) << (Tlb_entry::PageShift + 1))))

#define ENTER_CRITICAL(_f)  ((_f) = Proc::cli_save())
#define EXIT_CRITICAL(_f)   (Proc::sti_restore((_f)))

PUBLIC static inline
void Mem_unit::change_asid(unsigned long asid)
{
  write_c0_entryhi(0UL | asid);
}

PUBLIC static
void Mem_unit::tlb_flush(unsigned long asid)
{
  if (asid == Asid_invalid)
    return;

  Proc::Status flags;
  Mword old_entryhi;
  Mword entryhi;
  Mword entrylo0;
  Mword entrylo1;
  Signed32 tlbsize = Cpu::tlbsize();
  Signed32 idx;

  ENTER_CRITICAL(flags);
  /* Save old context */
  old_entryhi = read_c0_entryhi();

  idx = read_c0_wired();

  if (cpu_has_tlbinv && !idx) {
    /* Use TLBINV instruction if no wired entries need to be preserved */
    write_c0_entryhi(asid);
    mtc0_tlbw_hazard();
    tlb_invalidate_asid();
    goto out;
  }

  /* invalidate matching entries */
  while (idx < tlbsize) {

    write_c0_index(idx);
    mtc0_tlbw_hazard();
    tlb_read();
    tlbw_use_hazard();
    entryhi = read_c0_entryhi();
    entrylo0 = read_c0_entrylo0();
    entrylo1 = read_c0_entrylo1();
    if (((entryhi & Tlb_entry::AsidMask) == asid) &&
        !(entrylo0 & Tlb_entry::Global) &&
        !(entrylo1 & Tlb_entry::Global))
    {
      /* Make sure all entries differ. */
      write_c0_entryhi(UNIQUE_ENTRYHI(idx));
      write_c0_entrylo0(0);
      write_c0_entrylo1(0);
      write_c0_index(idx);
      mtc0_tlbw_hazard();
      tlb_write_indexed();
    }
    idx++;
  }
out:
  tlbw_use_hazard();
  write_c0_entryhi(old_entryhi);
  EXIT_CRITICAL(flags);
}

PUBLIC static
void Mem_unit::tlb_flush(void)
{
  Proc::Status flags;
  Mword old_entryhi;
  Signed32 tlbsize = Cpu::tlbsize();
  Signed32 idx;

  idx = read_c0_wired();

  if (cpu_has_tlbinv && !idx) {

    ENTER_CRITICAL(flags);

    /* Use TLBINVF instruction if no wired entries need to be preserved */
    write_c0_index(0);
    mtc0_tlbw_hazard();
    tlb_invalidate_flush();
    tlbw_use_hazard();

    EXIT_CRITICAL(flags);
    return;
  }

  ENTER_CRITICAL(flags);

  /* Save old context and create impossible VPN2 value */
  old_entryhi = read_c0_entryhi();
  write_c0_entrylo0(0);
  write_c0_entrylo1(0);
  write_c0_pagemask(0);

  /* Blast 'em all away. */
  while (idx < tlbsize) {
    /* Make sure all entries differ. */
    write_c0_entryhi(UNIQUE_ENTRYHI(idx));
    write_c0_index(idx);
    mtc0_tlbw_hazard();
    tlb_write_indexed();
    idx++;
  }
  write_c0_entryhi(old_entryhi);
  tlbw_use_hazard();
  EXIT_CRITICAL(flags);
}


/*
 * Invalidate all root TLB entries, slower, safer version than tlb_flush(void)
 *
 * Calls TLB Probe to check for an existing match rather than relying on the
 * UNIQUE_ENTRYHI convention for avoiding machine check exceptions. Useful when
 * initial state of the TLB is unknown.
 *
 */
PUBLIC static
void Mem_unit::tlb_flush_slow(void)
{
  Proc::Status flags;
  Mword old_entryhi;
  Signed32 tlbsize = Cpu::tlbsize();
  Signed32 idx_adjust = Cpu::tlbsize() + 1;
  Signed32 idx;

  idx = read_c0_wired();

  if (cpu_has_tlbinv && !idx) {

    ENTER_CRITICAL(flags);

    /* Use TLBINVF instruction if no wired entries need to be preserved */
    write_c0_index(0);
    mtc0_tlbw_hazard();
    tlb_invalidate_flush();
    tlbw_use_hazard();

    EXIT_CRITICAL(flags);
    return;
  }

  ENTER_CRITICAL(flags);

  /* Save old context and create impossible VPN2 value */
  old_entryhi = read_c0_entryhi();
  write_c0_entrylo0(0);
  write_c0_entrylo1(0);
  write_c0_pagemask(0);

  /* Blast 'em all away. */
  while (idx < tlbsize) {
    Signed32 probe_idx;

    /* Make sure all entries differ. */
    write_c0_entryhi(UNIQUE_ENTRYHI(idx + idx_adjust));
    mtc0_tlbw_hazard();
    tlb_probe();
    tlb_probe_hazard();
    probe_idx = read_c0_index();
    if (probe_idx > 0) {
      /* entry is not unique, retry */
      idx_adjust++;
      continue;
    }
    write_c0_index(idx);
    mtc0_tlbw_hazard();
    tlb_write_indexed();
    idx++;
  }
  write_c0_entryhi(old_entryhi);
  tlbw_use_hazard();
  EXIT_CRITICAL(flags);
}


//---------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mips_vz]:

PUBLIC static inline NEEDS["cpu.h"]
void Mem_unit::change_vzguestid(unsigned long id)
{
  if (cpu_has_vzguestid)
    mips32_SetGuestID(id);
}

PUBLIC static inline NEEDS["cpu.h"]
void Mem_unit::change_vzguestrid(unsigned long rid)
{
  if (cpu_has_vzguestid)
    mips32_SetGuestRID(rid);
}

PUBLIC static inline NEEDS["cpu.h"]
void Mem_unit::clear_vzguestrid()
{
  if (cpu_has_vzguestid)
    mips32_ClearGuestRID();
}

/*
 * Helper to test for a guest entry or for a guestid match.
 *
 * @guestctl1: guestctl1.RID field is compared with guestid
 * @guestid: the id to match or Mem_unit::FLUSH_ALL_GUESTIDS to match any guest
 * entry
 *
 * @return: true if guestctl1.RID matches guestid, or if
 * Mem_unit::FLUSH_ALL_GUESTIDS then return true for any guest entry
 *
 */
PRIVATE static
bool
Mem_unit::match_guestid(unsigned int guestctl1, unsigned int guestid)
{
	if (guestid == Mem_unit::FLUSH_ALL_GUESTIDS)
		return (guestctl1 & GUESTCTL1_RID) != 0;
	else
		return ((guestctl1 & GUESTCTL1_RID) >> GUESTCTL1_RID_SHIFT) == guestid;
}

/*
 * Invalidate all guest entries matching guestid in root TLB.
 *
 * @guestid: 0 to invalidate all guest entries.
 *           n to invalidate all matching guest entryies.
 *           guestid is ignored if !cpu_has_vzguestid; all entries are
 *           invalidated.
 *
 */
PUBLIC static
void
Mem_unit::local_flush_roottlb_guestid(unsigned int guestid)
{
        Proc::Status flags;
	unsigned long old_entryhi;
	unsigned long old_pagemask;
	int entry;
	Signed32 tlbsize = Cpu::tlbsize();

	ENTER_CRITICAL(flags);

	/* If EHINV is supported tlbr may zero entryhi and pagemask */
	old_entryhi = read_c0_entryhi();
	old_pagemask = read_c0_pagemask();

	/* Invalidate guest entries in root TLB while leaving root entries
	 * intact when possible */
	for (entry = 0; entry < tlbsize; entry++) {

		write_c0_index(entry);
		mtc0_tlbw_hazard();

		if (cpu_has_vzguestid) {
			unsigned long guestctl1;

			/* Read out GuestRID field and clear back to normal
			 * zeroed state */
			tlb_read();
			tlbw_use_hazard();
			guestctl1 = read_c0_guestctl1();
			mips32_ClearGuestRID();

			/* Invalidate guest entry if GuestRID field matches or
                         * if Mem_unit::FLUSH_ALL_GUESTIDS is set then
                         * invalidate any guest entry in the root TLB */
			if (!Mem_unit::match_guestid(guestctl1, guestid))
				continue;
		}

		/* Make sure all entries differ. */
		write_c0_entryhi(UNIQUE_ENTRYHI(entry));
		write_c0_entrylo0(0);
		write_c0_entrylo1(0);
		mtc0_tlbw_hazard();
		tlb_write_indexed();
	}

	write_c0_entryhi(old_entryhi);
	write_c0_pagemask(old_pagemask);
	tlbw_use_hazard();

	EXIT_CRITICAL(flags);
}

/*
 * Invalidate all entries in root tlb irrespective of guestid.
 *
 * If !cpu_has_vzguestid then entire tlb is flushed.
 *
 */
PUBLIC static
void
Mem_unit::local_flush_roottlb_all_guests(void)
{
	Mem_unit::local_flush_roottlb_guestid(Mem_unit::FLUSH_ALL_GUESTIDS);
}

/*
 * Invalidate all guest entries matching guestid in guest TLB.
 *
 * @guestid: 0 to invalidate all guest entries.
 *           n to invalidate all matching guest entryies.
 *           guestid is ignored if !cpu_has_vzguestid; all entries are
 *           invalidated.
 *
 */
PUBLIC static
void
Mem_unit::local_flush_guesttlb_guestid(unsigned int guestid)
{
        Proc::Status flags;
	unsigned long old_entryhi;
	unsigned long old_pagemask;
	int entry;
        Signed32 vztlbsize = Cpu::vztlbsize();

	ENTER_CRITICAL(flags);

	/* If EHINV is supported tlbgr may zero entryhi and pagemask */
	old_entryhi = read_c0_guest_entryhi();
	old_pagemask = read_c0_guest_pagemask();

	/* Invalidate guest entries in guest TLB */
	for (entry = 0; entry < vztlbsize; entry++) {

		write_c0_guest_index(entry);
		mtc0_tlbw_hazard();

		if (cpu_has_vzguestid && (guestid != Mem_unit::FLUSH_ALL_GUESTIDS)) {
			unsigned long guestctl1;

			/* Read out GuestRID field and clear back to normal
			 * zeroed state */
			tlb_read_guest_indexed();
			tlbw_use_hazard();
			guestctl1 = read_c0_guestctl1();
			mips32_ClearGuestRID();

			/* Invalidate guest entry if GuestRID field matches */
			if (!Mem_unit::match_guestid(guestctl1, guestid))
				continue;
		}

		/* Make sure all entries differ. */
		write_c0_guest_entryhi(UNIQUE_ENTRYHI(entry));
		write_c0_guest_entrylo0(0);
		write_c0_guest_entrylo1(0);
		mtc0_tlbw_hazard();
		tlb_write_guest_indexed();
	}
	write_c0_guest_entryhi(old_entryhi);
	write_c0_guest_pagemask(old_pagemask);
	tlbw_use_hazard();

	EXIT_CRITICAL(flags);
}

/*
 * Invalidate all entries in guest tlb irrespective of guestid.
 *
 * If !cpu_has_vzguestid then entire tlb is flushed.
 *
 */
PUBLIC static
void
Mem_unit::local_flush_guesttlb_all(void)
{
	Mem_unit::local_flush_guesttlb_guestid(Mem_unit::FLUSH_ALL_GUESTIDS);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32 && !mips_vz]:

PUBLIC static inline
void Mem_unit::change_vzguestid(unsigned long)
{}

PUBLIC static inline
void Mem_unit::change_vzguestrid(unsigned long)
{}

PUBLIC static inline
void Mem_unit::clear_vzguestrid()
{}

PUBLIC static
void
Mem_unit::local_flush_guesttlb_all(void)
{}

PUBLIC static
void
Mem_unit::local_flush_roottlb_all_guests(void)
{}

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

PUBLIC static
void Mem_unit::tlb_write(const Tlb_entry &tlb_entry)
{
  Proc::Status flags;
  Mword old_entryhi;
  volatile Signed32 idx;

  ENTER_CRITICAL(flags);

  /* KYMA NOTE: In cases where tlb_write sets up the tlb for another context we
   * cannot copy the current vzguestid to the guestrid field. In Fiasco the
   * kernel sometimes manipulates tlb entries for the non-current VZ guest
   * (unlike Linux where KVM only manipulates the current VZ guest tlb entries).
   */
  /* Set GuestRID for root probe and write of guest TLB entry */
  change_vzguestrid((tlb_entry.guestctl1() & GUESTCTL1_RID) >> GUESTCTL1_RID_SHIFT);

  old_entryhi = read_c0_entryhi();
  write_c0_entryhi(tlb_entry.entryhi());
  mtc0_tlbw_hazard();

  tlb_probe();
  tlb_probe_hazard();
  idx = read_c0_index();

  if (idx < 0) {
    idx = read_c0_random();
    write_c0_index(idx);
    mtc0_tlbw_hazard();
  }
  write_c0_pagemask(tlb_entry.pagemask());
  write_c0_entrylo0(tlb_entry.entrylo0());
  write_c0_entrylo1(tlb_entry.entrylo1());
  mtc0_tlbw_hazard();

  tlb_write_indexed();
  tlbw_use_hazard();

  /* Restore old ASID */
  write_c0_entryhi(old_entryhi);
  mtc0_tlbw_hazard();

  clear_vzguestrid();

  tlbw_use_hazard();
  EXIT_CRITICAL(flags);
}

struct kvm_mips_tlb {
  long tlb_mask;
  long tlb_hi;
  long tlb_lo0;
  long tlb_lo1;
  long guestctl1;
};

#define MIPS3_PG_G  0x00000001  /* Global; ignore ASID if in lo0 & lo1 */
#define MIPS3_PG_V  0x00000002  /* Valid */
#define MIPS3_PG_NV 0x00000000
#define MIPS3_PG_D  0x00000004  /* Dirty */

#define PRIx64			"llx"

#define MIPS3_PG_SHIFT      Tlb_entry::PFNShift
#define MIPS3_PG_FRAME      Tlb_entry::PFNMask 

#define TLB_VPN2(x)         ((x).tlb_hi & Tlb_entry::VPN2Mask)
#define TLB_ASID(x)         ((x).tlb_hi & Tlb_entry::AsidMask)

#define mips3_paddr_to_tlbpfn(x) \
    (((unsigned long)(x) >> MIPS3_PG_SHIFT) & MIPS3_PG_FRAME)
#define mips3_tlbpfn_to_paddr(x) \
    ((unsigned long)((x) & MIPS3_PG_FRAME) << MIPS3_PG_SHIFT)


PUBLIC static
void Mem_unit::dump_tlbs(void)
{
	struct kvm_mips_tlb tlb;
	int i;
	Proc::Status flags;
	unsigned long old_entryhi;
	unsigned long old_pagemask;
	Signed32 tlbsize = Cpu::tlbsize();

	ENTER_CRITICAL(flags);

	old_entryhi = read_c0_entryhi();
	old_pagemask = read_c0_pagemask();

	printf("ROOT TLBs:\n");
	printf("ASID: %#lx\n", read_c0_entryhi() & Tlb_entry::AsidMask);

	for (i = 0; i < tlbsize; i++) {
		write_c0_index(i);
		mtc0_tlbw_hazard();

		tlb_read();
		tlbw_use_hazard();

		tlb.tlb_hi = read_c0_entryhi();
		tlb.tlb_lo0 = read_c0_entrylo0();
		tlb.tlb_lo1 = read_c0_entrylo1();
		tlb.tlb_mask = read_c0_pagemask();
		tlb.guestctl1 = 0;
#ifdef CONFIG_MIPS_VZ
		if (cpu_has_vzguestid) {
			tlb.guestctl1 = read_c0_guestctl1();
			/* clear GuestRID after tlb_read in case it was changed */
			mips32_ClearGuestRID();
		}
#endif

		printf("TLB%c%3d Hi 0x%08lx ",
		       (tlb.tlb_lo0 | tlb.tlb_lo1) & MIPS3_PG_V ? ' ' : '*',
		       i, tlb.tlb_hi);
		if (cpu_has_vzguestid) {
			printf("GuestCtl1 0x%08lx ", tlb.guestctl1);
		}
		printf("Lo0=0x%09" PRIx64 " %c%c%c attr %lx ",
		       (Unsigned64) mips3_tlbpfn_to_paddr(tlb.tlb_lo0),
		       (tlb.tlb_lo0 & MIPS3_PG_D) ? 'D' : ' ',
		       (tlb.tlb_lo0 & MIPS3_PG_G) ? 'G' : ' ',
		       (tlb.tlb_lo0 & MIPS3_PG_V) ? 'V' : ' ',
		       (tlb.tlb_lo0 >> 3) & 7);
		printf("Lo1=0x%09" PRIx64 " %c%c%c attr %lx sz=%lx\n",
		       (Unsigned64) mips3_tlbpfn_to_paddr(tlb.tlb_lo1),
		       (tlb.tlb_lo1 & MIPS3_PG_D) ? 'D' : ' ',
		       (tlb.tlb_lo1 & MIPS3_PG_G) ? 'G' : ' ',
		       (tlb.tlb_lo0 & MIPS3_PG_V) ? 'V' : ' ',
		       (tlb.tlb_lo1 >> 3) & 7, tlb.tlb_mask);
	}
	write_c0_entryhi(old_entryhi);
	write_c0_pagemask(old_pagemask);
	mtc0_tlbw_hazard();
	EXIT_CRITICAL(flags);
}

