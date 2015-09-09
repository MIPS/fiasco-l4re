/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "config.h"
#include "initcalls.h"
#include "template_math.h"

EXTENSION class Mem_layout
{
public:
  enum Virt_layout : Address {
    User_max             = 0x7fffffff,
    Utcb_addr            = User_max + 1UL - (Config::PAGE_SIZE * 4),

    Cache_flush_area     = 0x80000000,
    Map_base             = 0xC0000000, // max 32MB kernel memory
    Map_end              = 0xC2000000,
    Caps_start           = 0xC2000000, // max 32 MB for caps
    Caps_end             = 0xC4000000,
    Tbuf_status_page     = 0xC4000000,
    Tbuf_ustatus_page    = Tbuf_status_page,
    Tbuf_buffer_area	 = Tbuf_status_page + Config::PAGE_SIZE,
    Tbuf_ubuffer_area    = Tbuf_buffer_area,
    Jdb_tmp_map_area     = 0xC5000000
  };

  static inline unsigned long round_page(unsigned long addr)
  { return (addr + Config::PAGE_SIZE - 1) & ~(Config::PAGE_SIZE-1); }
  static inline unsigned long trunc_page(unsigned long addr)
  { return addr & ~(Config::PAGE_SIZE-1); }
};


/*
 * Memory segments (32bit kernel mode addresses)
 * These are the traditional names used in the 32-bit universe.
 */
#define KUSEG                   0x00000000
#define KSEG0                   0x80000000
#define KSEG1                   0xa0000000
#define KSEG2                   0xc0000000
#define KSEG3                   0xe0000000

#define CKUSEG                  0x00000000
#define CKSEG0                  0x80000000
#define CKSEG1                  0xa0000000
#define CKSEG2                  0xc0000000
#define CKSEG3                  0xe0000000

//No Paging in kernel mode.
#define _ATYPE_         __PTRDIFF_TYPE__
#define _ATYPE32_       int
#define _ATYPE64_       __s64
#define _CONST64_(x)    x ## LL

#define _ACAST32_               (_ATYPE_)(_ATYPE32_)    /* widen if necessary */
#define _ACAST64_               (_ATYPE64_)             /* do _not_ narrow */

/*
 * Returns the kernel segment base of a given address
 */
#define KSEGX(a)                ((_ACAST32_ (a)) & 0xe0000000)

/*
 * Returns the physical address of a CKSEGx address
 */
#define CPHYSADDR(a)            ((_ACAST32_(a)) & 0x1fffffff)

/*
 * Map an address to a certain kernel segment
 */
#define CKSEG0ADDR(a)           (CPHYSADDR(a) | KSEG0)
#define CKSEG1ADDR(a)           (CPHYSADDR(a) | KSEG1)
#define CKSEG2ADDR(a)           (CPHYSADDR(a) | KSEG2)
#define CKSEG3ADDR(a)           (CPHYSADDR(a) | KSEG3)

#define IS_LOW512(addr) (!((Address)(addr) & (Address) ~0x1fffffffULL))

// ------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "panic.h"
#include <cstdio>
#include <cassert>
#include <kdb_ke.h>

PUBLIC static inline NEEDS[<cassert>]
Address
Mem_layout::phys_to_pmem (Address addr)
{
  if (IS_LOW512(addr) || (addr == CKSEG0ADDR(addr)))
    return CKSEG0ADDR(addr);

  return ~0UL;
}

PUBLIC static
Address
Mem_layout::ioremap_nocache(Address phys_addr, Address size)
{
  Address last_addr;

  /* Don't allow wraparound or zero size */
  last_addr = phys_addr + size - 1;
  if (!size || last_addr < phys_addr)
    return 0;

  /*
  * Map uncached objects in the low 512mb of address space using KSEG1.
  */
  if (IS_LOW512(phys_addr) && IS_LOW512(last_addr))
    return CKSEG1ADDR(phys_addr);

  return 0;
}

PUBLIC static inline NEEDS[<cassert>]
Address
Mem_layout::pmem_to_phys (Address addr)
{
  assert(in_pmem(addr));
  return CPHYSADDR(addr);
}

PUBLIC static inline NEEDS[<kdb_ke.h>]
Address
Mem_layout::pmem_to_phys (const void *ptr)
{
  Address addr = reinterpret_cast<Address>(ptr);

  assert_kdb(in_pmem(addr));
  return CPHYSADDR(addr);
}

PUBLIC static inline
bool
Mem_layout::is_user_space (const Address addr)
{
  return (KSEGX(addr) < KSEG0);
}

PUBLIC static inline
template< typename V >
bool
Mem_layout::read_special_safe(V const * /* *address */, V &/*v*/)
{
  panic("%s not implemented", __PRETTY_FUNCTION__);
  return false;
}

PUBLIC static inline
template< typename T >
T
Mem_layout::read_special_safe(T const *a)
{
  Mword res;
  asm volatile( "lw  %0, 0(%1)\n"
 		: "=r"(res) : "r"(a));
  return T(res);
}


/* no page faults can occur, return true */
#if 0
PUBLIC static inline
bool
Mem_layout::is_special_mapped(void const * /*a*/)
{
  return true;
}
#endif

PUBLIC static inline
Mword
Mem_layout::in_pmem(Address addr)
{
  return KSEGX(addr) == KSEG0;
}
