/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32 && sead3]:

#include "timer.h"

EXTENSION class Timer
{
public:
  static unsigned irq() { return 7; }
};
