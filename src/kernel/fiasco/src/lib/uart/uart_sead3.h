/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2013  MIPS Technologies, Inc.  All rights reserved.
 * Authors: Sanjay Lal <sanjayl@kymasys.com>
*/
#ifndef L4_CXX_UART_SEAD3_H__
#define L4_CXX_UART_SEAD3_H__


#include "uart_base.h"

namespace L4
{
  class Uart_sead3 : public Uart
  {
  public:
    enum
      {
        PAR_NONE = 0x00,
        PAR_EVEN = 0x18,
        PAR_ODD  = 0x08,
        DAT_5    = 0x00,
        DAT_6    = 0x01,
        DAT_7    = 0x02,
        DAT_8    = 0x03,
        STOP_1   = 0x00,
        STOP_2   = 0x04,

        MODE_8N1 = PAR_NONE | DAT_8 | STOP_1,
        MODE_7E1 = PAR_EVEN | DAT_7 | STOP_1,

        // these two values are to leave either mode
        // or baud rate unchanged on a call to change_mode
        MODE_NC  = 0x1000000,
        BAUD_NC  = 0x1000000,

      };

    bool startup(Io_register_block const *regs);
    void shutdown();
    bool change_mode(Transfer_mode m, Baud_rate r);
    int get_char(bool blocking = true) const;
    int char_avail() const;
    inline void out_char(char c) const;
    int write(char const *s, unsigned long count) const;

  private:
    unsigned long _base_rate;
  };
}

#endif /* L4_CXX_UART_SEAD3_H__ */
