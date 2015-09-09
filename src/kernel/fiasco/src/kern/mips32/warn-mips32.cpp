/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:
#include "panic.h"

#define NOT_IMPL WARN("%s not implemented", __PRETTY_FUNCTION__)
#define NOT_IMPL_PANIC panic("%s not implemented\n", __PRETTY_FUNCTION__)
