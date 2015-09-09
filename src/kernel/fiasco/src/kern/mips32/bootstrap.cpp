/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "types.h"
#include "initcalls.h"

IMPLEMENTATION [mips32]:

#include <cstdlib>
#include <cstdio>
#include <construction.h>
#include "boot_info.h"
#include "terminate.h"

void kernel_main(void);

extern char _bss_start[], _bss_end[];

extern "C" FIASCO_INIT
void bootstrap_main()
{
  /* Clear the BSS */
  __builtin_memset (_bss_start, 0, _bss_end - _bss_start);

  atexit(&static_destruction);
  static_construction();
  printf("Bootstrapped\n");
  kernel_main();
  terminate(0);
}
