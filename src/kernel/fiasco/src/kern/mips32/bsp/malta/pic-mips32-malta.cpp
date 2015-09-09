/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32 && malta]:

EXTENSION class Pic
{
public:
  enum { NR_IRQS = 256 };
};

// --------------------------------------------------------------------------
INTERFACE [mips32 && malta]:

#include "irq_chip_generic.h"
#include "spin_lock.h"

/**
 * IRQ Chip for the Malta platform.
 *
 * Supports traditional I8042 PC interrupts and MIPS cp0 interrupt sources.
 *
 */
class Irq_chip_malta : public Irq_chip_gen
{
public:
  /*
   * irqs  0..15 - I8042 PC interrupts (keyb, uart, mouse, ide, etc)
   * irqs 16..23 - Malta cp0 sw0..sw1 and hw0..hw5 interrupt sources
   */
  enum {
    I8259A_IRQ_BASE   = 0,
    I8259A_NUMIRQS    = 16,
    MIPS_CPU_IRQ_BASE = 16,
    MIPS_CPU_NUMIRQS  = 8, // No support for Config3.IPLW=1 yet
    MIPS_CPU_IRQ_I8259A = 2,
    MIPS_CPU_IRQ_IPI0   = 3,
    MIPS_CPU_IRQ_IPI1   = 4,
    MIPS_CPU_IRQ_IPI2   = 5,
    MIPS_CPU_IRQ_TMR    = 7,
    MIPS_GIC_IRQ_BASE   = (MIPS_CPU_IRQ_BASE + MIPS_CPU_NUMIRQS),
  };

  char const *chip_type() const { return "HW Malta_i8259_cp0"; }
};

INTERFACE [mips32 && pic_gic && malta]:

#include "gic.h"
#include "cm.h"

// ---------------------------------------------------------------------
IMPLEMENTATION [mips32 && malta]:

#include "types.h"
#include "initcalls.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"
#include "pic.h"
#include "mipsregs.h"
#include "cp0_status.h"
#include "processor.h"
#include "io.h"
#include "warn.h"
#include "lock_guard.h"
#include "assert.h"

#define GT_PCI0_IACK_OFS        0xc34
#define GT_PIC_BASE             Config::Sys_Controller_Base_GT
#define __PIC_READ(base, ofs)   (*(volatile Unsigned32 *)((base)+(ofs)))
#define GT_READ(ofs)            __PIC_READ(GT_PIC_BASE, ofs)

#define MSC01_PCI_IACK_OFS      0x0620
#define MSC01_PCI_REG_BASE      Config::Sys_Controller_Base_ROCIT
#define MSC01_PCI_IACK          (MSC01_PCI_REG_BASE + MSC01_PCI_IACK_OFS)
#define MSC_READ(reg, data)     do { data = *(volatile unsigned long *)(reg); } while (0)

EXTENSION class Irq_chip_malta
{
private:
  int (Irq_chip_malta::*mips_pcibios_iack)(void);
  Address pic_port_base;
  Spin_lock<> _mips_irq_lock;
public:
  int set_mode(Mword, Mode) { return 0; }
  bool is_edge_triggered(Mword) const { return false; }
  void set_cpu(Mword, Cpu_number) {}
};

PUBLIC FIASCO_INIT
Irq_chip_malta::Irq_chip_malta()
: Irq_chip_gen(Pic::NR_IRQS), _mips_irq_lock(Spin_lock<>::Unlocked)
{
  /*
   * Set up the correct ACK API based on the system controller detected
   */
  pic_port_base = Config::iodevs_port_base;

  switch (Config::sys_controller) {
    case Config::Sys_Controller_GT:
      printf("%s: Detected system controller GT @ base 0x%08lX\n",
        __PRETTY_FUNCTION__, pic_port_base);
      mips_pcibios_iack =
        &Irq_chip_malta::mips_GT_pcibios_iack;
      break;
    case Config::Sys_Controller_ROCIT:
      printf("%s: Detected system controller ROCIT @ base 0x%08lX\n",
        __PRETTY_FUNCTION__, pic_port_base);
      mips_pcibios_iack =
        &Irq_chip_malta::mips_ROCIT_pcibios_iack;
      break;
    case Config::Sys_Controller_NONE:
      panic("No recognized system controller detected!\n");
      break;
  }

  printf("%s: Initing i8259 Interrupt controllers\n", __PRETTY_FUNCTION__);

  Pic::pic_init(I8259A_IRQ_BASE, I8259A_IRQ_BASE+8, pic_port_base);
}

PUBLIC
void
Irq_chip_malta::mask(Mword irq)
{
  if (irq < Irq_chip_malta::MIPS_CPU_IRQ_BASE)
    Pic::disable_locked(irq);
  else if (irq < Irq_chip_malta::MIPS_GIC_IRQ_BASE)
    Cp0_Status::clear_status_bit(0x1 << (STATUSB_IP0 +
          (irq - Irq_chip_malta::MIPS_CPU_IRQ_BASE)));
  else if (Pic::is_gic_int(irq))
    Pic::gic_disable_locked(irq);
}

PUBLIC
void
Irq_chip_malta::mask_and_ack(Mword irq)
{
  mask(irq);
  ack(irq);
}

PUBLIC
void
Irq_chip_malta::ack(Mword irq)
{
  if (irq < Irq_chip_malta::MIPS_CPU_IRQ_BASE)
    Pic::acknowledge_locked(irq);
  else if (irq < Irq_chip_malta::MIPS_GIC_IRQ_BASE)
    ; // nop
  else if (Pic::is_gic_int(irq))
    Pic::gic_acknowledge_locked(irq);
}

PUBLIC
void
Irq_chip_malta::unmask(Mword irq)
{
  if (irq < Irq_chip_malta::MIPS_CPU_IRQ_BASE)
    Pic::enable_locked(irq);
  else if (irq < Irq_chip_malta::MIPS_GIC_IRQ_BASE)
    Cp0_Status::set_status_bit(0x1 << (STATUSB_IP0 +
          (irq - Irq_chip_malta::MIPS_CPU_IRQ_BASE)));
  else if (Pic::is_gic_int(irq))
    Pic::gic_enable_locked(irq);
}

PRIVATE
int Irq_chip_malta::mips_GT_pcibios_iack(void)
{
  int irq;

  /*
   * Determine highest priority pending interrupt by performing
   * a PCI Interrupt Acknowledge cycle.
   */
  irq = GT_READ(GT_PCI0_IACK_OFS);
  irq &= 0xff;
  return irq;
}

PRIVATE
int Irq_chip_malta::mips_ROCIT_pcibios_iack(void)
{
  int irq;

  /*
   * Determine highest priority pending interrupt by performing
   * a PCI Interrupt Acknowledge cycle.
   */
  MSC_READ(MSC01_PCI_IACK, irq);
  irq &= 0xff;
  return irq;

}

PRIVATE inline
int Irq_chip_malta::get_int(void)
{
  auto guard = lock_guard(_mips_irq_lock);
  int irq;

  irq = (this->*mips_pcibios_iack)();

  /*
   * The only way we can decide if an interrupt is spurious
   * is by checking the 8259 registers.  This needs a spinlock
   * on an SMP system,  so leave it up to the generic code...
   */

  return irq;
}

PUBLIC
void
Irq_chip_malta::irq_handler(Unsigned32 status, Unsigned32 cause, bool is_guestcontext)
{
  Mword p = cause & CAUSEF_IP;
  p &= status;

  assert(!is_guestcontext); // is_guestcontext not used in irq_handler

  if (!is_guestcontext) {
    while (p) {
      Mword cp0_irq = 31 - CAUSEB_IP - __builtin_clz(p);
      if (cp0_irq == MIPS_CPU_IRQ_I8259A) {
        Mword pic_irq = get_int();
        handle_irq<Irq_chip_malta>(I8259A_IRQ_BASE + pic_irq, 0);
      } else if (Pic::gic_present() && Pic::gic_assigned_cp0_irq(cp0_irq)) {
        Pic::gic_handle_irq();
      } else {
        handle_irq<Irq_chip_malta>(MIPS_CPU_IRQ_BASE + cp0_irq, 0);
      }
      p &= ~(0x1 << (STATUSB_IP0 + cp0_irq));
    }
  } 
}

extern "C"
void irq_handler(Unsigned32 status, Unsigned32 cause, bool is_guestcontext)
{ mgr->c.irq_handler(status, cause, is_guestcontext); }


// ------------------------------------------------------------------------
IMPLEMENTATION [malta]:

#include "pic.h"

EXTENSION class Pic
{
private:
  enum
  {
    MASTER_PIC_BASE = 0x20,
    SLAVES_PIC_BASE = 0xa0,
    OFF_ICW	    = 0x00,
    OFF_OCW	    = 0x01,

    _MASTER_ICW	    = MASTER_PIC_BASE + OFF_ICW,
    _MASTER_OCW	    = MASTER_PIC_BASE + OFF_OCW,
    _SLAVES_ICW	    = SLAVES_PIC_BASE + OFF_ICW,
    _SLAVES_OCW	    = SLAVES_PIC_BASE + OFF_OCW,
  };

public:
  static Address MASTER_ICW;
  static Address MASTER_OCW;
  static Address SLAVES_ICW;
  static Address SLAVES_OCW;
};

Address Pic::MASTER_ICW = ~0UL;
Address Pic::MASTER_OCW = ~0UL;
Address Pic::SLAVES_ICW = ~0UL;
Address Pic::SLAVES_OCW = ~0UL;

int Pic::special_fully_nested_mode = 0;

PUBLIC static FIASCO_INIT
void
Pic::init_port_base(Address pic_base)
{
  Pic::MASTER_ICW = pic_base + _MASTER_ICW;
  Pic::MASTER_OCW = pic_base + _MASTER_OCW;
  Pic::SLAVES_ICW = pic_base + _SLAVES_ICW;
  Pic::SLAVES_OCW = pic_base + _SLAVES_OCW;
}

static Static_object<Irq_mgr_single_chip<Irq_chip_malta> > mgr;

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  //
  // construct Irq_mgr and Irq_chip_malta
  //
  Irq_mgr::mgr = mgr.construct();

  Pic::gic_init();

  //
  // initialize interrupts
  //
  Irq_mgr::mgr->reserve(2);		// reserve cascade irq
  Irq_mgr::mgr->reserve(7);		// reserve spurious vect
  Irq_mgr::mgr->reserve(0xf);		// reserve spurious vect

  Pic::enable_locked(2);		// allow cascaded irqs
}

PRIVATE
bool
Irq_chip_malta::is_local_irq(Mword pin)
{
  return ((pin >= Irq_chip_malta::MIPS_CPU_IRQ_BASE) &&
      (pin < Irq_chip_malta::MIPS_GIC_IRQ_BASE));
}

PUBLIC
bool
Irq_chip_malta::alloc(Irq_base *irq, Mword pin)
{
  // allow local irqs to be allocated on each CPU
  return (is_local_irq(pin) && irq->chip() == this && irq->pin() == pin) || Irq_chip_gen::alloc(irq, pin);
}

//
// based on pic-i8259.cpp non special_fully_nested_mode
//
PUBLIC static FIASCO_INIT
void
Pic::pic_init(Address master_base, Address slave_base, Address pic_base)
{
  // first initialize pic port memory mapped IO base
  Pic::init_port_base(pic_base);

  // disallow all interrupts before we selectively enable them 
  Pic::disable_all_save();

  /* Initialize the master. */
  Io::out8_p(PICM_ICW1, MASTER_ICW);
  Io::out8_p(master_base, MASTER_OCW);
  Io::out8_p(PICM_ICW3, MASTER_OCW);
  Io::out8_p(PICM_ICW4, MASTER_OCW);

  /* Initialize the slave. */
  Io::out8_p(PICS_ICW1, SLAVES_ICW);
  Io::out8_p(slave_base, SLAVES_OCW);
  Io::out8_p(PICS_ICW3, SLAVES_OCW);
  Io::out8_p(PICS_ICW4, SLAVES_OCW);

  // set initial masks
  Io::out8_p(0xfb, MASTER_OCW);	// unmask irq2
  Io::out8_p(0xff, SLAVES_OCW);	// mask everything

  /* Ack any bogus intrs by setting the End Of Interrupt bit. */
  Io::out8_p(NON_SPEC_EOI, MASTER_ICW);
  Io::out8_p(NON_SPEC_EOI, SLAVES_ICW);

  // disallow all interrupts before we selectively enable them 
  Pic::disable_all_save();

}

// ------------------------------------------------------------------------
IMPLEMENTATION [mips32 && pic_gic && malta && mp]:

PUBLIC static
void Pic::init_ap(Cpu_number cpu)
{
  gic->init_ap(cpu);
}

// ------------------------------------------------------------------------
IMPLEMENTATION [mips32 && pic_gic && malta]:

EXTENSION class Pic
{
public:
  enum {
    GIC_BASE_ADDR = 0x1bdc0000,
    GIC_ADDRSPACE_SZ = (128 * 1024),
  };
};

PUBLIC static
unsigned Pic::gic_irq_base()
{
  return Irq_chip_malta::MIPS_GIC_IRQ_BASE;
}

PUBLIC static FIASCO_INIT
void Pic::gic_init()
{
  if (!Cm::mips_cm_present())
    return;

  write_gcr_gic_base(GIC_BASE_ADDR | CM_GCR_GIC_BASE_GICEN_MSK);
  _gic_present = true;

  printf("GIC present\n");

  gic.construct((Address)GIC_BASE_ADDR,
                (Address)GIC_ADDRSPACE_SZ,
                (unsigned int)Irq_chip_malta::MIPS_GIC_IRQ_BASE);

  /* verify that all IPI have been enabled */
  assert(Cp0_Status::ST_PLATFORM_IPI_INTS ==
      ((1UL << (Irq_chip_malta::MIPS_CPU_IRQ_IPI2 + STATUSB_IP0)) |
       (1UL << (Irq_chip_malta::MIPS_CPU_IRQ_IPI1 + STATUSB_IP0)) |
       (1UL << (Irq_chip_malta::MIPS_CPU_IRQ_IPI0 + STATUSB_IP0))));
}

// ------------------------------------------------------------------------
IMPLEMENTATION [mips32 && !pic_gic && malta]:

PUBLIC static FIASCO_INIT
void Pic::gic_init()
{
}
