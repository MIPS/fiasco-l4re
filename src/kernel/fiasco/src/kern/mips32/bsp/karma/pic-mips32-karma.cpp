/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32 && karma]:

#include "irq_chip_generic.h"

/**
 * IRQ Chip for the Karma guest platform.
 *
 * Supports MIPS cp0 interrupt sources.
 *
 */
class Irq_chip_karma : public Irq_chip_gen
{
public:
  /*
   * irqs 0..7 - MIPS cp0 sw0..sw1 and hw0..hw5 interrupt sources
   */
  enum {
    MIPS_CPU_IRQ_BASE = 0,
    MIPS_CPU_NUMIRQS  = 8, // No support for Config3.IPLW=1 yet
    MIPS_NUMIRQS      = MIPS_CPU_NUMIRQS,
  };

  char const *chip_type() const { return "Irq_chip_karma"; }
};

// ---------------------------------------------------------------------
IMPLEMENTATION [mips32 && karma]:

#include "types.h"
#include "initcalls.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"
#include "mipsregs.h"
#include "cp0_status.h"
#include "processor.h"
#include "io.h"
#include "assert.h"

EXTENSION class Irq_chip_karma
{
public:
  int set_mode(Mword, Mode) { return 0; }
  bool is_edge_triggered(Mword) const { return false; }
  void set_cpu(Mword, Cpu_number) {}
  void ack(Mword) { /* ack is empty */ }
};

PUBLIC
Irq_chip_karma::Irq_chip_karma() : Irq_chip_gen(MIPS_NUMIRQS)
{
#if defined(CONFIG_JDB) && defined (CONFIG_PF_karma)
#error JDB not supported due to lack of interrupt support
#endif
}

PUBLIC
void
Irq_chip_karma::mask(Mword irq)
{
  if (irq < Irq_chip_karma::MIPS_CPU_NUMIRQS)
    Cp0_Status::clear_status_bit(0x1 << (STATUSB_IP0 + (irq - Irq_chip_karma::MIPS_CPU_IRQ_BASE)));
}

PUBLIC
void
Irq_chip_karma::mask_and_ack(Mword irq)
{
  mask(irq);
  ack(irq);
}

PUBLIC
void
Irq_chip_karma::unmask(Mword irq)
{
  if (irq < Irq_chip_karma::MIPS_CPU_NUMIRQS)
    Cp0_Status::set_status_bit(0x1 << (STATUSB_IP0 + (irq - Irq_chip_karma::MIPS_CPU_IRQ_BASE)));
}

PUBLIC
void
Irq_chip_karma::irq_handler(Unsigned32 status, Unsigned32 cause, bool is_guestcontext)
{
  Mword p = cause & CAUSEF_IP;
  p &= status;

  assert(!is_guestcontext); // is_guestcontext not used in irq_handler

  if (!is_guestcontext) {
    while (p) {
      Mword cp0_irq = 23 - __builtin_clz(p);
      handle_irq<Irq_chip_karma>(cp0_irq, 0);
      p &= ~(0x1 << (STATUSB_IP0 + cp0_irq));
    }
  }
}

extern "C"
void irq_handler(Unsigned32 status, Unsigned32 cause, bool is_guestcontext)
{ mgr->c.irq_handler(status, cause, is_guestcontext); }

static Static_object<Irq_mgr_single_chip<Irq_chip_karma> > mgr;

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  //
  // construct Irq_mgr and Irq_chip_karma
  //
  Irq_mgr::mgr = mgr.construct();
}
