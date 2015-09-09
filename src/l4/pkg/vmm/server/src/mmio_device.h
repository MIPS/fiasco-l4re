#pragma once

#include <l4/sys/task>

#include <l4/sys/vcpu.h>
#include <l4/sys/l4int.h>
#include <l4/sys/types.h>

namespace Vmm {

struct Mmio_device
{
  virtual bool access(l4_addr_t pfa, l4_addr_t offset, l4_vcpu_state_t *vcpu,
                      L4::Cap<L4::Task> vm_task, l4_addr_t s, l4_addr_t e) = 0;
};

}
