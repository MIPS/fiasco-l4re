/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32 && sead3]:

#include "types.h"
#include "assert.h"
#include "initcalls.h"
#include "types.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"
#include "kmem.h"
#include "mipsregs.h"
#include "cp0_status.h"

#include "io.h"
#include "processor.h"

class Irq_chip_sead3 : public Irq_chip_gen
{
private:
  int (Irq_chip_sead3::*mips_pcibios_iack)(void);
  Unsigned32 pic_port_base;
public:
  int set_mode(Mword, Mode) { return 0; }
  bool is_edge_triggered(Mword) const { return false; }
  void set_cpu(Mword, Cpu_number) {}
  void ack(Mword) { /* ack is empty */ }
};

//TBD
PUBLIC
Irq_chip_sead3::Irq_chip_sead3() : Irq_chip_gen(10)
{

#if defined(CONFIG_JDB) && defined (CONFIG_PF_SEAD3)
#error JDB not supported due to lack of interrupt support
#endif

  /*
   * Set up the correct ACK API based on the system controller detected
   */
  pic_port_base = Config::iodevs_port_base;
  switch (Config::sys_controller) {
    case Config::Sys_Controller_GT:
      printf("%s: Detected system controller GT @ base 0x%08X\n",
        __PRETTY_FUNCTION__, pic_port_base);
      mips_pcibios_iack = NULL;
      break;
    case Config::Sys_Controller_ROCIT:
      printf("%s: Detected system controller ROCIT @ base 0x%08X\n",
        __PRETTY_FUNCTION__, pic_port_base);
      mips_pcibios_iack = NULL;
      break;
    case Config::Sys_Controller_NONE:
      printf("No recognized system controller detected!\n");
      break;
  }
}

PUBLIC
void
Irq_chip_sead3::mask(Mword irq)
{
  //assert(cpu_lock.test());
  assert (irq < 8);

  if (irq == 2) {
  }
  else
    Cp0_Status::clear_status_bit(0x1 << (8+irq));
}

PUBLIC
void
Irq_chip_sead3::mask_and_ack(Mword irq)
{
  if (irq == 2) {
    mask(irq);
  }
}

PUBLIC
void
Irq_chip_sead3::unmask(Mword irq)
{
  assert (irq < 8);
  //assert(cpu_lock.test());

  if (irq == 2) {
  }
  else
    Cp0_Status::set_status_bit(0x1 << (8+irq));
}

static Static_object<Irq_mgr_single_chip<Irq_chip_sead3> > mgr;

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  Irq_mgr::mgr = mgr.construct();
}

IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{
  Status s = 0;
  return s;
}

IMPLEMENT inline
void Pic::restore_all(Status)
{}

inline
int Irq_chip_sead3::get_int(void)
{
	int irq;
#if 0
  irq = (this->*mips_pcibios_iack)();
#endif
	return irq;
}

PUBLIC
void
Irq_chip_sead3::irq_handler(Unsigned32 status, Unsigned32 cause, bool is_guestcontext)
{
  Mword p = cause & CAUSEF_IP;
  p &= status;

  assert(!is_guestcontext); // is_guestcontext not used in irq_handler

  if (!is_guestcontext) {
    while (p) {
      Mword irq = 23 - __builtin_clz(p);
      if (irq == 2) {
      }
      handle_irq<Irq_chip_sead3>(irq, 0);
      p &= ~(0x1 << (8+irq));
      cause &= ~(0x1 << (8+irq));
    }
  } 
}

extern "C"
void irq_handler(Unsigned32 status, Unsigned32 cause, bool is_guestcontext)
{ mgr->c.irq_handler(status, cause, is_guestcontext); }

//---------------------------------------------------------------------------
IMPLEMENTATION [debug && sead3]:

PUBLIC
char const *
Irq_chip_sead3::chip_type() const
{ return "sead3"; }

