/*
 * (c) 2010 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <l4/vbus/vdevice-ops.h>

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <l4/cxx/exceptions>
#include <l4/io/pciids.h>
#include <l4/sys/err.h>

#include "debug.h"
#include "pci.h"
#include "vpci.h"
#include "virt/vbus_factory.h"

namespace Vi {

// -----------------------------------------------------------------------
// Pci_virtual_dev
// -----------------------------------------------------------------------

Pci_virtual_dev::Pci_virtual_dev()
{
  memset(&_h, 0, sizeof(_h));
}

int
Pci_virtual_dev::cfg_read(int reg, l4_uint32_t *v, Cfg_width order)
{
  reg >>= order;
  if ((unsigned)reg >= (_h_len >> order))
    return -L4_ERANGE;

#define RC(x) *v = *((Hw::Pci::Cfg_type<x>::Type const *)_h + reg); break
  *v = 0;
  switch (order)
    {
    case Hw::Pci::Cfg_byte: RC(Hw::Pci::Cfg_byte);
    case Hw::Pci::Cfg_short: RC(Hw::Pci::Cfg_short);
    case Hw::Pci::Cfg_long: RC(Hw::Pci::Cfg_long);
    }

  return 0;
#undef RC
}

int
Pci_virtual_dev::cfg_write(int reg, l4_uint32_t v, Cfg_width order)
{
  switch (reg & ~3)
    {
    case 0x4: // status is RO
      v &= 0x0000ffff << (reg & (3 >> order));
      break;

    default:
      break;
    }

  reg >>= order;
  if ((unsigned)reg >= (_h_len >> order))
    return -L4_ERANGE;

#define RC(x) *((Hw::Pci::Cfg_type<x>::Type *)_h + reg) = v; break
  switch (order)
    {
    case Hw::Pci::Cfg_byte: RC(Hw::Pci::Cfg_byte);
    case Hw::Pci::Cfg_short: RC(Hw::Pci::Cfg_short);
    case Hw::Pci::Cfg_long: RC(Hw::Pci::Cfg_long);
    }

  return 0;
#undef RC
}



// -----------------------------------------------------------------------
// Pci_proxy_dev
// -----------------------------------------------------------------------

Pci_proxy_dev::Pci_proxy_dev(Hw::Pci::If *hwf)
: _hwf(hwf), _rom(0)
{
  assert (hwf);
  for (int i = 0; i < 6; ++i)
    {
      Resource *r = _hwf->bar(i);

      if (!r)
	{
	  _vbars[i] = 0;
	  continue;
	}

      if (_hwf->is_64bit_high_bar(i))
	{
	  _vbars[i] = l4_uint64_t(r->start()) >> 32;
	}
      else
	{
	  _vbars[i] = r->start();
	  if (r->type() == Resource::Io_res)
	    _vbars[i] |= 1;

	  if (r->is_64bit())
	    _vbars[i] |= 4;

	  if (r->prefetchable())
	    _vbars[i] |= 8;

	}

      //printf("  bar: %d = %08x\n", i, _vbars[i]);
    }

  if (_hwf->rom())
    _rom = _hwf->rom()->start();
}

int
Pci_proxy_dev::irq_enable(Irq_info *irq)
{
  for (Resource_list::const_iterator i = host()->resources()->begin();
       i != host()->resources()->end(); ++i)
    {
      Resource *res = *i;

      if (!res->disabled() && res->type() == Resource::Irq_res)
	{
	  irq->irq = res->start();
          irq->trigger = !res->irq_is_level_triggered();
	  irq->polarity = res->irq_is_low_polarity();
	  d_printf(DBG_DEBUG, "Enable IRQ: %d %x %x\n", irq->irq, irq->trigger, irq->polarity);
	  if (dlevel(DBG_DEBUG2))
	    dump();
	  return 0;
	}
    }
  return -L4_EINVAL;
}



l4_uint32_t
Pci_proxy_dev::read_bar(int bar)
{
  // d_printf(DBG_ALL, "   read bar[%x]: %08x\n", bar, _vbars[bar]);
  return _vbars[bar];
}

void
Pci_proxy_dev::write_bar(int bar, l4_uint32_t v)
{
  Hw::Pci::If *p = _hwf;

  Resource *r = p->bar(bar);
  if (!r)
    return;

  // printf("  write bar[%x]: %llx-%llx...\n", bar, r->abs_start(), r->abs_end());
  l4_uint64_t size_mask = r->alignment();

  if (r->type() == Resource::Io_res)
    size_mask |= 0xffff0000;

  if (p->is_64bit_high_bar(bar))
    size_mask >>= 32;

  _vbars[bar] = (_vbars[bar] & size_mask) | (v & ~size_mask);

  // printf("    bar=%lx\n", _vbars[bar]);
}

void
Pci_proxy_dev::write_rom(l4_uint32_t v)
{
  Hw::Pci::If *p = _hwf;

  // printf("write rom bar %x %p\n", v, _dev->rom());
  Resource *r = p->rom();
  if (!r)
    return;

  l4_uint64_t size_mask = r->alignment();

  _rom = (_rom & size_mask) | (v & (~size_mask | 1));

  p->cfg_write(0x30, (r->start() & ~1U) | (v & 1), Hw::Pci::Cfg_long);
}

int
Pci_proxy_dev::cfg_read(int reg, l4_uint32_t *v, Cfg_width order)
{
  Hw::Pci::If *p = _hwf;

  l4_uint32_t buf;

  l4_uint32_t const *r = &buf;
  reg &= ~0U << order;
  int dw_reg = reg & ~3;
  switch (dw_reg)
    {
    case 0x00: buf = p->vendor_device_ids(); break;
    case 0x08: buf = p->class_rev(); break;
    case 0x04: p->cfg_read(dw_reg, &buf); break;
    /* simulate multi function on hdr type */
    case 0x0c: p->cfg_read(dw_reg, &buf); buf |= 0x00800000; break;
    case 0x10: /* bars 0 to 5 */
    case 0x14:
    case 0x18:
    case 0x1c:
    case 0x20:
    case 0x24: buf = read_bar((dw_reg - 0x10) / 4); break;
    case 0x2c: buf = p->subsys_vendor_ids(); break;
    case 0x30: buf = read_rom(); break;
    case 0x34: /* CAPS */
    case 0x38: buf = 0; break; /* PCI 3.0 reserved */
    /* latency and IRQs */
    case 0x3c: p->cfg_read(dw_reg, &buf); break;
    /* pass trough the rest ... */
    default:   p->cfg_read(dw_reg, &buf); break;
    }

  unsigned mask = ~0U >> (32 - (8U << order));
  *v = (*r >> ((reg & 3) *8)) & mask;
  return L4_EOK;
}


void
Pci_proxy_dev::_do_cmd_write(unsigned mask,unsigned value)
{
  l4_uint16_t cmd = _hwf->cfg_read<l4_uint16_t>(Pci::Config::Command);
  l4_uint16_t ncmd = cmd & ~mask; // get unmodified bits
  ncmd |= value & mask; // set new bits

  if (ncmd == cmd) // nothing modified so ignore the write
    return;

  unsigned enable_decoders = _hwf->recheck_bars(ncmd & ~cmd & 3);
  if ((ncmd & ~cmd & 3) && !enable_decoders)
    {
      d_printf(DBG_WARN, "warning: could not set bars, disable decoders\n");
      ncmd &= (~3U | enable_decoders);
    }

  _hwf->cfg_write(Pci::Config::Command, ncmd);
}

int
Pci_proxy_dev::_do_status_cmd_write(l4_uint32_t mask, l4_uint32_t value)
{
  if (mask & 0xffff)
    _do_cmd_write(mask & 0xffff, value & 0xffff);

  // status register has 'write 1 to clear' semantics so we can always write
  // 16bit with the bits masked that we do not want to affect.
  if (mask & value & 0xffff0000)
    _hwf->cfg_write(Pci::Config::Status, (value & mask) >> 16, Hw::Pci::Cfg_short);

  return 0;
}

int
Pci_proxy_dev::_do_rom_bar_write(l4_uint32_t mask, l4_uint32_t value)
{
  l4_uint32_t b = read_rom();

  if ((value & mask & 1) && !(b & mask & 1) && !_hwf->enable_rom())
    return 0;  // failed to enable

  b &= ~mask;
  b |= value & mask;
  write_rom(b);
  return 0;
}


int
Pci_proxy_dev::cfg_write(int reg, l4_uint32_t v, Cfg_width order)
{
  Hw::Pci::If *p = _hwf;

  reg &= ~0U << order;
  l4_uint32_t const offset_32 = reg & 3;
  l4_uint32_t const mask_32 = (~0U >> (32 - (8U << order))) << (offset_32 * 8);
  l4_uint32_t const value_32 = v << (offset_32 * 8);


  switch (reg & ~3)
    {
    case 0x00: return 0;
    case 0x08: return 0;
    case 0x04: return _do_status_cmd_write(mask_32, value_32);
    /* simulate multi function on hdr type */
    case 0x10: /* bars 0 to 5 */
    case 0x14:
    case 0x18:
    case 0x1c:
    case 0x20:
    case 0x24:
      {
        l4_uint32_t b = read_bar(reg / 4 - 4);
        b &= ~mask_32;
        b |= value_32 & mask_32;
        write_bar(reg / 4 - 4, b);
        return 0;
      }
    case 0x2c: return 0;
    case 0x30: return _do_rom_bar_write(mask_32, value_32);
    case 0x34: return 0;
    case 0x38: return 0;
    /* pass trough the rest ... */
    default:   return p->cfg_write(reg, v, order);
    }
}

void
Pci_proxy_dev::dump() const
{
  Hw::Pci::If *p = _hwf;

  printf("       %04x:%02x:%02x.%x:\n",
         0, p->bus_nr(), _hwf->device_nr(), _hwf->function_nr());
#if 0
  char buf[130];
  libpciids_name_device(buf, sizeof(buf), _dev->vendor(), _dev->device());
  printf("              %s\n", buf);
#endif
}


// -----------------------------------------------------------------------
// Virtual PCI dummy device
// -----------------------------------------------------------------------

class Pci_dummy : public Pci_virtual_dev, public Device
{
private:
  unsigned char _cfg_space[4*4];

public:
  int irq_enable(Irq_info *irq)
  {
    irq->irq = -1;
    return -1;
  }

  Pci_dummy()
  {
    add_feature(this);
    _h = &_cfg_space[0];
    _h_len = sizeof(_cfg_space);
    cfg_hdr()->hdr_type = 0x80;
    cfg_hdr()->vendor_device = 0x02000400;
    cfg_hdr()->status = 0;
    cfg_hdr()->class_rev = 0x36440000;
    cfg_hdr()->cmd = 0x0;
  }

  bool match_hw_feature(const Hw::Dev_feature*) const { return false; }
  int dispatch(l4_umword_t, l4_uint32_t, L4::Ipc::Iostream&)
  { return -L4_ENOSYS; }
  void set_host(Device *d) { _host = d; }
  Device *host() const { return _host; }

private:
  Device *_host;
};


// ----------------------------------------------------------------------
// Basic virtual PCI bridge functionality
// ----------------------------------------------------------------------

Pci_bridge::Dev::Dev()
{
  memset(_fns, 0, sizeof(_fns));
}

void
Pci_bridge::Dev::add_fn(Pci_dev *f)
{
  for (unsigned i = 0; i < sizeof(_fns)/sizeof(_fns[0]); ++i)
    {
      if (!_fns[i])
	{
	  _fns[i] = f;
	  return;
	}
    }
}

void
Pci_bridge::Dev::sort_fns()
{
  // disabled sorting because the relation of two functions is questionable
#if 0
  unsigned n;
  for (n = 0; n < sizeof(_fns)/sizeof(_fns[0]) && _fns[n]; ++n)
    ;

  if (n < 2)
    return;

  bool exchg = false;

  do
    {
      exchg = false;
      for (unsigned i = 0; i < n - 1; ++i)
	{
	  if (_fns[i]->dev()->function_nr() > _fns[i+1]->dev()->function_nr())
	    {
	      Pci_dev *t = _fns[i];
	      _fns[i] = _fns[i+1];
	      _fns[i+1] = t;
	      exchg = true;
	    }
	}
      n -= 1;
    }
  while (exchg && n >= 1);
#endif
}

void
Pci_bridge::Bus::add_fn(Pci_dev *pd, int slot)
{
  if (slot >= 0)
    {
      _devs[slot].add_fn(pd);
      _devs[slot].sort_fns();
      return;
    }

  for (unsigned d = 0; d < Devs && !_devs[d].empty(); ++d)
    if (_devs[d].cmp(pd))
      {
	_devs[d].add_fn(pd);
	_devs[d].sort_fns();
	return;
      }

  for (unsigned d = 0; d < Devs; ++d)
    if (_devs[d].empty())
      {
	_devs[d].add_fn(pd);
	return;
      }
}

void
Pci_bridge::add_child(Device *d)
{
  Pci_dev *vp = d->find_feature<Pci_dev>();

  // hm, we do currently not add non PCI devices.
  if (!vp)
    return;

  _bus.add_fn(vp);
  Device::add_child(d);
}


void
Pci_bridge::add_child_fixed(Device *d, Pci_dev *vp, unsigned dn, unsigned fn)
{
  _bus.dev(dn)->fn(fn, vp);
  Device::add_child(d);
}


Pci_bridge *
Pci_bridge::find_bridge(unsigned bus)
{
  // printf("PCI[%p]: look for bridge for bus %x %02x %02x\n", this, bus, _subordinate, _secondary);
  if (bus == _secondary)
    return this;

  if (bus < _secondary || bus > _subordinate)
    return 0;

  for (unsigned d = 0; d < Bus::Devs; ++d)
    for (unsigned f = 0; f < Dev::Fns; ++f)
      {
	Pci_dev *p = _bus.dev(d)->fn(f);
	if (!p)
	  break;

	Pci_bridge *b = dynamic_cast<Pci_bridge*>(p);
	if (b && (b = b->find_bridge(bus)))
	  return b;
      }

  return 0;
}


bool
Pci_bridge::child_dev(unsigned bus, unsigned char dev, unsigned char fn,
                       Pci_dev **rd)
{
  Pci_bridge *b = find_bridge(bus);
  if (!b)
    {
      *rd = 0;
      return true;
    }

  if (dev >= Bus::Devs || fn >= Dev::Fns)
    {
      *rd = 0;
      return true;
    }

  *rd = b->_bus.dev(dev)->fn(fn);
  return true;
}

void
Pci_bridge::setup_bus()
{
  for (unsigned d = 0; d < Bus::Devs; ++d)
    for (unsigned f = 0; f < Dev::Fns; ++f)
      {
	Pci_dev *p = _bus.dev(d)->fn(f);
	if (!p)
	  break;

	Pci_bridge *b = dynamic_cast<Pci_bridge*>(p);
	if (b)
	  {
	    b->_primary = _secondary;
	    if (b->_secondary <= _secondary)
	      {
	        b->_secondary = ++_subordinate;
		b->_subordinate = b->_secondary;
	      }

	    b->setup_bus();

	    if (_subordinate < b->_subordinate)
	      _subordinate = b->_subordinate;
	  }
      }
}

void
Pci_bridge::finalize_setup()
{
  for (unsigned dn = 0; dn < Bus::Devs; ++dn)
    {
      if (!_bus.dev(dn)->empty())
	continue;

      for (unsigned fn = 0; fn < Dev::Fns; ++fn)
	if (_bus.dev(dn)->fn(fn))
	  {
	    Pci_dummy *dummy = new Pci_dummy();
	    _bus.dev(dn)->fn(0, dummy);
	    Device::add_child(dummy);
	    break;
	  }
    }
#if 0
  for (VDevice *c = dynamic_cast<VDevice*>(children()); c; c = c->next())
    c->finalize_setup();
#endif
}

namespace {
  static Feature_factory_t<Pci_proxy_dev, Hw::Pci::Dev> __pci_f_factory1;
  static Dev_factory_t<Pci_dummy> __pci_dummy_factory("PCI_dummy_device");
}

}

