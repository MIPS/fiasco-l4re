/*
 * Copyright (C) 2015 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "types.h"
//#define DEBUG

#ifdef DEBUG
#define debug_printf printf
#else
#define debug_printf(format, args...) ((void)0)
#endif

#define preempt_disable() Mem::barrier()
#define preempt_enable()  Mem::barrier()

class Mem_op
{
public:

private:
  struct mips_cache_cfg {
    Unsigned32 icache_size;
    Unsigned32 icache_line_size;
    Unsigned32 icache_ways;
    Unsigned32 icache_way_size;
    Unsigned32 icache_way_mask;

    Unsigned32 dcache_size;
    Unsigned32 dcache_line_size;
    Unsigned32 dcache_ways;
    Unsigned32 dcache_way_size;
    Unsigned32 dcache_way_mask;

    Unsigned32 icache_stride;
    Unsigned32 icache_loopcount;
  };
  mips_cache_cfg cache_cfg;

  /* I-Cache Ops */
  void  (Mem_op::*op_icache_sync_all)(void);
  void  (Mem_op::*op_icache_sync_range)(vaddr_t, vsize_t);
  void  (Mem_op::*op_icache_sync_range_index)(vaddr_t, vsize_t);

  /* D-Cache Ops */
  void  (Mem_op::*op_dcache_wbinv_all)(void);
  void  (Mem_op::*op_dcache_wb_all)(void);
  void  (Mem_op::*op_dcache_wbinv_range)(vaddr_t, vsize_t);
  void  (Mem_op::*op_dcache_wbinv_range_index)(vaddr_t, vsize_t);
  void  (Mem_op::*op_dcache_inv_range)(vaddr_t, vsize_t);
  void  (Mem_op::*op_dcache_wb_range)(vaddr_t, vsize_t);

  static Mem_op *cache_obj;
};

// ------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "mipsregs.h"
#include "processor.h"
#include "cpu.h"
#include "mem_layout.h"
#include "cache_r4k.h"
#include "cache.h"

Mem_op *Mem_op::cache_obj;

PUBLIC static inline Mem_op *Mem_op::cache() { return cache_obj; }

PRIVATE
void Mem_op::decode_cache_config(struct mips_cache_cfg *cfg)
{
  Unsigned32 config = read_c0_config();
  Unsigned32 config1 = read_c0_config1();
  bool flush_multiple_lines_per_way;


  /* Decode D-Cache Parameters */
  switch (MIPS_CACHECFG_GET(CFG1_DL, config1)) {
    case MIPS_CACHECFG_CFG1_DL_NONE:
      cfg->dcache_line_size = cfg->dcache_way_size =
        cfg->dcache_ways = 0;
      break;
    case MIPS_CACHECFG_CFG1_DL_RSVD:
      panic("reserved MIPS32/64 Dcache line size");
      break;
    default:
      if (MIPS_CACHECFG_GET(CFG1_DS, config1) == MIPS_CACHECFG_CFG1_DS_RSVD)
        panic("reserved MIPS32/64 Dcache sets per way");
      cfg->dcache_line_size = MIPS_CACHECFG_CFG1_DL(config1);
      cfg->dcache_way_size =
        cfg->dcache_line_size * MIPS_CACHECFG_CFG1_DS(config1);
      cfg->dcache_ways = MIPS_CACHECFG_CFG1_DA(config1) + 1;

      /*
       * Compute the total size and "way mask" for the
       * primary D-Cache.
       */
      cfg->dcache_size =
        cfg->dcache_way_size * cfg->dcache_ways;
      cfg->dcache_way_mask = cfg->dcache_way_size - 1;
      break;
  }

  /* Decode I-Cache Parameters */
  switch (MIPS_CACHECFG_GET(CFG1_IL, config1)) {
  case MIPS_CACHECFG_CFG1_IL_NONE:
    cfg->icache_line_size = cfg->icache_way_size =
        cfg->icache_ways = 0;
    break;
  case MIPS_CACHECFG_CFG1_IL_RSVD:
    panic("reserved MIPS32/64 Icache line size");
    break;
  default:
    if (MIPS_CACHECFG_GET(CFG1_IS, config1) == MIPS_CACHECFG_CFG1_IS_RSVD)
      panic("reserved MIPS32/64 Icache sets per way");
    cfg->icache_line_size = MIPS_CACHECFG_CFG1_IL(config1);
    cfg->icache_way_size =
        cfg->icache_line_size * MIPS_CACHECFG_CFG1_IS(config1);
    cfg->icache_ways = MIPS_CACHECFG_CFG1_IA(config1) + 1;

    /*
     * Compute the total size and "way mask" for the
     * primary I-Cache.
     */
    cfg->icache_size =
        cfg->icache_way_size * cfg->icache_ways;
    cfg->icache_way_mask = cfg->icache_way_size - 1;
    break;
  }

  flush_multiple_lines_per_way = cfg->icache_way_size > Config::PAGE_SIZE;
  if (config & MIPS_CONF_VI) {
    /*
     * With a virtual Icache we don't need to flush
     * multiples of the page size with index ops; we just
     * need to flush one pages' worth.
     */
    flush_multiple_lines_per_way = false;
  }

  if (flush_multiple_lines_per_way) {
    cfg->icache_stride = Config::PAGE_SIZE;
    cfg->icache_loopcount = (cfg->icache_way_size / Config::PAGE_SIZE) *
        cfg->icache_ways;
  } else {
    cfg->icache_stride = cfg->icache_way_size;
    cfg->icache_loopcount = cfg->icache_ways;
  }

  if (config & MIPS_CONF_VI)
    printf("Instruction Cache is virtual\n");
  printf("icache_stride    = %d\n", cfg->icache_stride);
  printf("icache_loopcount = %d\n", cfg->icache_loopcount);
}

PRIVATE
void Mem_op::print_cache_config(void)
{
  if (cache_cfg.icache_line_size) 
    printf("I-Cache:: Line %d, Total %d, Ways %d\n",
      cache_cfg.icache_line_size, 
      cache_cfg.icache_way_size * cache_cfg.icache_ways,
      cache_cfg.icache_ways);
  else  
    printf("No Instruction Cache configured!\n");

  if (cache_cfg.dcache_line_size) 
    printf("D-Cache:: Line %d, Total %d, Ways %d\n",
      cache_cfg.dcache_line_size, 
      cache_cfg.dcache_way_size * cache_cfg.dcache_ways,
      cache_cfg.dcache_ways);
  else  
    printf("No Data Cache configured!\n");
}

PRIVATE
void Mem_op::init_cache_ops(void)
{
  switch(cache_cfg.icache_line_size) {
    case 16:
      op_icache_sync_all =
        &Mem_op::mips_icache_sync_all_16;
      op_icache_sync_range =
        &Mem_op::mips_icache_sync_range_16;
      op_icache_sync_range_index =
        &Mem_op::mips_icache_sync_range_index_16;
      break;
    case 32:
      op_icache_sync_all =
        &Mem_op::mips_icache_sync_all_32;
      op_icache_sync_range =
        &Mem_op::mips_icache_sync_range_32;
      op_icache_sync_range_index =
        &Mem_op::mips_icache_sync_range_index_32;
      break;
    case 64:
      op_icache_sync_all =
        &Mem_op::mips_icache_sync_all_64;
      op_icache_sync_range =
        &Mem_op::mips_icache_sync_range_64;
      op_icache_sync_range_index =
        &Mem_op::mips_icache_sync_range_index_64;
      break;
    default:
      printf("Unable to set up I-Cache ops!!\n");
      panic("I-Cache Line Size %d not supported\n",
        cache_cfg.icache_line_size);
      break;
  }

  switch (cache_cfg.dcache_line_size) {
    case 16:
      op_dcache_wbinv_all =
          &Mem_op::mips_dcache_wbinv_all_16;
      op_dcache_wb_all =
          &Mem_op::mips_dcache_wb_all_16;
      op_dcache_wbinv_range =
          &Mem_op::mips_dcache_wbinv_range_16;
      op_dcache_wbinv_range_index =
          &Mem_op::mips_dcache_wbinv_range_index_16;
      op_dcache_inv_range =
          &Mem_op::mips_dcache_inv_range_16;
      op_dcache_wb_range =
          &Mem_op::mips_dcache_wb_range_16;
      break;
    case 32:
      op_dcache_wbinv_all =
          &Mem_op::mips_dcache_wbinv_all_32;
      op_dcache_wb_all =
          &Mem_op::mips_dcache_wb_all_32;
      op_dcache_wbinv_range =
          &Mem_op::mips_dcache_wbinv_range_32;
      op_dcache_wbinv_range_index =
          &Mem_op::mips_dcache_wbinv_range_index_32;
      op_dcache_inv_range =
          &Mem_op::mips_dcache_inv_range_32;
      op_dcache_wb_range =
          &Mem_op::mips_dcache_wb_range_32;
      break;
    case 64:
      op_dcache_wbinv_all =
          &Mem_op::mips_dcache_wbinv_all_64;
      op_dcache_wb_all =
          &Mem_op::mips_dcache_wb_all_64;
      op_dcache_wbinv_range =
          &Mem_op::mips_dcache_wbinv_range_64;
      op_dcache_wbinv_range_index =
          &Mem_op::mips_dcache_wbinv_range_index_64;
      op_dcache_inv_range =
          &Mem_op::mips_dcache_inv_range_64;
      op_dcache_wb_range =
          &Mem_op::mips_dcache_wb_range_64;
      break;
    default:
      printf("Unable to set up D-Cache ops!!\n");
      panic("D-Cache Line Size %d not supported\n",
        cache_cfg.dcache_line_size);
      break;
  }
}

PUBLIC static
void Mem_op::cache_init(void)
{
  static Mem_op cache;

  cache.cache_obj = &cache;
  cache.decode_cache_config(&cache.cache_cfg);
  cache.print_cache_config();
  cache.init_cache_ops();
  printf("Caches initialized.\n");
#if 0
  cache.run_cache_test();
#endif
}

PUBLIC
void Mem_op::icache_invalidate_all(void)
{
  (this->*op_icache_sync_all)();
}

PUBLIC
void Mem_op::icache_invalidate_by_range(vaddr_t va, vsize_t size)
{
  (this->*op_icache_sync_range)(va, size);
}

PUBLIC
void Mem_op::icache_invalidate_by_index(vaddr_t va, vsize_t size)
{
  (this->*op_icache_sync_range_index)(va, size);
}

PUBLIC
void Mem_op::dcache_wbinvalidate_all(void)
{
  (this->*op_dcache_wbinv_all)();
}

PUBLIC
void Mem_op::dcache_wb_all(void)
{
  (this->*op_dcache_wb_all)();
}

PUBLIC
void Mem_op::dcache_wbinvalidate_by_range(vaddr_t va, vsize_t size)
{
  (this->*op_dcache_wbinv_range)(va, size);
}

PUBLIC
void Mem_op::dcache_wbinvalidate_by_index(vaddr_t va, vsize_t size)
{
  (this->*op_dcache_wbinv_range_index)(va, size);
}

PUBLIC
void Mem_op::dcache_invalidate_by_range(vaddr_t va, vsize_t size)
{
  (this->*op_dcache_inv_range)(va, size);
}

PUBLIC
void Mem_op::dcache_wb_by_range(vaddr_t va, vsize_t size)
{
  (this->*op_dcache_wb_range)(va, size);
}

PRIVATE
void Mem_op::run_cache_test(void)
{
  /* I-Cache Tests */
  icache_invalidate_all();
  icache_invalidate_by_range(0x80000000, Config::PAGE_SIZE);
  icache_invalidate_by_index(0x80000000, Config::PAGE_SIZE);

  /* D-Cache Tests */
  dcache_wbinvalidate_all();
  dcache_wb_all();
  dcache_wbinvalidate_by_range(0x80000000, Config::PAGE_SIZE);
  dcache_wbinvalidate_by_index(0x80000000, Config::PAGE_SIZE);
  dcache_invalidate_by_range(0x80000000, Config::PAGE_SIZE);
  dcache_wb_by_range(0x80000000, Config::PAGE_SIZE);
}

#define round_line_16(x)   (((x) + 15L) & -16L)
#define trunc_line_16(x)   ((x) & -16L)

PRIVATE
void Mem_op::mips_icache_sync_all_16()
{
  vaddr_t va, eva;

  va = MIPS_PHYS_TO_KSEG0(0);
  eva = va + cache_cfg.icache_size;

  debug_printf("I-Cache Size %d Va 0x%08X, EVa 0x%08X\n",
    cache_cfg.icache_size, (Unsigned32) va, (Unsigned32) eva);

  /*
   * Since we're hitting the whole thing, we don't have to
   * worry about the N different "ways".
   */
  mips_dcache_wbinv_all_16();

  while (va < eva) {
    cache_r4k_op_32lines_16(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
    va += (32 * 16);
  }

  SYNC;
  instruction_hazard();

  debug_printf("Instruction Cache (LSize 16) invalidate complete.\n");
}

PRIVATE
void Mem_op::mips_icache_sync_range_16(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_16(va + size);
  va = trunc_line_16(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  mips_dcache_wb_range_16(va, (eva - va));

  while ((eva - va) >= (32 * 16)) {
    cache_r4k_op_32lines_16(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
    va += (32 * 16);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
    va += 16;
  }

  SYNC;
  instruction_hazard();

  debug_printf("Instruction Cache (LSize 16) invalidate by range complete.\n");
}

PRIVATE
void Mem_op::mips_icache_sync_range_index_16(vaddr_t va, vsize_t size)
{
  vaddr_t eva, tmpva;
  Unsigned32 i, stride, loopcount;

  /*
   * Since we're doing Index ops, we expect to not be able
   * to access the address we've been given.  So, get the
   * bits that determine the cache index, and make a KSEG0
   * address out of them.
   */
  va = MIPS_PHYS_TO_KSEG0(va & cache_cfg.icache_way_mask);

  eva = round_line_16(va + size);
  va = trunc_line_16(va);

  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  /*
   * If we are going to flush more than is in a way, we are flushing
   * everything.
   */
  if (eva - va >= cache_cfg.icache_way_size) {
    mips_icache_sync_all_16();
    return;
  }

  /*
   * GCC generates better code in the loops if we reference local
   * copies of these global variables.
   */
  stride = cache_cfg.icache_stride;
  loopcount = cache_cfg.icache_loopcount;

  mips_dcache_wbinv_range_index_16(va, (eva - va));

  while ((eva - va) >= (8 * 16)) {
    tmpva = va;
    for (i = 0; i < loopcount; i++, tmpva += stride) {
      cache_r4k_op_8lines_16(tmpva,
          CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
    }
    va += 8 * 16;
  }

  while (va < eva) {
    tmpva = va;
    for (i = 0; i < loopcount; i++, tmpva += stride) {
      cache_op_r4k_line(tmpva,
          CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
    }
    va += 16;
  }

  SYNC;
  instruction_hazard();

  debug_printf("Instruction Cache (LSize 16) invalidate by index complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wbinv_all_16(void)
{
  vaddr_t va, eva;

  va = MIPS_PHYS_TO_KSEG0(0);
  eva = va + cache_cfg.dcache_size;
  debug_printf("D-Cache Size %d Va 0x%08X, EVa 0x%08X\n",
    cache_cfg.dcache_size, (Unsigned32) va, (Unsigned32) eva);

  /*
   * Since we're hitting the whole thing, we don't have to
   * worry about the N different "ways".
   */
  while (va < eva) {
    cache_r4k_op_32lines_16(va,
        CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
    va += (32 * 16);
  }

  SYNC;

  debug_printf("Data Cache (LSize 16) write-back and invalidate complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wb_all_16(void)
{
  vaddr_t va, eva;

  va = MIPS_PHYS_TO_KSEG0(0);
  eva = va + cache_cfg.dcache_size;
  debug_printf("D-Cache Size %d Va 0x%08X, EVa 0x%08X\n",
    cache_cfg.dcache_size, (Unsigned32) va, (Unsigned32) eva);

  /*
   * Since we're hitting the whole thing, we don't have to
   * worry about the N different "ways".
   */
  while (va < eva) {
    cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
    va += (32 * 16);
  }

  SYNC;

  debug_printf("Data Cache (LSize 16) write-back complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wbinv_range_16(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_16(va + size);
  va = trunc_line_16(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  while ((eva - va) >= (32 * 16)) {
    cache_r4k_op_32lines_16(va,
        CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
    va += (32 * 16);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
    va += 16;
  }

  SYNC;

  debug_printf("Data Cache (LSize 16) write-back and invalidate by range complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_internal_wbinv_range_index_16(vaddr_t va, vaddr_t eva)
{
  /*
   * Since we're doing Index ops, we expect to not be able
   * to access the address we've been given.  So, get the
   * bits that determine the cache index, and make a KSEG0
   * address out of them.
   */
  va = MIPS_PHYS_TO_KSEG0(va);
  eva = MIPS_PHYS_TO_KSEG0(eva);
  debug_printf("Va 0x%08X, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) eva);

  for (; (eva - va) >= (8 * 16); va += 8 * 16) {
    cache_r4k_op_8lines_16(va,
        CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
  }

  for (; va < eva; va += 16) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
  }
}

PRIVATE
void Mem_op::mips_dcache_wbinv_range_index_16(vaddr_t va, vsize_t size)
{
  const vaddr_t way_size = cache_cfg.dcache_way_size;
  const vaddr_t way_mask = way_size - 1;
  const Unsigned32 ways = cache_cfg.dcache_ways;
  vaddr_t eva;

  va &= way_mask;
  eva = round_line_16(va + size);
  va = trunc_line_16(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  /*
   * If we are going to flush more than is in a way, we are flushing
   * everything.
   */
  if (eva - va >= way_size) {
    mips_dcache_wbinv_all_16();
    return;
  }

  /*
   * Invalidate each way.  If the address range wraps past the end of
   * the way, we will be invalidating in two ways but eventually things
   * work out since the last way will wrap into the first way.
   */
  for (Unsigned32 way = 0; way < ways; way++) {
    mips_dcache_internal_wbinv_range_index_16(va, eva);
    va += way_size;
    eva += way_size;
  }

  debug_printf("Data Cache (LSize 16) write-back and invalidate by index complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_inv_range_16(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_16(va + size);
  va = trunc_line_16(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  while ((eva - va) >= (32 * 16)) {
    cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
    va += (32 * 16);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
    va += 16;
  }

  SYNC;

  debug_printf("Data Cache (LSize 16) invalidate by range complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wb_range_16(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_16(va + size);
  va = trunc_line_16(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  while ((eva - va) >= (32 * 16)) {
    cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
    va += (32 * 16);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
    va += 16;
  }

  SYNC;

  debug_printf("Data Cache (LSize 16) write-back by range complete.\n");
}

#define round_line_32(x)   (((x) + 31L) & -32L)
#define trunc_line_32(x)   ((x) & -32L)

PRIVATE
void Mem_op::mips_icache_sync_all_32(void)
{
  vaddr_t va, eva;

  va = MIPS_PHYS_TO_KSEG0(0);
  eva = va + cache_cfg.icache_size;
  debug_printf("I-Cache Size %d Va 0x%08X, EVa 0x%08X\n",
    cache_cfg.icache_size, (Unsigned32) va, (Unsigned32) eva);

  /*
   * Since we're hitting the whole thing, we don't have to
   * worry about the N different "ways".
   */
  mips_dcache_wbinv_all_32();

  while (va < eva) {
    cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
    va += (32 * 32);
  }

  SYNC;
  instruction_hazard();

  debug_printf("Instruction Cache (LSize 32) invalidate complete.\n");
}

PRIVATE
void Mem_op::mips_icache_sync_range_32(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_32(va + size);
  va = trunc_line_32(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  mips_dcache_wb_range_32(va, (eva - va));

  while ((eva - va) >= (32 * 32)) {
    cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
    va += (32 * 32);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
    va += 32;
  }

  SYNC;
  instruction_hazard();

  debug_printf("Instruction Cache (LSize 32) invalidate by range complete.\n");
}

PRIVATE
void Mem_op::mips_icache_sync_range_index_32(vaddr_t va, vsize_t size)
{
  vaddr_t eva, tmpva;
  Unsigned32 i, stride, loopcount;

  /*
   * Since we're doing Index ops, we expect to not be able
   * to access the address we've been given.  So, get the
   * bits that determine the cache index, and make a KSEG0
   * address out of them.
   */
  va = MIPS_PHYS_TO_KSEG0(va & cache_cfg.icache_way_mask);

  eva = round_line_32(va + size);
  va = trunc_line_32(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  /*
   * If we are going to flush more than is in a way, we are flushing
   * everything.
   */
  if (eva - va >= cache_cfg.icache_way_size) {
    mips_icache_sync_all_32();
    return;
  }

  /*
   * GCC generates better code in the loops if we reference local
   * copies of these global variables.
   */
  stride = cache_cfg.icache_stride;
  loopcount = cache_cfg.icache_loopcount;

  mips_dcache_wbinv_range_index_32(va, (eva - va));

  while ((eva - va) >= (8 * 32)) {
    tmpva = va;
    for (i = 0; i < loopcount; i++, tmpva += stride) {
      cache_r4k_op_8lines_32(tmpva,
          CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
    }
    va += 8 * 32;
  }

  while (va < eva) {
    tmpva = va;
    for (i = 0; i < loopcount; i++, tmpva += stride) {
      cache_op_r4k_line(tmpva,
          CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
    }
    va += 32;
  }

  SYNC;
  instruction_hazard();

  debug_printf("Instruction Cache (LSize 32) invalidate by index complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wbinv_all_32(void)
{
  vaddr_t va, eva;

  va = MIPS_PHYS_TO_KSEG0(0);
  eva = va + cache_cfg.dcache_size;
  debug_printf("D-Cache Size %d Va 0x%08X, EVa 0x%08X\n",
    cache_cfg.dcache_size, (Unsigned32) va, (Unsigned32) eva);

  /*
   * Since we're hitting the whole thing, we don't have to
   * worry about the N different "ways".
   */
  while (va < eva) {
    cache_r4k_op_32lines_32(va,
        CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
    va += (32 * 32);
  }

  SYNC;

  debug_printf("Data Cache (LSize 32) write-back and invalidate complete.\n");
}

void Mem_op::mips_dcache_wb_all_32(void)
{
  vaddr_t va, eva;

  va = MIPS_PHYS_TO_KSEG0(0);
  eva = va + cache_cfg.dcache_size;
  debug_printf("D-Cache Size %d Va 0x%08X, EVa 0x%08X\n",
    cache_cfg.dcache_size, (Unsigned32) va, (Unsigned32) eva);

  /*
   * Since we're hitting the whole thing, we don't have to
   * worry about the N different "ways".
   */
  while (va < eva) {
    cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
    va += (32 * 32);
  }

  SYNC;

  debug_printf("Data Cache (LSize 32) write-back complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wbinv_range_32(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_32(va + size);
  va = trunc_line_32(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  while ((eva - va) >= (32 * 32)) {
    cache_r4k_op_32lines_32(va,
        CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
    va += (32 * 32);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
    va += 32;
  }

  SYNC;

  debug_printf("Data Cache (LSize 32) write-back and invalidate by range complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_internal_wbinv_range_index_32(vaddr_t va, vaddr_t eva)
{
  /*
   * Since we're doing Index ops, we expect to not be able
   * to access the address we've been given.  So, get the
   * bits that determine the cache index, and make a KSEG0
   * address out of them.
   */
  va = MIPS_PHYS_TO_KSEG0(va);
  eva = MIPS_PHYS_TO_KSEG0(eva);
  debug_printf("Va 0x%08X, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) eva);

  for (; (eva - va) >= (8 * 32); va += 8 * 32) {
    cache_r4k_op_8lines_32(va,
        CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
  }

  for (; va < eva; va += 32) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
  }
}

PRIVATE
void Mem_op::mips_dcache_wbinv_range_index_32(vaddr_t va, vsize_t size)
{
  const vaddr_t way_size = cache_cfg.dcache_way_size;
  const vaddr_t way_mask = way_size - 1;
  const Unsigned32 ways = cache_cfg.dcache_ways;
  vaddr_t eva;

  va &= way_mask;
  eva = round_line_32(va + size);
  va = trunc_line_32(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  /*
   * If we are going to flush more than is in a way, we are flushing
   * everything.
   */
  if (eva - va >= way_size) {
    mips_dcache_wbinv_all_32();
    return;
  }

  /*
   * Invalidate each way.  If the address range wraps past the end of
   * the way, we will be invalidating in two ways but eventually things
   * work out since the last way will wrap into the first way.
   */
  for (Unsigned32 way = 0; way < ways; way++) {
    mips_dcache_internal_wbinv_range_index_32(va, eva);
    va += way_size;
    eva += way_size;
  }

  debug_printf("Data Cache (LSize 32) write-back and invalidate by index complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_inv_range_32(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_32(va + size);
  va = trunc_line_32(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  while ((eva - va) >= (32 * 32)) {
    cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
    va += (32 * 32);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
    va += 32;
  }

  SYNC;

  debug_printf("Data Cache (LSize 32) invalidate by range complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wb_range_32(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_32(va + size);
  va = trunc_line_32(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  while ((eva - va) >= (32 * 32)) {
    cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
    va += (32 * 32);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
    va += 32;
  }

  SYNC;

  debug_printf("Data Cache (LSize 32) write-back by range complete.\n");
}

#define round_line_64(x)   (((x) + 63L) & -64L)
#define trunc_line_64(x)   ((x) & -64L)

PRIVATE
void Mem_op::mips_icache_sync_all_64()
{
  vaddr_t va, eva;

  va = MIPS_PHYS_TO_KSEG0(0);
  eva = va + cache_cfg.icache_size;

  debug_printf("I-Cache Size %d Va 0x%08X, EVa 0x%08X\n",
    cache_cfg.icache_size, (Unsigned32) va, (Unsigned32) eva);

  /*
   * Since we're hitting the whole thing, we don't have to
   * worry about the N different "ways".
   */
  mips_dcache_wbinv_all_64();

  while (va < eva) {
    cache_r4k_op_32lines_64(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
    va += (32 * 64);
  }

  SYNC;
  instruction_hazard();

  debug_printf("Instruction Cache (LSize 64) invalidate complete.\n");
}

PRIVATE
void Mem_op::mips_icache_sync_range_64(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_64(va + size);
  va = trunc_line_64(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  mips_dcache_wb_range_64(va, (eva - va));

  while ((eva - va) >= (32 * 64)) {
    cache_r4k_op_32lines_64(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
    va += (32 * 64);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
    va += 64;
  }

  SYNC;
  instruction_hazard();

  debug_printf("Instruction Cache (LSize 64) invalidate by range complete.\n");
}

PRIVATE
void Mem_op::mips_icache_sync_range_index_64(vaddr_t va, vsize_t size)
{
  vaddr_t eva, tmpva;
  Unsigned32 i, stride, loopcount;

  /*
   * Since we're doing Index ops, we expect to not be able
   * to access the address we've been given.  So, get the
   * bits that determine the cache index, and make a KSEG0
   * address out of them.
   */
  va = MIPS_PHYS_TO_KSEG0(va & cache_cfg.icache_way_mask);

  eva = round_line_64(va + size);
  va = trunc_line_64(va);

  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  /*
   * If we are going to flush more than is in a way, we are flushing
   * everything.
   */
  if (eva - va >= cache_cfg.icache_way_size) {
    mips_icache_sync_all_64();
    return;
  }

  /*
   * GCC generates better code in the loops if we reference local
   * copies of these global variables.
   */
  stride = cache_cfg.icache_stride;
  loopcount = cache_cfg.icache_loopcount;

  mips_dcache_wbinv_range_index_64(va, (eva - va));

  while ((eva - va) >= (8 * 64)) {
    tmpva = va;
    for (i = 0; i < loopcount; i++, tmpva += stride) {
      cache_r4k_op_8lines_64(tmpva,
          CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
    }
    va += 8 * 64;
  }

  while (va < eva) {
    tmpva = va;
    for (i = 0; i < loopcount; i++, tmpva += stride) {
      cache_op_r4k_line(tmpva,
          CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
    }
    va += 64;
  }

  SYNC;
  instruction_hazard();

  debug_printf("Instruction Cache (LSize 64) invalidate by index complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wbinv_all_64(void)
{
  vaddr_t va, eva;

  va = MIPS_PHYS_TO_KSEG0(0);
  eva = va + cache_cfg.dcache_size;
  debug_printf("D-Cache Size %d Va 0x%08X, EVa 0x%08X\n",
    cache_cfg.dcache_size, (Unsigned32) va, (Unsigned32) eva);

  /*
   * Since we're hitting the whole thing, we don't have to
   * worry about the N different "ways".
   */
  while (va < eva) {
    cache_r4k_op_32lines_64(va,
        CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
    va += (32 * 64);
  }

  SYNC;

  debug_printf("Data Cache (LSize 64) write-back and invalidate complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wb_all_64(void)
{
  vaddr_t va, eva;

  va = MIPS_PHYS_TO_KSEG0(0);
  eva = va + cache_cfg.dcache_size;
  debug_printf("D-Cache Size %d Va 0x%08X, EVa 0x%08X\n",
    cache_cfg.dcache_size, (Unsigned32) va, (Unsigned32) eva);

  /*
   * Since we're hitting the whole thing, we don't have to
   * worry about the N different "ways".
   */
  while (va < eva) {
    cache_r4k_op_32lines_64(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
    va += (32 * 64);
  }

  SYNC;

  debug_printf("Data Cache (LSize 64) write-back complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wbinv_range_64(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_64(va + size);
  va = trunc_line_64(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  while ((eva - va) >= (32 * 64)) {
    cache_r4k_op_32lines_64(va,
        CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
    va += (32 * 64);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
    va += 64;
  }

  SYNC;

  debug_printf("Data Cache (LSize 64) write-back and invalidate by range complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_internal_wbinv_range_index_64(vaddr_t va, vaddr_t eva)
{
  /*
   * Since we're doing Index ops, we expect to not be able
   * to access the address we've been given.  So, get the
   * bits that determine the cache index, and make a KSEG0
   * address out of them.
   */
  va = MIPS_PHYS_TO_KSEG0(va);
  eva = MIPS_PHYS_TO_KSEG0(eva);
  debug_printf("Va 0x%08X, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) eva);

  for (; (eva - va) >= (8 * 64); va += 8 * 64) {
    cache_r4k_op_8lines_64(va,
        CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
  }

  for (; va < eva; va += 64) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
  }
}

PRIVATE
void Mem_op::mips_dcache_wbinv_range_index_64(vaddr_t va, vsize_t size)
{
  const vaddr_t way_size = cache_cfg.dcache_way_size;
  const vaddr_t way_mask = way_size - 1;
  const Unsigned32 ways = cache_cfg.dcache_ways;
  vaddr_t eva;

  va &= way_mask;
  eva = round_line_64(va + size);
  va = trunc_line_64(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  /*
   * If we are going to flush more than is in a way, we are flushing
   * everything.
   */
  if (eva - va >= way_size) {
    mips_dcache_wbinv_all_64();
    return;
  }

  /*
   * Invalidate each way.  If the address range wraps past the end of
   * the way, we will be invalidating in two ways but eventually things
   * work out since the last way will wrap into the first way.
   */
  for (Unsigned32 way = 0; way < ways; way++) {
    mips_dcache_internal_wbinv_range_index_64(va, eva);
    va += way_size;
    eva += way_size;
  }

  debug_printf("Data Cache (LSize 64) write-back and invalidate by index complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_inv_range_64(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_64(va + size);
  va = trunc_line_64(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  while ((eva - va) >= (32 * 64)) {
    cache_r4k_op_32lines_64(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
    va += (32 * 64);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
    va += 64;
  }

  SYNC;

  debug_printf("Data Cache (LSize 64) invalidate by range complete.\n");
}

PRIVATE
void Mem_op::mips_dcache_wb_range_64(vaddr_t va, vsize_t size)
{
  vaddr_t eva;

  eva = round_line_64(va + size);
  va = trunc_line_64(va);
  debug_printf("Va 0x%08X, Size %d, EVa 0x%08X\n",
    (Unsigned32) va, (Unsigned32) size, (Unsigned32) eva);

  while ((eva - va) >= (32 * 64)) {
    cache_r4k_op_32lines_64(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
    va += (32 * 64);
  }

  while (va < eva) {
    cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
    va += 64;
  }

  SYNC;

  debug_printf("Data Cache (LSize 64) write-back by range complete.\n");
}
