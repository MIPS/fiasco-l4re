/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [karma]:

#include "uart_karma.h"

IMPLEMENT Address Uart::base() const { return -1; }

IMPLEMENT int Uart::irq() const { return -1; }

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_karma uart;
  return &uart;
}
