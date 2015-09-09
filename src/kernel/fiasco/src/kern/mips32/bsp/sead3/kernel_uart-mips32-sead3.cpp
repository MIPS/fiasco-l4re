/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE:

EXTENSION class Kernel_uart { enum { Bsp_init_mode = Init_after_mmu }; };

IMPLEMENTATION [mips32 && sead3 && serial]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup(unsigned /*port*/, int /*irq*/)
{
  return Uart::startup();
}
