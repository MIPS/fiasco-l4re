/*
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <l4/util/port_io.h>
#include "debug.h"
#include "hw_device.h"
#include "pci.h"
#include "main.h"
#include "pci_memport_root_bridge.h"

void
Pci_memport_root_bridge::init()
{
  d_printf(DBG_DEBUG, "Pci_memport_root_bridge init\n");

  if ((_pcictrl_phys == ~0UL) || (_pcictrl_size == ~0UL))
    {
      d_printf(DBG_ERR, "ERROR: Hw::Pci::Root_bridge: 'pcictrl' or 'pcictrl_size' not set.\n");
      return;
    }

  _pcictrl_virt = res_map_iomem(_pcictrl_phys, _pcictrl_size);
  if (!_pcictrl_virt) {
    d_printf(DBG_ERR, "ERROR mapping system controller pcictrl memory: 0x%lx\n", _pcictrl_phys);
    return;
  }

  add_resource_rq(new Resource(Resource::Mmio_res,
                            _pcictrl_phys,
                            _pcictrl_phys + _pcictrl_size - 1));

  discover_bus(this);

  Hw::Device::init();
}

void
Pci_memport_root_bridge::discover_bus(Hw::Device *host)
{
  if (!_pcictrl_virt)
    return;
  Hw::Pci::Root_bridge::discover_bus(host);
}

int
Pci_memport_root_bridge::cfg_read(Cfg_addr addr, l4_uint32_t *value, Cfg_width w)
{
  using namespace Hw;

  if (!_pcictrl_virt)
    return -1;

  l4_uint32_t data;

  l4util_out32((addr.to_compat_addr() | 0x80000000) & ~3UL, _pcictrl_virt + 0xcf8);
  data = l4util_in32(_pcictrl_virt + 0xcfc);

  if (w == Pci::Cfg_byte)
    *value = (data >> ((addr.reg() & 3) << 3)) & 0xff;
  else if (w == Pci::Cfg_short)
    *value = (data >> ((addr.reg() & 3) << 3)) & 0xffff;
  else if (w == Pci::Cfg_long)
    *value = data;

  d_printf(DBG_ALL, "Pci_memport_root_bridge::cfg_read(%x, %x, %x, %x, %x, %d)\n",
           addr.bus(), addr.dev(), addr.fn(), addr.reg(), *value, w);

  return 0;
}

int
Pci_memport_root_bridge::cfg_write(Cfg_addr addr, l4_uint32_t value, Cfg_width w)
{
  using namespace Hw;

  if (!_pcictrl_virt)
    return -1;

  l4_uint32_t data = 0;

  d_printf(DBG_ALL, "Pci_memport_root_bridge::cfg_write(%x, %x, %x, %x, %x, %d)\n",
           addr.bus(), addr.dev(), addr.fn(), addr.reg(), value, w);

  l4util_out32((addr.to_compat_addr() | 0x80000000) & ~3UL, _pcictrl_virt + 0xcf8);

  if (w == Pci::Cfg_byte)
    data = (data & ~(0xff << ((addr.reg() & 3) << 3))) |
            (value << ((addr.reg() & 3) << 3));
  else if (w == Pci::Cfg_short)
    data = (data & ~(0xffff << ((addr.reg() & 3) << 3))) |
            (value << ((addr.reg() & 3) << 3));
  else if (w == Pci::Cfg_long)
    data = value;

  l4util_out32(data, _pcictrl_virt + 0xcfc);

  return 0;
}

template< typename T>
int
Pci_memport_root_bridge::getval(const char *s, cxx::String const &prop,
                              Prop_val const &val, T *rval)
{
  if (prop == s)
    {
      if (val.type != Prop_val::Int)
        return -E_inval;

      *rval = val.val.integer;
      return E_ok;
    }
  return -E_no_prop;
}

int
Pci_memport_root_bridge::set_property(cxx::String const &prop, Prop_val const &val)
{
  int r;

  if ((r = getval("pcictrl", prop, val, &_pcictrl_phys)) != -E_no_prop)
    return r;
  else if ((r = getval("pcictrl_size", prop, val, &_pcictrl_size)) != -E_no_prop)
    return r;

  return Hw::Device::set_property(prop, val);
}

static Hw::Device_factory_t<Pci_memport_root_bridge>
              __hw_pci_root_bridge_factory("Pci_memport_root_bridge");
