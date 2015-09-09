/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE:

#include "types.h"
#include "mipsregs.h"

class Cp0_Status
{
public:
  enum Status_bits {
    ST_IE  = 1UL << 0, 
    ST_EXL = 1UL << 1,
    ST_ERL = 1UL << 2, 
    ST_KSU_MASK = 3UL << 3,
    ST_KSU_SUPERVISOR = 1UL << 3,
    ST_KSU_USER = 1UL << 4,
    ST_UX = 1UL << 5, 
    ST_SX = 1UL << 6,
    ST_KX = 1UL << 7,
    ST_IP = 0xFFUL << 8,
    ST_DE = 1UL << 16,
    ST_CE = 1UL << 17,
    ST_FR = 1UL << 26,
    ST_CU0 = 1UL << 28,
    ST_CU1 = 1UL << 29,
  };

private:
  static void check_status_hazards(Mword bits);
};

//------------------------------------------------------------------------------
IMPLEMENTATION:

#include "mipsregs.h"

IMPLEMENT inline NEEDS["mipsregs.h"]
void
Cp0_Status::check_status_hazards(Mword bits)
{
  //for these context synchronization is mandatory
  static Mword mask = (ST_IE | ST_CU1);

  if(bits & mask)
    irq_enable_hazard();
}

PUBLIC static inline
void
Cp0_Status::set_status_bit(Mword bits)
{
  Mword reg = read_c0_status();
  reg |= bits;
  write_c0_status(reg);
  check_status_hazards(bits);
}

PUBLIC static inline
void
Cp0_Status::clear_status_bit(Mword bits)
{
  Mword reg = read_c0_status();
  reg &= ~bits;
  write_c0_status(reg);
  check_status_hazards(bits);
}

PUBLIC static inline
Mword
Cp0_Status::read_status()
{
  Mword ret = read_c0_status();
  return ret;
}

/*
 * set kernel mode and set EXL to prepare for eret to kernel mode, disable
 * interrupts but leave interrupt masks set.
 */
PUBLIC static inline
Mword
Cp0_Status::status_eret_to_kern_di(Mword status_word)
{
  status_word &= ~(ST_KSU_MASK | ST_IP | ST_IE);
  status_word |= ST_EXL | ST_PLATFORM_ENABLE_INTS;
  return status_word;
}

/*
 * set user mode, enable interrupts and interrupt masks, and set EXL to prepare
 * for eret to user mode
 */
PUBLIC static inline
Mword
Cp0_Status::status_eret_to_user_ei(Mword status_word)
{
  status_word &= ~(ST_KSU_MASK | ST_IP);
  status_word |= ST_KSU_USER | ST_EXL | ST_IE | ST_PLATFORM_ENABLE_INTS;
  return status_word;
}

PUBLIC static inline
Mword
Cp0_Status::status_eret_to_guest_ei(Mword status_word)
{
  status_word &= ~(ST_KSU_MASK | ST_IP);
  status_word |= ST_KSU_USER | ST_EXL | ST_IE | ST_PLATFORM_ENABLE_INTS;
  return status_word;
}
