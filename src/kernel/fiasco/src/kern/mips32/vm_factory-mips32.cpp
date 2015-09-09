/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32 && mips_vz]:

#include "ram_quota.h"
#include "vm.h"
#include "warn.h"

IMPLEMENT
Vm *
Vm_factory::create(Ram_quota *quota, int *err)
{
  if (!cpu_has_vz) {
    WARN("MIPS VZ not supported\n");
    *err = L4_err::ENodev;
    return 0;
  }

  if (void *t = Vm::allocator()->q_alloc(quota))
    {
      Vm *a = new (t) Vm(quota);
      if (a->initialize())
        return a;

      delete a;
    }

  *err = L4_err::ENomem;
  return 0;
}
