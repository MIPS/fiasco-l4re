/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

EXTENSION class Timer
{
private:
  static Unsigned32 mips_timer_intv;
};

/**
 * MIPS timer using internal count/compare
 */
INTERFACE [mips32]:

#include "irq_chip.h"

IMPLEMENTATION [mips32]:

#include "cpu.h"
#include "config.h"
#include "globals.h"
#include "kip.h"
#include "warn.h"
#include "mipsregs.h"

#include <cstdio>

Unsigned32 Timer::mips_timer_intv;

IMPLEMENT inline NEEDS ["kip.h", "config.h", <cstdio>, "timer.h"]
void
Timer::init(Cpu_number)
{
  printf("Using MIPS count/compare for scheduling\n");

  init_system_clock();

  mips_timer_intv = (Kip::k()->frequency_cpu * 1000)/Config::Scheduler_granularity;
  update_timer(mips_timer_intv);
}

PUBLIC static inline NEEDS["irq_chip.h"]
Irq_chip::Mode
Timer::irq_mode()
{ return Irq_chip::Mode::F_raising_edge; }

PUBLIC static inline NEEDS["mipsregs.h", "timer.h"]
void
Timer::acknowledge()
{ 
  write_c0_compare(read_c0_compare());
  back_to_back_c0_hazard();
  update_timer(mips_timer_intv);
  update_system_clock(current_cpu());
}

IMPLEMENT inline NEEDS ["kip.h"]
void
Timer::init_system_clock()
{
  Kip::k()->clock = 0;
}

IMPLEMENT inline NEEDS ["globals.h", "kip.h"]
Unsigned64
Timer::system_clock()
{
  return Kip::k()->clock;
}

IMPLEMENT inline NEEDS ["config.h", "kip.h"]
void
Timer::update_system_clock(Cpu_number cpu)
{
  if (cpu == Cpu_number::boot_cpu())
  {
    Kip::k()->clock += Config::Scheduler_granularity;
  }
}

IMPLEMENT inline NEEDS["mipsregs.h"]
void
Timer::update_timer(Unsigned64 wakeup)
{
  write_c0_compare(read_c0_count() + wakeup);
  back_to_back_c0_hazard();
}
