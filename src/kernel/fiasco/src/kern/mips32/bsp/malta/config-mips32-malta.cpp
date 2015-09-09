/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32 && malta]:

#define TARGET_NAME "MALTA"

IMPLEMENTATION [mips32 && malta]:

#include "platform_control.h"
#include "warn.h"

IMPLEMENT_DEFAULT FIASCO_INIT
void
Config::init_arch_platform()
{
  Config::num_ap_cpus = Platform_control::max_secondary_cpus();
  if (Config::Max_num_cpus > (Config::num_ap_cpus + 1))
    WARN("CONFIG_MP_MAX_CPUS %d exceeds Malta platform limit of %d.\n",
        Config::Max_num_cpus, Config::num_ap_cpus + 1);
}
