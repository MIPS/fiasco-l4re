/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [libkarma]:

#include "io_regblock.h"
#include "kmem.h"

EXTENSION class Uart
{
  L4::Io_register_block_mmio _regs;
};

IMPLEMENT inline NEEDS["kmem.h"]
IMPLEMENT Uart::Uart() : Console(DISABLED), _regs(base(), 0) {}

PUBLIC bool Uart::startup()
{ return uart()->startup(&_regs); }

