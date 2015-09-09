/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "types.h"
#include "mem_unit.h"
#include "ptab_base.h"
#include "tlb_entry.h"


class PF {
public:
  enum Page_fault_code
  {
    PF_ERR_EXCCODE    = 0x000000ff, ///< PF: Exception Code Mask
    PF_ERR_NOTPRESENT = 0x00000100, ///< PF: Page Is NOT Present In PTE
    PF_ERR_WRITEPROT  = 0x00000200, ///< PF: Page Is Write Protected
    PF_ERR_USRMODE    = 0x00000400, ///< PF: Caused By User Mode Code
    PF_ERR_VZGUEST    = 0x00000800, ///< PF: Caused By VZ Guest
    PF_ERR_MASK       = 0x0000ff00, ///< PF: Pagefault Error Mask
  };
};

class Page
{
public:
  enum Attribs_enum
  {
    Cache_mask    = 0x00000038, ///< Cache attrbute mask
    NONCACHEABLE  = 0x00000010,
    CACHEABLE     = 0x00000018,
  };
};

class Pt_entry
{
public:
  enum
  {
    Super_level      = 0,
    Global           = Tlb_entry::Global,    ///< Global, pinned in the TLB
    Valid            = Tlb_entry::Valid,     ///< Valid
    Dirty            = Tlb_entry::Dirty,     ///< Page was modified
    Noncacheable     = Tlb_entry::Uncached,  ///< Caching is off
    Cacheable        = Tlb_entry::Cacheable, ///< Cache is enabled
    Cache_mask       = Tlb_entry::Cca,       ///< Cache attrbute mask
    Writable         = 0x00000040,           ///< Writable
    User             = 0x00000080,           ///< User accessible
    Referenced       = 0x00000100,           ///< Page was referenced
    Pse_bit          = 0x00000200,           ///< Indicates a super page
    XD               = 0,
    PTE_ATTRIBS_MASK = Writable | User,
    TLB_ATTRIBS_MASK = Global | Valid | Dirty | Cache_mask,
  };

private:
  static unsigned _super_level;
  static bool _have_superpages;
};

class Pte_ptr : private Pt_entry
{
public:
  Pte_ptr(void *pte, unsigned char level) : pte((Mword*)pte), level(level) {}
  Pte_ptr() = default;

  using Pt_entry::Super_level;

  typedef struct {
    Mword pte_even;
    Mword pte_odd;
  } Pte_pair_raw;

  Mword *pte;
  unsigned char level;
};


INTERFACE [mips32 && pagesize_4k]:

typedef Ptab::List< Ptab::Traits<Unsigned32, 22, 10, false>,
                    Ptab::Traits<Unsigned32, 12, 10, true> > Ptab_traits;

INTERFACE [mips32 && pagesize_16k]:

// next_level must also be changed to 256Byte second level table i.e. 8 bit
typedef Ptab::List< Ptab::Traits<Unsigned32, 24, 8, false>,
                    Ptab::Traits<Unsigned32, 14, 10, true> > Ptab_traits;

// Kyma: If Config::have_superpages then set leaf to true.
//typedef Ptab::List< Ptab::Traits<Unsigned32, 22, 10, true>,
//                    Ptab::Traits<Unsigned32, 12, 10, true> > Ptab_traits;

INTERFACE [mips32]:
typedef Ptab::Shift<Ptab_traits, Virt_addr::Shift>::List Ptab_traits_vpn;
typedef Ptab::Page_addr_wrap<Page_number, Virt_addr::Shift> Ptab_va_vpn;


//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "atomic.h"

bool Pt_entry::_have_superpages;
unsigned  Pt_entry::_super_level;

PUBLIC static inline
void
Pt_entry::have_superpages(bool yes)
{
  _have_superpages = yes;
  _super_level = yes ? Super_level : (Super_level + 1);
}

PUBLIC static inline
unsigned
Pt_entry::super_level()
{ return _super_level; }

/**
 * Global entries are entries that are not automatically flushed when the
 * page-table base register is reloaded. They are intended for kernel data
 * that is shared between all tasks.
 * @return global page-table--entry flags
 */
PUBLIC inline
Mword
Pte_ptr::is_global() const
{ return *pte & Global; }

PUBLIC inline
bool
Pte_ptr::is_valid() const
{ return *pte & Valid; }

PUBLIC inline
bool
Pte_ptr::is_leaf() const
{ return level == Pdir::Depth || (*pte & Pse_bit); }


//---------------------------------------------------------------------------
IMPLEMENTATION [mips32 && pagesize_4k]:

/**
 * \pre is_leaf() == false
 */
PUBLIC inline
Mword
Pte_ptr::next_level() const
{ return cxx::mask_lsb(*pte, (unsigned)Config::PAGE_SHIFT); }


//---------------------------------------------------------------------------
IMPLEMENTATION [mips32 && pagesize_16k]:

/**
 * \pre is_leaf() == false
 */
PUBLIC inline
Mword
Pte_ptr::next_level() const
{ return cxx::mask_lsb(*pte, 8); }


//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

/**
 * \pre cxx::get_lsb(phys_addr, Config::PAGE_SHIFT) == 0
 */
PUBLIC inline
void
Pte_ptr::set_next_level(Mword phys_addr)
{ *pte = phys_addr | Valid | User | Writable; }

PUBLIC inline
void
Pte_ptr::set_page(Mword phys, Mword attr)
{
  Mword v = phys | Valid | attr;
  if (level < Pdir::Depth)
    v |= Pse_bit;
  *pte = v;
}

PUBLIC inline
Mword
Pte_ptr::page_addr() const
{ return cxx::mask_lsb(*pte, Pdir::page_order_for_level(level)) & ~Mword(XD); }


PUBLIC inline
void
Pte_ptr::set_attribs(Page::Attr attr)
{
  typedef L4_fpage::Rights R;
  typedef Page::Type T;
  typedef Page::Kern K;
  Mword r = 0;
  if (attr.rights & R::W()) r |= Writable | Dirty;
  if (attr.rights & R::U()) r |= User;
  if (!(attr.rights & R::X())) r |= XD;
  if (attr.type == T::Normal()) r |= Page::CACHEABLE;
  if (attr.type == T::Uncached()) r |= Page::NONCACHEABLE;
  if (attr.kern & K::Global()) r |= Global;
  *pte = (*pte & ~(PTE_ATTRIBS_MASK | TLB_ATTRIBS_MASK)) | r;
}

PUBLIC inline
void
Pte_ptr::create_page(Phys_mem_addr addr, Page::Attr attr)
{
  Mword r = (level < Pdir::Depth) ? (Mword)Pse_bit : 0;
  typedef L4_fpage::Rights R;
  typedef Page::Type T;
  typedef Page::Kern K;
  if (attr.rights & R::W()) r |= Writable | Dirty;
  if (attr.rights & R::U()) r |= User;
  if (!(attr.rights & R::X())) r |= XD;
  if (attr.type == T::Normal()) r |= Page::CACHEABLE;
  if (attr.type == T::Uncached()) r |= Page::NONCACHEABLE;
  if (attr.kern & K::Global()) r |= Global;
  *pte = cxx::int_value<Phys_mem_addr>(addr) | r | Valid;
}

PUBLIC inline
Page::Attr
Pte_ptr::attribs() const
{
  typedef L4_fpage::Rights R;
  typedef Page::Type T;

  Mword _raw = *pte;
  R r = R::R();
  if (_raw & Writable) r |= R::W();
  if (_raw & User) r |= R::U();
  if (!(_raw & XD)) r |= R::X();

  T t;
  switch (_raw & Page::Cache_mask)
    {
    default:
    case Page::CACHEABLE:    t = T::Normal(); break;
    case Page::NONCACHEABLE: t = T::Uncached(); break;
    }
  // do not report kernel special flags such as K::Global, as this is used for user
  // level mappings. Doing so violates assertion e.g. in Mem_space::v_delete.
  return Page::Attr(r, t);
}

PUBLIC inline
bool
Pte_ptr::add_attribs(Page::Attr attr)
{
  typedef L4_fpage::Rights R;
  unsigned long a = 0;

  if (attr.rights & R::W())
    a = Writable | Dirty;

  if (attr.rights & R::X())
    a |= XD;

  if (!a)
    return false;

  auto p = access_once(pte);
  auto o = p;
  p ^= XD;
  p |= a;
  p ^= XD;

  if (o != p)
    {
      write_now(pte, p);
      return true;
    }
  return false;
}

PUBLIC inline
void
Pte_ptr::add_attribs(Mword attr)
{ *pte |= attr; }

PUBLIC inline
unsigned char
Pte_ptr::page_order() const
{ return Pdir::page_order_for_level(level); }

PUBLIC inline
unsigned int
Pte_ptr::page_mask() const
{ return (1 << (Pdir::page_order_for_level(level) + 1)) - 1; }

PUBLIC inline
Mword
Pte_ptr::page_tlb_attribs() const
{ return (*pte & TLB_ATTRIBS_MASK); }

PUBLIC inline
Pte_ptr::Pte_pair_raw*
Pte_ptr::pte_pair_boundary()
{
  // get raw odd/even pair boundary in page table
  return reinterpret_cast<Pte_pair_raw*>(reinterpret_cast<Mword>(pte) & ~(2 * sizeof(pte) - 1));
}

PUBLIC inline NEEDS["atomic.h"]
L4_fpage::Rights
Pte_ptr::access_flags() const
{

  if (!is_valid())
    return L4_fpage::Rights(0);

  L4_fpage::Rights r;
  for (;;)
    {
      auto raw = *pte;

      if (raw & Dirty)
        r = L4_fpage::Rights::RW();
      else if (raw & Referenced)
        r = L4_fpage::Rights::R();
      else
        return L4_fpage::Rights(0);

      if (mp_cas(pte, raw, raw & ~(Dirty | Referenced)))
        return r;
    }
}

PUBLIC inline
void
Pte_ptr::clear()
{ *pte = 0; }

PUBLIC inline
void
Pte_ptr::del_attribs(Mword attr)
{ *pte &= ~attr; }

PUBLIC inline
void
Pte_ptr::del_rights(L4_fpage::Rights r)
{
  if (r & L4_fpage::Rights::W())
    *pte &= ~(Writable | Dirty);

  if (r & L4_fpage::Rights::X())
    *pte |= XD;
}


//--------------------------------------------------------------------------

PUBLIC static inline
bool
Pte_ptr::need_cache_write_back(bool)
{ return false; }

PUBLIC inline
void
Pte_ptr::write_back_if(bool, Mword asid = Mem_unit::Asid_invalid)
{
#if 1
  (void)asid;
#else // KYMAXXX TODO consider whether we really need tlb flush here
  if (asid != Mem_unit::Asid_invalid)
    Mem_unit::tlb_flush(asid);
#endif
}

PUBLIC static inline
void
Pte_ptr::write_back(void * /*start*/, void * /*end*/)
{}

PUBLIC
void
Pte_ptr::update_tlb(Address virt, Mword asid, Mword vzguestid)
{
  if (asid == Mem_unit::Asid_invalid)
    return;

  Tlb_entry e;
  e.entryhi(virt, asid);
  e.pagemask(page_mask());

  Pte_pair_raw* pte_pair = pte_pair_boundary();
  e.entrylo0(pte_pair->pte_even, pte_pair->pte_even | (is_global() ? Global : 0));
  e.entrylo1(pte_pair->pte_odd, pte_pair->pte_odd | (is_global() ? Global : 0));
  e.guestctl1(vzguestid, 0);

  //printf("update_tlb: virt %lx, pte_even %lx, pte_odd %lx (this->pte %p)\n",
      //virt, pte_pair->pte_even, pte_pair->pte_odd, this->pte);
  Mem_unit::tlb_write(e);
}

//--------------------------------------------------------------------------

IMPLEMENT inline
Mword PF::is_translation_error(Mword error)
{
  return (error & PF_ERR_NOTPRESENT);
}

IMPLEMENT inline
Mword PF::is_usermode_error(Mword error)
{
  return (error & PF_ERR_USRMODE);
}

IMPLEMENT inline
Mword PF::is_read_error(Mword error)
{
  return !(error & PF_ERR_WRITEPROT);
}

IMPLEMENT inline
Mword PF::addr_to_msgword0(Address pfa, Mword error)
{
  Mword a = pfa & ~3;
  if(is_translation_error(error))
    a |= 1;
  if(!is_read_error(error))
    a |= 2;
  return a;
}
