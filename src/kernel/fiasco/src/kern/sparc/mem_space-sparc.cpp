INTERFACE [sparc]:

#include "entry_frame.h"

extern "C"
Mword
pagefault_entry(Address, Mword, Mword, Return_frame *);

EXTENSION class Mem_space
{
public:

  typedef Pdir Dir_type;

  /** Return status of v_insert. */
  enum //Status 
  {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_warn_exists,		///< Mapping already existed
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,		///< Couldn't alloc new page table
    Insert_err_exists		///< A mapping already exists at the target addr
  };

  /** Attribute masks for page mappings. */
  enum Page_attrib
    {
      Page_no_attribs = 0,
      /// Page is writable.
      Page_writable = 0, // XXX
      Page_cacheable = 0,
      /// Page is noncacheable.
      Page_noncacheable = 0, // XXX
      /// it's a user page.
      Page_user_accessible = 0, // XXX
    };

  // Mapping utilities
  enum				// Definitions for map_util
  {
    Need_insert_tlb_flush = 0,
    Map_page_size = Config::PAGE_SIZE,
    Page_shift = Config::PAGE_SHIFT,
    Whole_space = MWORD_BITS,
    Identity_map = 0,
  };

protected:
  // DATA
  Dir_type *_dir;
};


//----------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

#include <cstring>
#include <cstdio>
#include "cpu.h"
#include "kdb_ke.h"
#include "l4_types.h"
#include "mem_layout.h"
#include "paging.h"
#include "std_macros.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "logdefs.h"
#include "panic.h"
#include "lock_guard.h"
#include "cpu_lock.h"
#include "warn.h"




PUBLIC explicit inline
Mem_space::Mem_space(Ram_quota *q) : _quota(q), _dir(0) {}

PROTECTED inline NEEDS["kmem_alloc.h"]
bool
Mem_space::initialize()
{
  void *b;
  if (EXPECT_FALSE(!(b = Kmem_alloc::allocator()
	  ->q_alloc(_quota, Config::PAGE_SHIFT))))
    return false;

  _dir = static_cast<Dir_type*>(b);
  _dir->clear(true);	// initialize to zero
  return true; // success
}

PUBLIC
Mem_space::Mem_space(Ram_quota *q, Dir_type* pdir)
  : _quota(q), _dir(pdir)
{
  _kernel_space = this;
  _current.cpu(Cpu_number::boot_cpu()) = this;
}

IMPLEMENT inline
void
Mem_space::make_current()
{
  printf("%s FIXME\n", __func__);
}


PROTECTED inline
int
Mem_space::sync_kernel()
{
  printf("%s FIXME\n", __func__);
  return 0;
}


IMPLEMENT inline NEEDS ["kmem.h"]
void Mem_space::switchin_context(Mem_space *from)
{
  (void)from;
  printf("%s FIXME\n", __func__);
}

PUBLIC static inline
bool
Mem_space::is_full_flush(L4_fpage::Rights rights)
{
  return rights & L4_fpage::Rights::R();
}

PUBLIC inline NEEDS["cpu.h"]
static bool
Mem_space::has_superpages()
{
  return Cpu::have_superpages();
}

//we flush tlb in htab implementation
PUBLIC static inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush(bool = false)
{
  //Mem_unit::tlb_flush();
}

PUBLIC static inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush_spaces(bool all, Mem_space *s1, Mem_space *s2)
{
  if (all) // || !Have_asids)
    Mem_unit::tlb_flush();
  else
    {
      if (s1)
        s1->tlb_flush(true);
      if (s2)
        s2->tlb_flush(true);
    }
}

/*
PUBLIC inline
bool 
Mem_space::set_attributes(Address virt, unsigned page_attribs)
{*/
/*
  Pdir::Iter i = _dir->walk(virt);

  if (!i.e->valid() || i.shift() != Config::PAGE_SHIFT)
    return 0;

  i.e->del_attr(Page::MAX_ATTRIBS);
  i.e->add_attr(page_attribs);
  return true;
*/
/*  NOT_IMPL_PANIC;
  return false;
}*/

PROTECTED inline
void
Mem_space::destroy()
{}

/**
 * Destructor.  Deletes the address space and unregisters it from
 * Space_index.
 */
PRIVATE
void
Mem_space::dir_shutdown()
{

  // free ldt memory if it was allocated
  //free_ldt_memory();

  // free all page tables we have allocated for this address space
  // except the ones in kernel space which are always shared
  /*
  _dir->alloc_cast<Mem_space_q_alloc>()
    ->destroy(0, Kmem::mem_user_max, 0, Pdir::Depth,
              Mem_space_q_alloc(_quota, Kmem_alloc::allocator()));
*/
  NOT_IMPL_PANIC;
}

IMPLEMENT inline
Mem_space *
Mem_space::current_mem_space(Cpu_number cpu) /// XXX: do not fix, deprecated, remove!
{
  return _current.cpu(cpu);
}

/** Insert a page-table entry, or upgrade an existing entry with new
    attributes.
    @param phys Physical address (page-aligned).
    @param virt Virtual address for which an entry should be created.
    @param size Size of the page frame -- 4KB or 4MB.
    @param page_attribs Attributes for the mapping (see
                        Mem_space::Page_attrib).
    @return Insert_ok if a new mapping was created;
             Insert_warn_exists if the mapping already exists;
             Insert_warn_attrib_upgrade if the mapping already existed but
                                        attributes could be upgraded;
             Insert_err_nomem if the mapping could not be inserted because
                              the kernel is out of memory;
             Insert_err_exists if the mapping could not be inserted because
                               another mapping occupies the virtual-address
                               range
    @pre phys and virt need to be size-aligned according to the size argument.
 */
IMPLEMENT inline
Mem_space::Status
Mem_space::v_insert(Phys_addr phys, Vaddr virt, Page_order size,
		    Attr page_attribs)
{
  (void)phys; (void)virt; (void)page_attribs;
  assert (cxx::get_lsb(Phys_addr(phys), size) == 0);
  assert (cxx::get_lsb(Virt_addr(virt), size) == 0);
/*
  printf("v_insert: phys %08lx virt %08lx (%s) %p\n", phys, virt, 
         page_attribs & Page_writable?"rw":"ro", this);*/
  return Insert_err_nomem;
}


/**
 * Simple page-table lookup.
 *
 * @param virt Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline NEEDS ["paging.h"]
Address
Mem_space::virt_to_phys (Address virt) const
{
  return dir()->virt_to_phys(virt);
}

PUBLIC inline
virtual Address
Mem_space::virt_to_phys_s0(void *a) const
{
  return dir()->virt_to_phys((Address)a);
}

PUBLIC inline
Address
Mem_space::pmem_to_phys (Address virt) const
{
  return virt;
}



/** Look up a page-table entry.
    @param virt Virtual address for which we try the look up.
    @param phys Meaningful only if we find something (and return true).
                If not 0, we fill in the physical address of the found page
                frame.
    @param page_attribs Meaningful only if we find something (and return true).
                If not 0, we fill in the page attributes for the found page
                frame (see Mem_space::Page_attrib).
    @param size If not 0, we fill in the size of the page-table slot.  If an
                entry was found (and we return true), this is the size
                of the page frame.  If no entry was found (and we
                return false), this is the size of the free slot.  In
                either case, it is either 4KB or 4MB.
    @return True if an entry was found, false otherwise.
 */
IMPLEMENT
bool
Mem_space::v_lookup(Vaddr virt, Phys_addr *phys, Page_order *order,
		    Attr *page_attribs)
{
  (void)virt; (void)phys; (void)order; (void)page_attribs;
  return false;
}

/** Delete page-table entries, or some of the entries' attributes.  This
    function works for one or multiple mappings (in contrast to v_insert!).
    @param virt Virtual address of the memory region that should be changed.
    @param size Size of the memory region that should be changed.
    @param page_attribs If nonzero, delete only the given page attributes.
                        Otherwise, delete the whole entries.
    @return Combined (bit-ORed) page attributes that were removed.  In
            case of errors, ~Page_all_attribs is additionally bit-ORed in.
 */
IMPLEMENT
L4_fpage::Rights
Mem_space::v_delete(Vaddr virt, Page_order size,
		    L4_fpage::Rights page_attribs)
{
  (void)virt; (void)size; (void)page_attribs;
  return page_attribs;
}

PUBLIC static inline
Page_number
Mem_space::canonize(Page_number v)
{ return v; }

IMPLEMENT inline
void
Mem_space::v_set_access_flags(Vaddr, L4_fpage::Rights)
{}
