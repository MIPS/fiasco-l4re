/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32]:

#include "io.h"
#include "boot_info.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
#define SOFTRES_REGISTER 0xBF000500
#define GORESET 0x42
  *(unsigned *)SOFTRES_REGISTER = GORESET;
  for(;;);
}

