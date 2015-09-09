/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32]:

#include <cstring>
#include "mipsregs.h"
#include "panic.h"
#include "warn.h"

PRIVATE
static void
Utcb_init::validate_utcb()
{
  const Mword dummy = 0x12345678;
  Mword old_value = 0;
  Mword new_value = 0;

  // TODO may need to expand validate_utcb() to catch an exception but this
  // approach is sufficient for platforms which silently ignore the accesses.
  // Write the test such that the exception handler can skip the faulting access
  // and still detect a failure, or consider using _recover_jmpbuf.

  if ((int)Config::Warn_level >= Warning)
    printf("Verifying access to UTCB pointer... ");

  // Detect issues with accessing the UTCB pointer early on.
  old_value = read_c0_ddatalo();
  write_c0_ddatalo(dummy);
  back_to_back_c0_hazard();
  new_value = read_c0_ddatalo();
  write_c0_ddatalo(old_value);

  if (dummy != new_value)
    panic("UTCB pointer could not be stored in DDataLo register.\n"
          "Consider using another COP0 register available to your platform.\n");
  else if ((int)Config::Warn_level >= Warning)
    printf("UTCB pointer stored in DDataLo\n");
}

// XXXKYMA: Store the UTCB pointer in COP0-DDataLo Register.
// This can be set in kernel mode and read in user mode.
IMPLEMENT 
void
Utcb_init::init()
{
  validate_utcb();
}
