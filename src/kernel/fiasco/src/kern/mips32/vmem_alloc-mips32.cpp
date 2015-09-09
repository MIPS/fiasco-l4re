/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32]:


#include <cassert>
#include <cstdio>
#include <cstring>
#include "config.h"
#include "kdb_ke.h"
#include "kmem_alloc.h"
#include "mem_layout.h"
#include "mem_space.h"
#include "ram_quota.h"
#include "paging.h"
#include "mem.h"

IMPLEMENT
void Vmem_alloc::init()
{
  NOT_IMPL_PANIC;
}

IMPLEMENT
void *Vmem_alloc::page_alloc(void * address, Zero_fill zf, unsigned mode)
{
  void *vpage = Kmem_alloc::allocator()->alloc(Config::PAGE_SHIFT);

  if (EXPECT_FALSE(!vpage))
    return 0;

  Address page = Mem_layout::pmem_to_phys((Address)vpage);

  Mem_unit::inv_dcache(vpage, ((char*)vpage) + Config::PAGE_SIZE);

  // insert page into master page table
  auto pte = Mem_space::kernel_space()->dir()->walk(Virt_addr(address),
      Pdir::Depth, true, Kmem_alloc::q_allocator(Ram_quota::root));

  Page::Rights r = Page::Rights::RWX();
  if (mode & User)
    r |= Page::Rights::U();

  pte.create_page(Phys_mem_addr(page), Page::Attr(r, Page::Type::Normal(), Page::Kern::Global()));
  pte.write_back_if(true);

  printf("%s: %p -> %#lx\n", __func__, address, page);
  if (zf == ZERO_FILL)
    Mem::memset_mwords((unsigned long *)address, 0, Config::PAGE_SIZE >> 2);

  return address;

}
