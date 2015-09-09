/*
 * Derived in part from linux arch/mips/mti-malta/malta-amon.c
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 MIPS Technologies, Inc.  All rights reserved.
 * Copyright (C) 2013 Imagination Technologies Ltd.
 *
 * Arbitrary Monitor Interface
 */

INTERFACE [mips32 && mp && malta]:

#include "types.h"

IMPLEMENTATION [mips32 && mp && malta]:

#include <stdio.h>

#include "launch.h"
#include "config.h"
#include "mem_layout.h"
#include "processor.h"
#include "mem.h"

PRIVATE static
int
Platform_control::amon_cpu_ready(int cpu)
{
        struct cpulaunch *launch = (struct cpulaunch *)CKSEG0ADDR(CPULAUNCH);

        if (cpu < 0 || cpu >= NCPULAUNCH) {
                printf("avail: cpu%d is out of range\n", cpu);
                return 0;
        }

        launch += cpu;
        if (!(launch->flags & LAUNCH_FREADY)) {
                return 0;
        }

        return 1;
}

PRIVATE static
int
Platform_control::amon_cpu_avail(int cpu)
{
        struct cpulaunch *launch = (struct cpulaunch *)CKSEG0ADDR(CPULAUNCH);

        if (cpu < 0 || cpu >= NCPULAUNCH) {
                printf("avail: cpu%d is out of range\n", cpu);
                return 0;
        }

        launch += cpu;
        if (!(launch->flags & LAUNCH_FREADY)) {
                printf("avail: cpu%d is not ready\n", cpu);
                return 0;
        }
        if (launch->flags & (LAUNCH_FGO|LAUNCH_FGONE)) {
                printf("avail: too late.. cpu%d is already gone\n", cpu);
                return 0;
        }

        return 1;
}

PRIVATE static
void
Platform_control::amon_cpu_start(int cpu,
                    unsigned long pc, unsigned long sp,
                    unsigned long gp, unsigned long a0)
{
        volatile struct cpulaunch *launch =
                (struct cpulaunch  *)CKSEG0ADDR(CPULAUNCH);

        if (!amon_cpu_avail(cpu))
                return;

        if (cpu == Cpu_phys_id::val(Proc::cpu_id())) {
                printf("launch: I am cpu%d!\n", cpu);
                return;
        }
        launch += cpu;

        printf("launch: starting cpu%d\n", cpu);

        launch->pc = pc;
        launch->gp = gp;
        launch->sp = sp;
        launch->a0 = a0;

        Mem::mp_wmb();          /* Target must see parameters before go */
        launch->flags |= LAUNCH_FGO;
        Mem::mp_wmb();          /* Target must see go before we poll  */

        while ((launch->flags & LAUNCH_FGONE) == 0)
                ;
        Mem::mp_rmb();          /* Target will be updating flags soon */

        printf("launch: cpu%d gone!\n", cpu);
}

PUBLIC static
void
Platform_control::boot_secondary(Address start_pc, Address start_sp)
{
  for (Cpu_phys_id i = Cpu_phys_id(1);
       i < Cpu_phys_id(NCPULAUNCH) && i < Cpu_phys_id(Config::Max_num_cpus);
       ++i)
    {
      int cpu = Cpu_phys_id::val(i);
      amon_cpu_start(cpu, start_pc, start_sp, 0, 0);
    }
}

PUBLIC static
int
Platform_control::max_secondary_cpus()
{
  return NCPULAUNCH - 1;
}

IMPLEMENTATION [mips32 && !mp && malta]:

PUBLIC static
int
Platform_control::max_secondary_cpus()
{
  return 0;
}
