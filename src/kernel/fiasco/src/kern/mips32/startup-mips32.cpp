/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32]:

#include "banner.h"
#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "fpu.h"
#include "kip_init.h"
#include "kernel_task.h"
#include "kmem_alloc.h"
#include "kernel_uart.h"
#include "per_cpu_data.h"
#include "per_cpu_data_alloc.h"
#include "cm.h"
#include "cpc.h"
#include "pic.h"
#include "ipi.h"
#include "static_init.h"
#include "thread.h"
#include "timer.h"
#include "utcb_init.h"
#include "mipsregs.h"
#include "mipsdefs.h"
#include <cstdio>



IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void
Startup::stage1()
{
  Proc::cli();
  Boot_info::init();
  Banner::init();
  Cpu::early_init();
  Config::init();
}

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void
Startup::stage2()
{
  Cpu_number const boot_cpu = Cpu_number::boot_cpu();

  printf("CPU[%d] Hello from Startup::stage2\n", Cpu_number::val(boot_cpu));

  if (Config::Max_num_cpus > 1)
  {
    printf("Max_num_cpus %d\n", Config::Max_num_cpus);
  }

  Mem_op::cache_init();

  /*
   * Copy over General Exception Handling Code to the correct memory location
   */
  printf("Setting up exception handlers... ");
  if (_exceptionhandlerEnd - _exceptionhandler > 0x80) {
    printf("Unable to copy - Main exception vector code too large!\n");
    /* TBD : return -1; */
  }

  memcpy((void *)MIPS_GENERAL_EXC_VECTOR, _exceptionhandler,
    _exceptionhandlerEnd - _exceptionhandler);

  /* Entry point for TLB Miss exceptions when EXL == 0 */
  memcpy((void *)MIPS_UTLB_MISS_EXC_VECTOR, _exceptionhandler,
    _exceptionhandlerEnd - _exceptionhandler);
  printf("done.\n");

  /* Sync the icache */
  Mem_op::cache()->icache_invalidate_by_range((vaddr_t) MIPS_UTLB_MISS_EXC_VECTOR,
    Config::PAGE_SIZE);

  /* Switch to new exception base  */
  write_c0_status(read_c0_status() & ~ST0_BEV);

  Kip_init::init();
  //init paging
  Mem_space::init();
  //init buddy allocator
  Kmem_alloc::init();

  // Initialize cpu-local data management and run constructors for CPU 0
  Per_cpu_data::init_ctors();
  Per_cpu_data_alloc::alloc(boot_cpu);
  Per_cpu_data::run_ctors(boot_cpu);

  //idle task
  Kernel_task::init();
  Mem_space::kernel_space(Kernel_task::kernel_task());
  Cm::cm.construct();
  Cpc::cpc.construct();
  Pic::init();

  //Cpu::init_mmu();
  Cpu::cpus.cpu(boot_cpu).init(true);

  // set frequency in KIP to that of the boot CPU
  Kip_init::init_freq(Cpu::cpus.cpu(boot_cpu));

  Kip::k()->print();

  Fpu::init(boot_cpu, false);
  Ipi::init(boot_cpu);
  Thread_ipi::init(boot_cpu);
  Timer::init(boot_cpu);
  Utcb_init::init();

  puts("Startup::stage2 finished");
}

