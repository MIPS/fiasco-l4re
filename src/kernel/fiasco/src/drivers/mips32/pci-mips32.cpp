/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32 && malta]:

#include "io.h"
#include "config.h"
#include <cassert>

PUBLIC static inline NEEDS["io.h", <cassert>, "config.h"]
Unsigned32
Pci::read_cfg(Cfg_addr addr, Cfg_width w)
{
  Io::out32((addr.compat_addr() | (1<<31)) & ~3UL, Config::pci_port_base + Cfg_addr_port);
  Unsigned32 data = Io::in32(Config::pci_port_base + Cfg_data_port);

  switch (w)
    {
    case Cfg_width::Byte:  return (data >> ((addr.reg() & 3) << 3)) & 0xff;
    case Cfg_width::Short: return (data >> ((addr.reg() & 3) << 3)) & 0xffff;
    case Cfg_width::Long:  return data;
    default: assert (false /* invalid PCI config access */);
    }
  return 0;
}

PUBLIC static template< typename VT > inline
void
Pci::read_cfg(Cfg_addr addr, VT *value)
{
  static_assert (sizeof (VT) == 1 || sizeof (VT) == 2 || sizeof (VT)==4,
                 "wrong PCI config space access size");
  Cfg_width w;
  switch (sizeof(VT))
    {
    case 1: w = Cfg_width::Byte; break;
    case 2: w = Cfg_width::Short; break;
    case 4: w = Cfg_width::Long; break;
    }

  *value = read_cfg(addr, w);
}


PUBLIC static template< typename VT > inline NEEDS["io.h", "config.h"]
void
Pci::write_cfg(Cfg_addr addr, VT value)
{
  Unsigned32 data;

  static_assert (sizeof (VT) == 1 || sizeof (VT) == 2 || sizeof (VT)==4,
                 "wrong PCI config space access size");
  Io::out32((addr.compat_addr() | (1<<31)) & ~3UL, Config::pci_port_base + Cfg_addr_port);
  switch (sizeof(VT))
    {
    case 1:
      data = (data & ~(0xff << ((addr.reg() & 3) << 3))) |
              (value << ((addr.reg() & 3) << 3));
      break;
    case 2:
      data = (data & ~(0xffff << ((addr.reg() & 3) << 3))) |
              (value << ((addr.reg() & 3) << 3));
      break;
    case 4:
      data = value;
      break;
    }
  Io::out32(data, Config::pci_port_base + Cfg_data_port + addr.reg_offs(Cfg_width::Long));
}

