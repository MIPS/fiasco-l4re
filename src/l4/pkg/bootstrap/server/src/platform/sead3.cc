/*
 * \brief  Support for the sead3 platform
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 */

#include "support.h"
#include <l4/drivers/uart_sead3.h>
#include "platform.h"

#include <string.h>
#include "startup.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

namespace {
class Platform_mips_sead3 : public Platform_single_region_ram
{

public :  
  bool probe() { return true; }

  void init()
  {
    unsigned long uart_base = 0xbf000800;

    static L4::Io_register_block_mmio r(uart_base, 2); // uart1
    static L4::Uart_sead3 _uart(14745600);
    _uart.startup(&r);
    set_stdio_uart(&_uart);

    kuart.access_type  = L4_kernel_options::Uart_type_mmio;
    kuart.irqno        = 2;
    kuart.base_address = uart_base;
    kuart.baud         = 38400;
    kuart_flags       |=   L4_kernel_options::F_uart_base
                         | L4_kernel_options::F_uart_baud;
    kuart_flags |= L4_kernel_options::F_uart_irq;
  }

  const char *get_platform_name(void) {
    return "sead3";
  }
};
}

REGISTER_PLATFORM(Platform_mips_sead3);
