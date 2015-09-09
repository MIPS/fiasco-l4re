/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32 && !pic_gic]:

PUBLIC
static bool Pic::gic_present()
{
  return false;
}

PUBLIC
static bool Pic::is_gic_int(unsigned)
{
  return false;
}

PUBLIC
static bool Pic::gic_assigned_cp0_irq(unsigned)
{
  return false;
}

PUBLIC
static void Pic::gic_disable_locked(unsigned)
{
}

PUBLIC
static void Pic::gic_enable_locked(unsigned)
{
}

PUBLIC
static void Pic::gic_acknowledge_locked(unsigned)
{
}

PUBLIC
static void Pic::gic_handle_irq(void)
{
}

//-------------------------------------------------------------------
IMPLEMENTATION [mips32 && pic_gic]:

#include "gic.h"
#include "initcalls.h"
#include "bitmap_linux.h"
#include "find_next_bit.h"

EXTENSION class Pic
{
public:
  static Static_object<Gic> gic;
private:
  static bool _gic_present;
};

Static_object<Gic> Pic::gic;

bool Pic::_gic_present = false;

PUBLIC
static bool Pic::gic_present()
{
  return _gic_present;
}

PUBLIC
static bool Pic::is_gic_int(unsigned irq)
{
  return ((irq >= Irq_chip_malta::MIPS_GIC_IRQ_BASE) &&
      (irq < (Irq_chip_malta::MIPS_GIC_IRQ_BASE + Pic::gic->nr_irqs())));
}

PUBLIC
static bool Pic::gic_assigned_cp0_irq(unsigned irq)
{
  return Pic::gic->ipi_mapped(irq);
}

PUBLIC
static void Pic::gic_disable_locked(unsigned irq)
{
  Pic::gic->disable_locked(irq);
}

PUBLIC
static void Pic::gic_enable_locked(unsigned irq)
{
  Pic::gic->enable_locked(irq);
}

PUBLIC
static void Pic::gic_acknowledge_locked(unsigned irq)
{
  Pic::gic->acknowledge_locked(irq);
}

PUBLIC
static void Pic::gic_handle_irq(void)
{
  unsigned long irq;
  DECLARE_BITMAP(pending, Gic::GIC_NUM_INTRS);

  Pic::gic->gic_get_int_mask(pending, Pic::gic->get_ipi_ints());

  irq = find_first_bit(pending, Gic::GIC_NUM_INTRS);

  while (irq < Gic::GIC_NUM_INTRS) {
    Irq_mgr::mgr->chip(Irq_chip_malta::MIPS_GIC_IRQ_BASE + irq).chip->handle_irq<Irq_chip_malta>(Irq_chip_malta::MIPS_GIC_IRQ_BASE + irq, 0);

    irq = find_next_bit(pending, Gic::GIC_NUM_INTRS, irq + 1);
  }
}
