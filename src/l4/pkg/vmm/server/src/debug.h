#pragma once

#include <l4/re/util/debug>

struct Err : L4Re::Util::Err
{
  Err(Level l = Fatal) : L4Re::Util::Err(l, "VMM") {}
};

struct Dbg : L4Re::Util::Dbg
{
  enum
  {
    Info = 1,
    Warn = 2,

    Mmio   = 0x10000,
    Gicd   = 0x20000,
    Vm_bus = 0x40000,
  };

  Dbg(unsigned long lvl = Info, char const *subsys = "")
  : L4Re::Util::Dbg(lvl, "VMM", subsys)
  {}
};
