/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include "uart_karma.h"
#include <string.h>
#include <stdio.h>
#include "hypercall.h"

namespace L4 {

  static char _shared_mem[1024] __attribute__((aligned(4096)));

  static void karma_ser_write(unsigned long opcode, unsigned long val){
      karma_hypercall1(KARMA_MAKE_COMMAND(KARMA_DEVICE_ID(ser), opcode), &val);
  }

  static int karma_ser_read(unsigned long opcode)
  {
      KARMA_READ_IMPL(ser, opcode);
  }

  bool Uart_karma::startup(const L4::Io_register_block*)
  {
    karma_ser_write(karma_ser_df_init, (unsigned long)&_shared_mem);
    return 0;
  }

  void Uart_karma::shutdown()
  {
  }

  int Uart_karma::get_char(bool /*blocking*/) const
  {
    int c;

    // TODO handle non-blocking...
    //while (!char_avail())
    //  if (!blocking)
    //    return -1;

    c = (int)karma_ser_read(karma_ser_df_read);

    return c;
  }

  int Uart_karma::write(char const *s, unsigned long count) const
  {
    if (count > sizeof(_shared_mem))
        count = sizeof(_shared_mem);

    memcpy(_shared_mem, s, count);
    karma_ser_write(karma_ser_df_writeln, count);

    return count;
  }

  void Uart_karma::out_char(char c) const
  {
    _shared_mem[0] = c;
    karma_ser_write(karma_ser_df_writeln, 1);
  }

  /* UNIMPLEMENTED */
  bool Uart_karma::change_mode(Transfer_mode, Baud_rate){ return true; }
  int  Uart_karma::char_avail() const { return 1; }
};
