/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

EXTENSION class Proc
{
public:
  static inline Status interrupts(Status status);
};

#include "warn.h"
#include "types.h"
#include "cp0_status.h"

IMPLEMENTATION [mips32]:

EXTENSION class Proc
{
public:
  //disable power savings mode
 static Mword wake(Mword);
 static Cpu_phys_id cpu_id();
};

/// Unblock external inetrrupts
IMPLEMENT static inline ALWAYS_INLINE
void Proc::sti()
{
  asm volatile (
    "ei   \n"
    "ehb  \n"
    : /* no outputs */
    : /* no inputs */
    : "memory"
  );

}

/// Block external interrupts
IMPLEMENT static inline
void Proc::cli()
{
   asm volatile (
    "di   \n"
    "ehb  \n"
    : /* no outputs */
    : /* no inputs */
    : "memory"
  );
}

/// Are external interrupts enabled ?
IMPLEMENT static inline
Proc::Status Proc::interrupts()
{
  return (Status)Cp0_Status::read_status() & Cp0_Status::ST_IE;
}

/// Are interrupts enabled in saved status state?
IMPLEMENT static inline
Proc::Status Proc::interrupts(Status status)
{
  return status & Cp0_Status::ST_IE;
}

/// Block external interrupts and save the old state
IMPLEMENT static inline ALWAYS_INLINE
Proc::Status Proc::cli_save()
{
  Status flags;

   asm volatile (
    "di   %[flags]\n"
    "ehb  \n"
    : [flags] "=r" (flags)
    : /* no inputs */
    : "memory"
  );
  return flags & Cp0_Status::ST_IE;
}

/// Conditionally unblock external interrupts
IMPLEMENT static inline ALWAYS_INLINE
void Proc::sti_restore(Status status)
{
  if (status & Cp0_Status::ST_IE)
    Proc::sti();
}

IMPLEMENT static inline
void Proc::pause()
{
}

IMPLEMENT static inline
void Proc::halt()
{
  //enable interrupts and wait
  Cp0_Status::set_status_bit(Cp0_Status::ST_IE);
  asm volatile ("wait");
}

IMPLEMENT static inline
Mword Proc::wake(Mword /* srr1 */)
{
  NOT_IMPL_PANIC;
}

IMPLEMENT static inline
void Proc::irq_chance()
{
  asm volatile ("nop; nop;" : : :  "memory");
}

IMPLEMENT static inline
void Proc::stack_pointer(Mword sp)
{
  asm volatile ( " move $29,%0 \n"
		 : : "r" (sp)
		 );
}

IMPLEMENT static inline
Mword Proc::stack_pointer()
{
  Mword sp;
  asm volatile ( " move %0,$29  \n"
		 : "=r" (sp)
		 );
  return sp;
}

IMPLEMENT static inline
Mword Proc::program_counter()
{
  Mword pc; 
  asm volatile (
      " move  $t0, $ra   \n"
		  " jal   1f        \n"
		  "  1:             \n"
		  " move  %0, $ra   \n"
		  " move  $ra, $t0  \n"
		 : "=r" (pc) : : "t0");

  return pc;
}

IMPLEMENTATION [mips32 && !mp]:

IMPLEMENT static inline
Cpu_phys_id Proc::cpu_id()
{ return Cpu_phys_id(0); }

IMPLEMENTATION [mips32 && mp]:

#include "mipsregs.h"

IMPLEMENT static inline
Cpu_phys_id Proc::cpu_id()
{ return Cpu_phys_id(get_ebase_cpunum()); }
