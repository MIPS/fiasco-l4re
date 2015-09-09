/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32 && sead3]:

#include "cp0_status.h"
#include "mipsregs.h"

EXTENSION class Cp0_Status
{
public:
  enum Status_mask_platform {
    ST_PLATFORM_ENABLE_INTS = STATUSF_IP7,
  };
};
