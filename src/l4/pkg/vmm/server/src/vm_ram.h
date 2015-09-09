#pragma once

#include <l4/sys/l4int.h>

#include "virtio.h"

namespace Vmm {

class Vm_ram
{
protected:
  Vm_ram() : _vm_start(~0UL), _cont(false), _ident(false) {}

public:
  template<typename T>
  T *access(Virtio::Ptr<T> p) const { return (T*)(p.get() + _offset); }

  l4_addr_t vm_start() const { return _vm_start; }
  l4_size_t size() const { return _size; }
  l4_addr_t local_start() const { return _local_start; }

protected:
  l4_mword_t _offset;
  l4_addr_t _local_start;
  l4_addr_t _local_end;

  l4_addr_t _vm_start;
  l4_size_t _size;

  bool _cont;
  bool _ident;
};

}
