/*
 * \brief  Support for the Karma guest platform
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include "support.h"
#include <l4/karma/uart_karma.h>
#include "platform.h"

#include <string.h>
#include "startup.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

namespace {
class Platform_mips_karma_guest : public Platform_single_region_ram
{

public :
  bool probe() { return true; }

  void init()
  {
    unsigned long uart_base = 0;

    static L4::Io_register_block_mmio r(uart_base);
    static L4::Uart_karma _uart;
    _uart.startup(&r);
    set_stdio_uart(&_uart);

    kuart.access_type  = L4_kernel_options::Uart_type_mmio;
    kuart.irqno        = 2;
    kuart.base_address = uart_base;
    kuart.base_baud    = 38400;
    kuart.baud         = 38400;
    kuart_flags       |=   L4_kernel_options::F_uart_base
                         | L4_kernel_options::F_uart_baud;
    kuart_flags |= L4_kernel_options::F_uart_irq;
  }

  const char *get_platform_name(void) {
    return "Karma Guest";
  }
};
}

REGISTER_PLATFORM(Platform_mips_karma_guest);
