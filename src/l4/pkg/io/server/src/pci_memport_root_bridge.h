/*
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014  Imagination Technologies, Inc.  All rights reserved.
 * Authors: Yann Le Du <ledu@kymasys.com>
 *
 */
#pragma once

#include "hw_device.h"
#include "pci.h"
#include "main.h"

class Pci_memport_root_bridge : public Hw::Pci::Root_bridge, public Hw::Device
{
public:
  typedef Hw::Pci::Cfg_width Cfg_width;
  typedef Hw::Pci::Cfg_addr Cfg_addr;

  explicit Pci_memport_root_bridge(unsigned bus_nr = 0)
  : Hw::Pci::Root_bridge(bus_nr, this),
  _pcictrl_virt(0), _pcictrl_phys(~0UL), _pcictrl_size(~0UL)
  {}

  int cfg_read(Cfg_addr addr, l4_uint32_t *value, Cfg_width);
  int cfg_write(Cfg_addr addr, l4_uint32_t value, Cfg_width);

  void discover_bus(Hw::Device *host);

  void init();

  int set_property(cxx::String const &prop, Prop_val const &val);
  template< typename T>
  int getval(const char *s, cxx::String const &prop,
             Prop_val const &val, T *rval);

private:
  l4_addr_t _pcictrl_virt, _pcictrl_phys, _pcictrl_size;
};
