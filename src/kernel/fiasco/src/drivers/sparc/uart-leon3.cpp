IMPLEMENTATION[uart_leon3 && libuart]:

#include "uart_leon3.h"
#include "mem_layout.h"

IMPLEMENT Address Uart::base() const { return Mem_layout::Uart_base; }

IMPLEMENT int Uart::irq() const { return 0; }

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_leon3 uart;
  return &uart;
}
