/*
 * Copyright (C) 2015 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE:

#include "mem.h"
#include "std_macros.h"
#include "processor.h"

//-----------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "mem_op-mips32.h"

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache()
{
  Mem_op::cache()->dcache_wb_all();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache(void const *va)
{
  Mem_op::cache()->dcache_wb_by_range((Address)va, 1);
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache(void const *start, void const *end)
{
  vsize_t sz = (Address)end - (Address)start;

  Mem_op::cache()->dcache_wb_by_range((Address)start, sz);
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_cache()
{
  // invalidating the I cache already does D cache flush (wb inv)
  //Mem_op::cache()->dcache_wbinvalidate_all();
  Mem_op::cache()->icache_invalidate_all();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_cache(void const *start, void const *end)
{
  vsize_t sz = (Address)end - (Address)start;

  // invalidating the I cache already does D cache flush (wb inv)
  //Mem_op::cache()->dcache_wbinvalidate_by_range((Address)start, sz);
  Mem_op::cache()->icache_invalidate_by_range((Address)start, sz);
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache()
{
  Mem_op::cache()->dcache_wbinvalidate_all();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache(void const *start, void const *end)
{
  vsize_t sz = (Address)end - (Address)start;

  Mem_op::cache()->dcache_wbinvalidate_by_range((Address)start, sz);
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::inv_dcache(void const *start, void const *end)
{
  vsize_t sz = (Address)end - (Address)start;

  Mem_op::cache()->dcache_invalidate_by_range((Address)start, sz);
}
