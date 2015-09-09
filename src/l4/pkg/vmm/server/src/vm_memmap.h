#pragma once

#include <l4/sys/l4int.h>
#include <map>

#include "mmio_device.h"

struct Region
{
  l4_addr_t start;
  l4_addr_t end;

  Region() {}
  Region(l4_addr_t a) : start(a), end(a) {}
  Region(l4_addr_t s, l4_addr_t e) : start(s), end(e) {}

  static Region ss(l4_addr_t start, l4_size_t size)
  { return Region(start, start + size - 1); }

  bool operator < (Region const &r) const { return end < r.start; }
};

typedef std::map<Region, Vmm::Mmio_device *> Vm_mem;
