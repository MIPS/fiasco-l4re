/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32 && malta]:

#include "timer.h"
#include "pic.h"

EXTENSION class Timer
{
public:
  static unsigned irq() { return Irq_chip_malta::MIPS_CPU_IRQ_BASE + Irq_chip_malta::MIPS_CPU_IRQ_TMR; }
};
