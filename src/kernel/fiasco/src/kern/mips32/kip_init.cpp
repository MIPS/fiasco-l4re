/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "initcalls.h"
#include "types.h"
#include "kip.h"

class Cpu;

class Kip_init
{
public:
  static void init();
};


//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include <cstring>
#include "config.h"
#include "boot_info.h"
#include "cpu.h"
#include "div32.h"
#include "panic.h"
#include "mem_layout.h"
#include "processor.h"

/** KIP initialization. */
PUBLIC static FIASCO_INIT
void
Kip_init::init_freq(Cpu const &cpu)
{
  (void)cpu;
  Kip::k()->frequency_cpu	= div32(cpu.frequency(), 1000);
}

// Make the stuff below apearing only in this compilation unit.
// Trick Preprocess to let the struct reside in the cc file rather
// than putting it into the _i.h file which is perfectly wrong in 
// this case.
namespace KIP_namespace
{
  enum
  {
    Num_mem_descs = 20,
    Max_len_version = 512,

    Size_mem_descs = sizeof(Mword) * 2 * Num_mem_descs,
  };

  struct KIP
  {
    Kip kip;
    char mem_descs[Size_mem_descs];
  };

  KIP mips32_kernel_info_page asm("mips32_kernel_info_page") __attribute__((section(".kernel_info_page"))) =
    {
      {
	/* 00/00  */ L4_KERNEL_INFO_MAGIC,
	             Config::Kernel_version_id,
	             (Size_mem_descs + sizeof(Kip)) >> 4,
	             {}, 0, {},
	/* 10/20  */ 0, {},
	/* 20/40  */ 0, 0, {},
	/* 30/60  */ 0, 0, {},
	/* 40/80  */ 0, 0, {},
	/* 50/A0  */ 0, (sizeof(Kip) << (sizeof(Mword)*4)) | Num_mem_descs, {},
	/* 60/C0  */ {},
	/* A0/140 */ 0, 0, 0, 0,
	/* B0/160 */ {},
	/* E0/1C0 */ 0, 0, {},
	/* F0/1D0 */ {"", 0, 0, {}},
      },
      {}
    };
};


IMPLEMENT
void Kip_init::init()
{
  Kip *kinfo = reinterpret_cast<Kip*>(&KIP_namespace::mips32_kernel_info_page);
  Kip::init_global_kip(kinfo);

  Kip::k()->clock = 0;
  Kip::k()->sched_granularity = Config::Scheduler_granularity;

  Mem_desc *md = kinfo->mem_descs();
  Mem_desc *end = md + kinfo->num_mem_descs();

  /* add virtual user space */
  kinfo->add_mem_region(Mem_desc(0, Mem_layout::User_max, 
	                Mem_desc::Conventional, true));

  
  for (;md != end; ++md) {
    md->dump();
  }

  kinfo->platform_info.cpuid = Cpu_phys_id::val(Proc::cpu_id());
  Config::probe_sys_controller();
}

