/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Sanjay Lal <sanjayl@kymasys.com>
 */

IMPLEMENTATION [sead3]:

#include "uart_sead3.h"
#include "cp0_status.h"
#include "mipsregs.h"

IMPLEMENT Address Uart::base() const { return 0xbf000800; }

IMPLEMENT int Uart::irq() const { return -1; }

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_sead3 uart;
  return &uart;
}
