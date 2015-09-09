/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32]:

#include "mem_unit.h"
#include "ram_quota.h"
#include "mem_region.h"
#include "kmem.h"

PRIVATE //inline
bool
Kmem_alloc::map_pmem(unsigned long phy, unsigned long size)
{
  static unsigned long next_map = Mem_layout::Map_base;
  size = Mem_layout::round_page(size + (phy & ~Config::SUPERPAGE_MASK));
  phy = Mem_layout::trunc_page(phy);

  if (next_map + size > Mem_layout::Map_end)
    return false;

  for (unsigned long i = 0; i <size; i += Config::SUPERPAGE_SIZE)
    {
      auto pte = Kmem::kdir()->walk(Virt_addr(next_map + i), Pdir::Super_level);
      pte.create_page(Phys_mem_addr(phy + i), Page::Attr(Page::Rights::RW()));
      pte.write_back_if(true);
    }
  next_map += size;
  return true;
}


PUBLIC inline NEEDS["kmem.h"]
Address
Kmem_alloc::to_phys(void *v) const
{
  if (Mem_layout::in_pmem ((Address) v))
    return Mem_layout::pmem_to_phys(v);
  else
    return Kmem::kdir()->virt_to_phys((Address)v);
}

IMPLEMENT
Kmem_alloc::Kmem_alloc()
{
  Mword alloc_size = Config::Kmem_size;
  Mem_region_map<64> map;
  Mem_region f;
  unsigned long available_size = create_free_map(Kip::k(), &map);

  printf("Available physical memory: %#lx\n", available_size);

  // sanity check whether the KIP has been filled out, number is arbitrary
  if (available_size < (1 << 18))
    panic("Kmem_alloc: No kernel memory available (%ld)\n",
          available_size);

  for (int i = map.length() - 1; i >= 0 && alloc_size > 0; --i) {
    f = map[i];

    /* Search for a large configuous region of PMEM for KMEM */
    if (f.size() > alloc_size)
      f.start += (f.size() - alloc_size);
    else
      continue;

    alloc_size -= f.size();
  }

  if (alloc_size)
    panic("Kmem_alloc: cannot allocate sufficient kernel memory");

#if 0
  if (!map_pmem(f.start, f.size()))
    panic("Kmem_alloc: cannot map physical memory %p\n", (void*)f.start);
#endif

  Kip::k()->add_mem_region(Mem_desc(f.start, f.end, Mem_desc::Reserved));
  printf("ALLOC1: [%08lx; %08lx] sz=%ld\n", f.start, f.end, f.size());
  printf("Add KMEM memory @ %#lx, size %#lx\n", Mem_layout::phys_to_pmem(f.start), f.size());
  a->init(Mem_layout::phys_to_pmem(f.start));
  a->add_mem((void *)Mem_layout::phys_to_pmem(f.start), f.size());
}

//----------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include <cstdio>

#include "kip_init.h"
#include "panic.h"

PUBLIC
void Kmem_alloc::debug_dump()
{
  a->dump();

  unsigned long free = a->avail();
  printf("Used %ldKB out of %dKB of Kmem\n",
   (Config::Kmem_size - free + 1023)/1024,
   (Config::Kmem_size + 1023)/1024);
}
