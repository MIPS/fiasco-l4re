/*
 * (c) 2013 Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "pci.h"

#include "cfg.h"
#include "debug.h"
#include "hw_msi.h"
#include "main.h"

namespace Hw { namespace Pci {

namespace {

class Saved_msi_cap : public Saved_cap
{
public:
  explicit Saved_msi_cap(unsigned pos) : Saved_cap(Cap::Msi, pos) {}

private:
  enum
  {
    Control = 0x02,
    Addr_lo = 0x04,
    Addr_hi = 0x08,
    Data_32 = 0x08,
    Data_64 = 0x0c
  };

  l4_uint32_t addr_lo, addr_hi;
  l4_uint16_t data;
  l4_uint16_t control;

  void _save(Config cap)
  {
    cap.read(Control, &control);
    cap.read(Addr_lo, &addr_lo);
    if (control & (1 << 7))
      {
        cap.read(Addr_hi, &addr_hi);
        cap.read(Data_64, &data);
      }
    else
      cap.read(Data_32, &data);
  }

  void _restore(Config cap)
  {
    cap.write(Addr_lo, addr_lo);
    if (control & (1 << 7))
      {
        cap.write(Addr_hi, addr_hi);
        cap.write(Data_64, data);
      }
    else
      cap.write(Data_32, data);
    cap.write(Control, control);
  }

};

static unsigned _last_msi;

class Msi
{
public:
  enum Addrs
  {
    Base_hi = 0,
    Base_lo = 0xfee00000,
  };

  enum Mode
  {
    Dest_mode_phys = 0 << 2,
    Dest_mode_log  = 1 << 2,
  };

  enum Redir
  {
    Dest_redir_cpu     = 0 << 3,
    Dest_redir_lowprio = 1 << 3,
  };

  enum Delivery
  {
    Data_del_fixed   = 0 << 8,
    Data_del_lowprio = 1 << 8,
  };

  enum Level
  {
    Data_level_deassert = 0 << 14,
    Data_level_assert   = 1 << 14,
  };

  enum Trigger
  {
    Data_trigger_edge  = 0 << 15,
    Data_trigger_level = 1 << 15,
  };

  static unsigned data(int vector) { return vector & 0xff; }
};

class Msi_res : public Hw::Msi_resource
{
public:
  Msi_res(unsigned msi, Bus *bus, Cfg_addr cfg_addr)
  : Msi_resource(msi), _bus(bus), _cfg_addr(cfg_addr)
  {}

  int bind(L4::Cap<L4::Irq> irq, unsigned mode);
  int unbind();

  void dump(int indent) const
  { Resource::dump("MSI   ", indent);  }

private:
  Bus *_bus;
  Cfg_addr _cfg_addr;
};

int
Msi_res::bind(L4::Cap<L4::Irq> irq, unsigned mode)
{
  int err = Msi_resource::bind(irq, mode);
  if (err < 0)
    return err;

  l4_umword_t msg = 0;
  int e2 = l4_error(system_icu()->icu->msi_info(pin(), &msg));
  if (e2 < 0)
    {
      d_printf(DBG_ERR, "ERROR: could not get MSI message (pin=%x)\n", pin());
      return e2;
    }

  // MSI capability
  l4_uint32_t ctl;
  int msg_offs = 8;

  _bus->cfg_read(_cfg_addr + 2, &ctl, Cfg_short);
  if (ctl & (1 << 7))
    msg_offs = 12;

  _bus->cfg_write(_cfg_addr + 4, Msi::Base_lo | Msi::Dest_mode_phys | Msi::Dest_redir_cpu, Cfg_long);

  if (ctl & (1 << 7))
    _bus->cfg_write(_cfg_addr + 8, Msi::Base_hi, Cfg_long);

  _bus->cfg_write(_cfg_addr + msg_offs, Msi::Data_level_assert | Msi::Data_trigger_edge | Msi::Data_del_fixed | Msi::data(msg), Cfg_short);

  _bus->cfg_write(_cfg_addr + 2, ctl | 1, Cfg_short);

  d_printf(DBG_DEBUG2, "MSI: enable kernel PIN=%x hwpci=%02x:%02x.%x: reg=%03x msg=%lx\n",
           pin(), _cfg_addr.bus(), _cfg_addr.dev(), _cfg_addr.fn(),
           _cfg_addr.reg(), msg);

  return err;
}

int
Msi_res::unbind()
{
  // disable MSI
  l4_uint32_t ctl;
  _bus->cfg_read(_cfg_addr + 2, &ctl, Cfg_short);
  _bus->cfg_write(_cfg_addr + 2, ctl & ~1, Cfg_short);

  return Msi_resource::unbind();
}

}


void
Dev::parse_msi_cap(Cfg_addr cap_ptr)
{
  if (   !Io_config::cfg->transparent_msi(host())
      || !system_icu()->info.supports_msi())
    return;

  unsigned msi = _last_msi++;
  if (msi >= system_icu()->info.nr_msis)
    {
      d_printf(DBG_WARN, "WARNING: run out of MSI vectors, use normal IRQ\n");
      return;
    }

  for (Resource_list::const_iterator i = host()->resources()->begin();
      i != host()->resources()->end(); ++i)
    {
      if ((*i)->type() == Resource::Irq_res)
        {
          (*i)->set_empty();
          (*i)->add_flags(Resource::F_disabled);
        }
    }

  d_printf(DBG_DEBUG, "Use MSI PCI device %02x:%02x:%x: pin=%x\n",
           bus()->num, host()->adr() >> 16, host()->adr() & 0xff, msi);

  Resource *res = new Msi_res(msi, bus(), cap_ptr);
  flags.msi() = true;
  _host->add_resource_rq(res);
  _saved_state.add_cap(new Saved_msi_cap(cap_ptr.reg()));
}

}}
