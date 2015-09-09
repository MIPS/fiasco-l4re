/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "config.h"


class Tlb_entry
{
public:
  enum
  {
    Global    = 0x00000001,
    Valid     = 0x00000002,
    Dirty     = 0x00000004,
    Uncached  = 0x00000010,
    Cacheable = 0x00000018,
    Cca       = 0x00000038,
    AttrMask  = 0x0000003f,
    PageShift = Config::PAGE_SHIFT,
    VPN2Mask  = ~((1 << 13) - 1),
    AsidMask  = 0x000000ff,
    PFNShift  = 6,
    PFNMask   = 0x3fffffc0,
    PgMaskReg = 0x1ffff800,
  };

  Mword pagemask() const { return _pagemask; };
  Mword entryhi() const  { return _entryhi; };
  Mword entrylo0() const { return _entrylo0; };
  Mword entrylo1() const { return _entrylo1; };
  Mword guestctl1() const { return _guestctl1; };

  void pagemask(Mword pagemask) { _pagemask = pagemask & PgMaskReg; };
  void entryhi(Mword entryhi)   { _entryhi = entryhi; };
  void entrylo0(Mword entrylo0) { _entrylo0 = entrylo0; };
  void entrylo1(Mword entrylo1) { _entrylo1 = entrylo1; };
  void guestctl1(Mword guestctl1) { _guestctl1 = guestctl1; };

  Tlb_entry() : _pagemask{0}, _entryhi{0}, _entrylo0{0}, _entrylo1{0}, _guestctl1{0} {};

private:
  Mword _pagemask;
  Mword _entryhi;
  Mword _entrylo0;
  Mword _entrylo1;
  Mword _guestctl1;
};


//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "mipsvzregs.h"

PUBLIC inline
void
Tlb_entry::entryhi(Mword vaddr, Mword asid)
{
  _entryhi = (vaddr & Tlb_entry::VPN2Mask) | (asid & Tlb_entry::AsidMask);
}

PUBLIC inline
void
Tlb_entry::entrylo0(Mword paddr, Mword tlb_attr)
{
  _entrylo0 = (((paddr >> Tlb_entry::PFNShift) & Tlb_entry::PFNMask) | 
      (tlb_attr & Tlb_entry::AttrMask));
}

PUBLIC inline
void
Tlb_entry::entrylo1(Mword paddr, Mword tlb_attr)
{
  _entrylo1 = (((paddr >> Tlb_entry::PFNShift) & Tlb_entry::PFNMask) | 
      (tlb_attr & Tlb_entry::AttrMask));
}

PUBLIC inline NEEDS["mipsvzregs.h"]
void
Tlb_entry::guestctl1(Mword rid, Mword id)
{
  _guestctl1 = ((rid << GUESTCTL1_RID_SHIFT) & GUESTCTL1_RID) |
               ((id << GUESTCTL1_ID_SHIFT) & GUESTCTL1_ID);
}
