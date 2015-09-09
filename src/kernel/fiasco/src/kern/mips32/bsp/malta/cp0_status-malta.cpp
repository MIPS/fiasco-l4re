/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32 && malta && !pic_gic]:

#include "cp0_status.h"
#include "mipsregs.h"

EXTENSION class Cp0_Status
{
public:
  enum Status_mask_platform {
    ST_PLATFORM_IPI_INTS = 0,
    ST_PLATFORM_ENABLE_INTS = STATUSF_IP7 | STATUSF_IP2,
  };
};

INTERFACE [mips32 && malta && pic_gic]:

#include "cp0_status.h"
#include "mipsregs.h"

EXTENSION class Cp0_Status
{
public:
  enum Status_mask_platform {
    ST_PLATFORM_IPI_INTS = STATUSF_IP5 | STATUSF_IP4 | STATUSF_IP3,
    ST_PLATFORM_ENABLE_INTS = STATUSF_IP7 | STATUSF_IP2 | ST_PLATFORM_IPI_INTS,
  };
};
