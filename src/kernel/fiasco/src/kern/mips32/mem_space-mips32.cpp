/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "auto_quota.h"
#include "kmem.h"               // for "_unused_*" virtual memory regions
#include "member_offs.h"
#include "paging.h"
#include "types.h"
#include "ram_quota.h"

EXTENSION class Mem_space
{
  friend class Jdb;

public:
  typedef Pdir Dir_type;

  /** Return status of v_insert. */
  enum // Status
  {
    Insert_ok = 0,              ///< Mapping was added successfully.
    Insert_warn_exists,         ///< Mapping already existed
    Insert_warn_attrib_upgrade, ///< Mapping already existed, attribs upgrade
    Insert_err_nomem,           ///< Couldn't alloc new page table
    Insert_err_exists           ///< A mapping already exists at the target addr
  };

  // Mapping utilities

  enum                          // Definitions for map_util
  {
    Need_insert_tlb_flush = 1,
    Map_page_size = Config::PAGE_SIZE,
    Page_shift = Config::PAGE_SHIFT,
    Whole_space = MWORD_BITS,
    Identity_map = 0,
  };


  static void kernel_space(Mem_space *);

private:
  // DATA
  Dir_type *_dir;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include <cassert>
#include <cstring>
#include <new>

#include "atomic.h"
#include "config.h"
#include "globals.h"
#include "kdb_ke.h"
#include "l4_types.h"
#include "panic.h"
#include "paging.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "mem_unit.h"
#include "tlb_entry.h"
#include "cpu.h"


/** Constructor.  Creates a new address space and registers it with
  * Space_index.
  *
  * Registration may fail (if a task with the given number already
  * exists, or if another thread creates an address space for the same
  * task number concurrently).  In this case, the newly-created
  * address space should be deleted again.
  */
PUBLIC inline NEEDS[Mem_space::vzguestid]
Mem_space::Mem_space(Ram_quota *q)
: _quota(q), _dir(0), _is_vmspace(false), _use_vzguestid(false)
{
  asid(Mem_unit::Asid_invalid);
  vzguestid(Mem_unit::Vz_root_guestid);
}

PROTECTED inline NEEDS[<new>, "kmem_alloc.h", Mem_space::asid]
bool
Mem_space::initialize()
{
  Auto_quota<Ram_quota> q(ram_quota(), sizeof(Dir_type));
  if (EXPECT_FALSE(!q))
    return false;

  _dir = (Dir_type*)Kmem_alloc::allocator()->unaligned_alloc(sizeof(Dir_type));
  if (!_dir)
    return false;

  _dir->clear(Pte_ptr::need_cache_write_back(false));

  q.release();
  return true;
}

PUBLIC
Mem_space::Mem_space(Ram_quota *q, Dir_type* pdir)
  : _quota(q), _dir(pdir), _is_vmspace(false), _use_vzguestid(false)
{
  asid(Mem_unit::Asid_invalid);
  vzguestid(Mem_unit::Vz_root_guestid);
  _current.cpu(Cpu_number::boot_cpu()) = this;
}

PUBLIC static inline
bool
Mem_space::is_full_flush(L4_fpage::Rights rights)
{
  return rights & L4_fpage::Rights::R();
}

// Mapping utilities

PUBLIC inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush(bool force = false)
{
  if (!Have_asids)
    Mem_unit::tlb_flush();
  else if (force && c_asid() != Mem_unit::Asid_invalid)
    Mem_unit::tlb_flush(c_asid());

  // else do nothing, we manage ASID local flushes in v_* already
  // Mem_unit::tlb_flush();
}

PUBLIC static inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush_spaces(bool all, Mem_space *s1, Mem_space *s2)
{
  if (all || !Have_asids)
    Mem_unit::tlb_flush();
  else
    {
      if (s1)
        s1->tlb_flush(true);
      if (s2)
        s2->tlb_flush(true);
    }
}


IMPLEMENT inline
Mem_space *
Mem_space::current_mem_space(Cpu_number cpu)
{
  return _current.cpu(cpu);
}

IMPLEMENT inline NEEDS ["kmem.h", Mem_space::c_asid]
void Mem_space::switchin_context(Mem_space *from)
{
#if 0
  // never switch to kernel space (context of the idle thread)
  if (this == kernel_space())
    return;
#endif

  if (from != this)
    make_current();
  else
    tlb_flush(true);
}


IMPLEMENT inline
void Mem_space::kernel_space(Mem_space *_k_space)
{
  _kernel_space = _k_space;
}

IMPLEMENT
Mem_space::Status
Mem_space::v_insert(Phys_addr phys, Vaddr virt, Page_order size,
                    Attr page_attribs)
{
  bool const flush = _current.current() == this;
  Mem_space::Status ret;

  assert (cxx::get_lsb(Phys_addr(phys), size) == 0);
  assert (cxx::get_lsb(Virt_addr(virt), size) == 0);

  int level;
  for (level = 0; level <= Pdir::Depth; ++level)
    if (Page_order(Pdir::page_order_for_level(level)) <= size)
      break;

  auto i = _dir->walk(virt, level, Pte_ptr::need_cache_write_back(flush),
                      Kmem_alloc::q_allocator(_quota));

  if (EXPECT_FALSE(!i.is_valid() && i.level != level))
    return Insert_err_nomem;

  if (EXPECT_FALSE(i.is_valid()
                   && (i.level != level || Phys_addr(i.page_addr()) != phys)))
  {
    ret = Insert_err_exists;
  }
  else if (i.is_valid())
  {
    if (EXPECT_FALSE(!i.add_attribs(page_attribs)))
    {
      ret = Insert_warn_exists;
    }
    else
    {
      i.write_back_if(flush, c_asid());
      ret = Insert_warn_attrib_upgrade;
    }
  }
  else
  {
    i.create_page(phys, page_attribs);
    i.write_back_if(flush, Mem_unit::Asid_invalid);
    ret = Insert_ok;
  }

#if 0
  static int debug = 0;
  if (is_vmspace() || debug)
    printf("v_insert vmspace cur() %08lx, this %08lx, flush %d, c_asid %02lx, c_vzguestid %02lx\n",
         (Mword)_current.current(), (Mword)this, flush, c_asid(), c_vzguestid());
#endif

#if 1 // Kyma: use eager mapping
  i.update_tlb(Virt_addr::val(virt), c_asid(), c_vzguestid());
#else // Kyma: use lazy mapping
  if (ret != Insert_ok) {
    i.update_tlb(Virt_addr::val(virt), c_asid(), c_vzguestid());
  }
#endif
  return ret;
}

IMPLEMENT
void
Mem_space::v_set_access_flags(Vaddr virt, L4_fpage::Rights access_flags)
{
  auto i = _dir->walk(virt);

  if (EXPECT_FALSE(!i.is_valid()))
    return;

  unsigned page_attribs = 0;

  if (access_flags & L4_fpage::Rights::R())
    page_attribs |= Pt_entry::Referenced;
  if (access_flags & L4_fpage::Rights::W())
    page_attribs |= Pt_entry::Dirty;

  i.add_attribs(page_attribs);
}

/**
 * Simple page-table lookup.
 *
 * @param virt Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline NEEDS ["paging.h"]
Address
Mem_space::virt_to_phys(Address virt) const
{
  return dir()->virt_to_phys(virt);
}

/**
 * Simple page-table lookup.
 *
 * @param virt Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline NEEDS ["mem_layout.h"]
Address
Mem_space::pmem_to_phys(Address virt) const
{
  return Mem_layout::pmem_to_phys(virt);
}

/**
 * Simple page-table lookup.
 *
 * This method is similar to Mem_space's lookup().
 * The difference is that this version handles Sigma0's
 * address space with a special case:  For Sigma0, we do not
 * actually consult the page table -- it is meaningless because we
 * create new mappings for Sigma0 transparently; instead, we return the
 * logically-correct result of physical address == virtual address.
 *
 * @param a Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline
virtual Address
Mem_space::virt_to_phys_s0(void *a) const
{
  return dir()->virt_to_phys((Address)a);
}

IMPLEMENT
bool
Mem_space::v_lookup(Vaddr virt, Phys_addr *phys,
                    Page_order *order, Attr *page_attribs)
{
  auto i = _dir->walk(virt);
  if (order) *order = Page_order(i.page_order());

  if (!i.is_valid())
    return false;

  if (phys) *phys = Phys_addr(i.page_addr());
  if (page_attribs) *page_attribs = i.attribs();

  return true;
}

IMPLEMENT
L4_fpage::Rights
Mem_space::v_delete(Vaddr virt, Page_order size,
                    L4_fpage::Rights page_attribs)
{
  (void)size;
  assert (cxx::get_lsb(Virt_addr(virt), size) == 0);
  auto i = _dir->walk(virt);

  if (EXPECT_FALSE (! i.is_valid()))
    return L4_fpage::Rights(0);

  assert (! (*i.pte & Pt_entry::Global)); // Cannot unmap shared pages

  L4_fpage::Rights ret = i.access_flags();

  if (! (page_attribs & L4_fpage::Rights::R()))
    i.del_rights(page_attribs);
  else
    i.clear();

  i.write_back_if(_current.current() == this, c_asid());
  i.update_tlb(Virt_addr::val(virt), c_asid(), c_vzguestid());

  return ret;
}

PUBLIC
bool
Mem_space::v_pagein_mem(Vaddr virt)
{
  auto i = _dir->walk(virt);
  if (i.is_valid()) {
#if 0
    static int debug = 0;
    if (is_vmspace() || debug)
      printf("v_pagein_mem vmspace cur() %08lx, this %08lx, c_asid %02lx, c_vzguestid %02lx\n",
           (Mword)_current.current(), (Mword)this, c_asid(), c_vzguestid());
#endif
    i.update_tlb(Virt_addr::val(virt), c_asid(), c_vzguestid());
    return true;
  }
  return false;
}

PUBLIC static
bool
Mem_space::v_pagein_kmem(Vaddr virt)
{
  auto i = Kmem::kdir()->walk(virt);
  if (i.is_valid()) {
    i.update_tlb(Virt_addr::val(virt), 0, Mem_unit::Vz_root_guestid);
    return true;
  }
  return false;
}

/**
 * \brief Free all memory allocated for this Mem_space.
 * \pre Runs after the destructor!
 */
PUBLIC
Mem_space::~Mem_space()
{
  reset_asid();
  if (_dir)
    {

      // free all page tables we have allocated for this address space
      // except the ones in kernel space which are always shared
      _dir->destroy(Virt_addr(0UL),
                    Virt_addr(Mem_layout::User_max), 0, Pdir::Depth,
                    Kmem_alloc::q_allocator(_quota));
      // free all unshared page table levels for the kernel space
      if (Virt_addr(Mem_layout::User_max) < Virt_addr(~0UL))
        _dir->destroy(Virt_addr(Mem_layout::User_max + 1),
                      Virt_addr(~0UL), 0, Pdir::Super_level,
                      Kmem_alloc::q_allocator(_quota));
      Kmem_alloc::allocator()->q_unaligned_free(ram_quota(), sizeof(Dir_type), _dir);
    }
}

PROTECTED inline
int
Mem_space::sync_kernel()
{
  return _dir->sync(Virt_addr(Mem_layout::User_max + 1), kernel_space()->_dir,
                    Virt_addr(Mem_layout::User_max + 1),
                    Virt_size(-(Mem_layout::User_max + 1)), Pdir::Super_level,
                    Pte_ptr::need_cache_write_back(this == _current.current()),
                    Kmem_alloc::q_allocator(_quota));
}

PUBLIC static inline
Page_number
Mem_space::canonize(Page_number v)
{ return v; }

PUBLIC static
void
Mem_space::init_page_sizes()
{
  add_page_size(Page_order(Config::PAGE_SHIFT));

  if (Config::have_superpages)
    add_page_size(Page_order(Config::SUPERPAGE_SHIFT));
}

PUBLIC static
void
Mem_space::init()
{
  Pt_entry::have_superpages(Config::have_superpages);
  init_page_sizes();
  init_vzguestid();
}

EXTENSION class Mem_space
{
public:
  enum { Have_asids = 1 };
private:
  typedef Per_cpu_array<unsigned long> Asid_array;
  Asid_array _asid;

  static Per_cpu<unsigned char> _next_free_asid;
  static Per_cpu<Mem_space *[256]>   _active_asids;
};

PRIVATE inline static
unsigned long
Mem_space::next_asid(Cpu_number cpu)
{
  if (_next_free_asid.cpu(cpu) == 0)
    ++_next_free_asid.cpu(cpu);
  return _next_free_asid.cpu(cpu)++;
}

DEFINE_PER_CPU Per_cpu<unsigned char>    Mem_space::_next_free_asid;
DEFINE_PER_CPU Per_cpu<Mem_space *[256]> Mem_space::_active_asids;

PRIVATE inline
void
Mem_space::asid(unsigned long a)
{
  for (Asid_array::iterator i = _asid.begin(); i != _asid.end(); ++i)
    *i = a;
}

PUBLIC inline
unsigned long
Mem_space::c_asid() const
{ return _asid[current_cpu()]; }

PRIVATE inline NEEDS[Mem_space::next_asid, "types.h"]
unsigned long
Mem_space::asid()
{
  Cpu_number cpu = current_cpu();
  if (EXPECT_FALSE(_asid[cpu] == Mem_unit::Asid_invalid))
    {
      // FIFO ASID replacement strategy
      unsigned char new_asid = next_asid(cpu);
      Mem_space **bad_guy = &_active_asids.cpu(cpu)[new_asid];
      while (Mem_space *victim = access_once(bad_guy))
        {
          // need ASID replacement
          if (victim == current_mem_space(cpu))
            {
              // do not replace the ASID of the current space
              new_asid = next_asid(cpu);
              bad_guy = &_active_asids.cpu(cpu)[new_asid];
              continue;
            }

          //LOG_MSG_3VAL(current(), "ASIDr", new_asid, (Mword)*bad_guy, (Mword)this);

          // If the victim is valid and we get a 1 written to the ASID array
          // then we have to reset the ASID of our victim, else the
          // reset_asid function is currently resetting the ASIDs of the
          // victim on a different CPU.
          if (victim != reinterpret_cast<Mem_space*>(~0UL) &&
              mp_cas(bad_guy, victim, reinterpret_cast<Mem_space*>(1)))
            write_now(&victim->_asid[cpu], (Mword)Mem_unit::Asid_invalid);
          break;
        }

      _asid[cpu] = new_asid;
      Mem_unit::tlb_flush(new_asid);
      write_now(bad_guy, this);
    }

  //LOG_MSG_3VAL(current(), "ASID", (Mword)this, _asid[cpu], (Mword)__builtin_return_address(0));
  return _asid[cpu];
};

PRIVATE inline
void
Mem_space::reset_asid()
{
  for (Cpu_number i = Cpu_number::first(); i < Config::max_num_cpus(); ++i)
    {
      unsigned long asid = access_once(&_asid[i]);
      if (asid == Mem_unit::Asid_invalid)
        continue;

      Mem_space **a = &_active_asids.cpu(i)[asid];
      if (!mp_cas(a, this, reinterpret_cast<Mem_space*>(~0UL)))
        // It could be our ASID is in the process of being preempted,
        // so wait until this is done.
        while (access_once(a) == reinterpret_cast<Mem_space*>(1))
          ;
    }
}

/**
 * Make the Mem_space (or VM Mem_space) the current context.
 *
 * When VZ GuestID is supported, the VM's GuestID remains set in GuestCtl1.ID
 * until another VM is made current.  As a rule GuestRID remains zeroed when in
 * root context unless the kernel is actively manipulating guest tlb entries.
 *
 * NOTE: We still set the asid for VM guests which support vzguestid because
 * leaving a value of Asid_invalid confuses Fiasco.
 *
 */
IMPLEMENT inline
void
Mem_space::make_current()
{
  Mem_unit::change_asid(asid());
  if (EXPECT_FALSE(_is_vmspace)) {
    if (_use_vzguestid) {
      Mem_unit::change_vzguestid(vzguestid());
    } else {
      // TODO guest tlb flush not required if same vm is still current
      Mem_unit::local_flush_guesttlb_all();
    }
  }

  _current.current() = this;
}

EXTENSION class Mem_space
{
private:
  typedef Per_cpu_array<unsigned long> Vzguestid_array;
  Vzguestid_array _vzguestid;
  bool _is_vmspace;
  bool _use_vzguestid;

  static Vzguestid_array _vzguestid_cache;
  static const unsigned long VZGUESTID_INC = 1;
  static unsigned long _vzguestid_mask;
  static unsigned long _vzguestid_first_version;
  static unsigned long _vzguestid_version_mask;

public:
  bool is_vmspace(void) { return _is_vmspace; };
  bool use_vzguestid(void) { return _use_vzguestid; };
};

DEFINE_PER_CPU Mem_space::Vzguestid_array Mem_space::_vzguestid_cache;
unsigned long Mem_space::_vzguestid_mask;
unsigned long Mem_space::_vzguestid_first_version;
unsigned long Mem_space::_vzguestid_version_mask;

PRIVATE static
void Mem_space::init_vzguestid(void)
{
  if (!cpu_has_vzguestid)
    return;

  _vzguestid_mask = Cpu::vzguestid_mask();
  _vzguestid_first_version = _vzguestid_mask + 1;
  _vzguestid_version_mask = 0xffff & ~_vzguestid_mask;

  Vzguestid_array::iterator i;
  for (i = _vzguestid_cache.begin(); i != _vzguestid_cache.end(); ++i)
    *i = _vzguestid_first_version;

}

PUBLIC inline NEEDS[Mem_space::use_vzguestid]
void Mem_space::is_vmspace(bool enable)
{
  _is_vmspace = enable;
  use_vzguestid(enable);
}

PUBLIC inline NEEDS["cpu.h"]
void Mem_space::use_vzguestid(bool enable)
{
  if (enable && _is_vmspace && cpu_has_vzguestid)
    _use_vzguestid = true;
  else
    _use_vzguestid = false;
}

PRIVATE inline
void
Mem_space::vzguestid(unsigned long id)
{
  for (Vzguestid_array::iterator i = _vzguestid.begin(); i != _vzguestid.end(); ++i)
    *i = id;
}

PUBLIC inline
unsigned long
Mem_space::c_vzguestid() const
{ return _vzguestid[current_cpu()] & _vzguestid_mask; }

PRIVATE static
void
Mem_space::next_vzguestid(Cpu_number cpu)
{
  unsigned long vzguestid = _vzguestid_cache[cpu];

  if (!((vzguestid += VZGUESTID_INC) & _vzguestid_mask))
  {
    /* fix version if needed */
    if (!vzguestid)
      vzguestid = _vzguestid_first_version;

    /* vzguestid 0 reserved for root */
    vzguestid += VZGUESTID_INC;

    /* start new vzguestid cycle */
    Mem_unit::local_flush_roottlb_all_guests();
    Mem_unit::local_flush_guesttlb_all();

  }

  _vzguestid_cache[cpu] = vzguestid;
}

PRIVATE inline
bool
Mem_space::vzguestid_valid(Cpu_number cpu)
{
  /* vzguestid is invalid if cache version has advanced to a newer generation */
  return !(((_vzguestid[cpu] ^ _vzguestid_cache[cpu])) & _vzguestid_version_mask);
}

PRIVATE
unsigned long
Mem_space::vzguestid()
{
  Cpu_number cpu = current_cpu();
  if (!vzguestid_valid(cpu)) {
    next_vzguestid(cpu);
    _vzguestid[cpu] = _vzguestid_cache[cpu];
    assert(_vzguestid[cpu] != Mem_unit::Vz_root_guestid);
  }
  return _vzguestid[cpu] & _vzguestid_mask;
}
