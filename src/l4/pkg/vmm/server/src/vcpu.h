#pragma once

#include <l4/sys/vcpu.h>

namespace Vmm {

struct Cpu : l4_vcpu_state_t
{
  unsigned get_vcpu_id() const
  { return *(reinterpret_cast<unsigned char const*>(this) + 0x208); }

  void set_vcpu_id(unsigned id)
  { *(reinterpret_cast<unsigned char *>(this) + 0x208) = id; }

  static Cpu *from_vcpu(l4_vcpu_state_t *vcpu)
  { return static_cast<Cpu*>(vcpu); }

  static Cpu const *from_vcpu(l4_vcpu_state_t const *vcpu)
  { return static_cast<Cpu const *>(vcpu); }
};

}
