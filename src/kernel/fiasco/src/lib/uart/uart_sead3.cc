/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2013  MIPS Technologies, Inc.  All rights reserved.
 * Authors: Sanjay Lal <sanjayl@kymasys.com>
*/

#include "uart_sead3.h"
#include "poll_timeout_counter.h"

namespace L4
{
  enum Registers
  {
    TRB      = 0x00, // Transmit/Receive Buffer  (read/write)
    BRD_LOW  = 0x00, // Baud Rate Divisor LSB if bit 7 of LCR is set  (read/write)
    IER      = 0x01, // Interrupt Enable Register  (read/write)
    BRD_HIGH = 0x01, // Baud Rate Divisor MSB if bit 7 of LCR is set  (read/write)
    IIR      = 0x02, // Interrupt Identification Register  (read only)
    FCR      = 0x02, // 16550 FIFO Control Register  (write only)
    LCR      = 0x03, // Line Control Register  (read/write)
    MCR      = 0x04, // Modem Control Register  (read/write)
    LSR      = 0x05, // Line Status Register  (read only)
    MSR      = 0x06, // Modem Status Register  (read only)
    SPR      = 0x07, // Scratch Pad Register  (read/write)
  };

  bool Uart_sead3::startup(Io_register_block const *regs)
  {
    _regs = regs;

    char scratch, scratch2, scratch3;

    scratch = _regs->read<unsigned int>(IER);
    _regs->write<unsigned int>(IER, 0);

    _regs->delay();

    scratch2 = _regs->read<unsigned int>(IER);
    _regs->write<unsigned int>(IER, 0xf);

    _regs->delay();

    scratch3 = _regs->read<unsigned int>(IER);
    _regs->write<unsigned int>(IER, scratch);

    if (!(scratch2 == 0x00 && scratch3 == 0x0f))
      return false;  // this is not the uart

    /* clearall interrupts */
    _regs->read<unsigned int>(MSR); /* IRQID 0*/
    _regs->read<unsigned int>(IIR); /* IRQID 1*/
    _regs->read<unsigned int>(TRB); /* IRQID 2*/
    _regs->read<unsigned int>(LSR); /* IRQID 3*/

    Poll_timeout_counter i(5000000);
    while (i.test(_regs->read<unsigned int>(LSR) & 1/*DATA READY*/))
      _regs->read<unsigned int>(TRB);

    return true;
  }

  void Uart_sead3::shutdown()
  {
    _regs->write<unsigned int>(MCR, 6);
    _regs->write<unsigned int>(FCR, 0);
    _regs->write<unsigned int>(LCR, 0);
    _regs->write<unsigned int>(IER, 0);
  }

  bool Uart_sead3::change_mode(Transfer_mode m, Baud_rate r)
  {
		return true;
    unsigned long old_lcr = _regs->read<unsigned int>(LCR);
    if(r != BAUD_NC) {
      unsigned short divisor = _base_rate / r;
      _regs->write<unsigned int>(LCR, old_lcr | 0x80/*DLAB*/);
      _regs->write<unsigned int>(TRB, divisor & 0x0ff);        /* BRD_LOW  */
      _regs->write<unsigned int>(IER, (divisor >> 8) & 0x0ff); /* BRD_HIGH */
      _regs->write<unsigned int>(LCR, old_lcr);
    }
    if (m != MODE_NC)
      _regs->write<unsigned int>(LCR, m & 0x07f);

    return true;
  }

  int Uart_sead3::get_char(bool blocking) const
  {
    char old_ier, ch;

    if (!blocking && !char_avail())
      return -1;

    old_ier = _regs->read<unsigned int>(IER);
    _regs->write<unsigned int>(IER, old_ier & ~0xf);
    while (!char_avail())
      ;
    ch = _regs->read<unsigned int>(TRB);
    _regs->write<unsigned int>(IER, old_ier);
    return ch;
  }

  int Uart_sead3::char_avail() const
  {
    return _regs->read<unsigned int>(LSR) & 1; // DATA READY
  }

  void Uart_sead3::out_char(char c) const
  {
    write(&c, 1);
  }

  int Uart_sead3::write(char const *s, unsigned long count) const
  {
    /* disable uart irqs */
    char old_ier;
    unsigned i;
    old_ier = _regs->read<unsigned int>(IER);
    _regs->write<unsigned int>(IER, old_ier & ~0x0f);

    /* transmission */
    Poll_timeout_counter cnt(5000000);
    for (i = 0; i < count; i++) {
      cnt.set(5000000);
      while (cnt.test(!(_regs->read<unsigned int>(LSR) & 0x20 /* THRE */)))
        ;
      _regs->write<unsigned int>(TRB, s[i]);
    }

    /* wait till everything is transmitted */
    cnt.set(5000000);
    while (cnt.test(!(_regs->read<unsigned int>(LSR) & 0x40 /* TSRE */)))
      ;

    _regs->write<unsigned int>(IER, old_ier);
    return count;
  }
};
