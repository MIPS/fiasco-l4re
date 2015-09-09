/*
 * (c) 2010 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include <l4/re/protocols>

#include <l4/cxx/ipc_server>

#include <l4/re/env>
#include <l4/re/namespace>
#include <l4/re/dataspace-sys.h>
#include <l4/re/inhibitor-sys.h>

#include <l4/re/error_helper>

#include <l4/vbus/vbus_types.h>
#include <l4/vbus/vdevice-ops.h>

#include <cstdio>

#include "debug.h"
#include "hw_msi.h"
#include "vbus.h"
#include "vmsi.h"
#include "vicu.h"
#include "server.h"
#include "res.h"
#include "cfg.h"
#include "vbus_factory.h"

Vi::System_bus::Root_resource_factory::Factory_list
  Vi::System_bus::Root_resource_factory::_factories;

namespace {

class Root_irq_rs : public Resource_space
{
private:
  Vi::System_bus *_bus;
  Vi::Sw_icu *_icu;

public:
  Root_irq_rs(Vi::System_bus *bus)
    : Resource_space(), _bus(bus), _icu(new Vi::Sw_icu())
  {
    _bus->add_child(_icu);
    _bus->sw_icu(_icu);
    _icu->name("L4ICU");
  }

  bool request(Resource *parent, Device *, Resource *child, Device *)
  {
    if (0)
      {
        printf("VBUS: IRQ resource request: ");
        child->dump();
      }

    if (!parent)
      return false;

    if (!_icu)
      {
        _icu = new Vi::Sw_icu();
        _bus->add_child(_icu);
        _bus->sw_icu(_icu);
      }

    d_printf(DBG_DEBUG2, "Add IRQ resources to vbus: ");
    if (dlevel(DBG_DEBUG2))
      child->dump();

    _icu->add_irqs(child);
    return _bus->add_resource_to_bus(child);
  };

  bool alloc(Resource *parent, Device *, Resource *child, Device *, bool)
  {
    d_printf(DBG_DEBUG2, "Allocate virtual IRQ resource ...\n");
    if (dlevel(DBG_DEBUG2))
      child->dump();

    Vi::Msi_resource *msi = dynamic_cast<Vi::Msi_resource*>(child);
    if (!msi || !parent)
      return false;

    d_printf(DBG_DEBUG2, "  Allocate Virtual MSI...\n");

    if (!_icu)
      {
        _icu = new Vi::Sw_icu();
        _bus->add_child(_icu);
      }

    int nr = _icu->alloc_irq(msi->flags(), msi->hw_msi());
    if (nr < 0)
      {
        d_printf(DBG_ERR, "ERROR: cannot allocate MSI resource\n");
        return false;
      }

    msi->start_end(nr, nr);
    msi->del_flags(Resource::F_disabled);

    if (dlevel(DBG_DEBUG2))
      {
        msi->dump(4);
        msi->hw_msi()->dump(4);
      }

    return _bus->add_resource_to_bus(msi);
  }

  ~Root_irq_rs() {}
};

class Root_x_rs : public Resource_space
{
private:
  Vi::System_bus *_bus;

public:
  Root_x_rs(Vi::System_bus *bus) : Resource_space(), _bus(bus)
  {}

  bool request(Resource *parent, Device *, Resource *child, Device *)
  {
    if (0)
      {
        printf("VBUS: X resource request: ");
        child->dump();
      }

    if (!parent)
      return false;


    return _bus->add_resource_to_bus(child);
  }

  bool alloc(Resource *, Device *, Resource *, Device *, bool)
  { return false; }

  ~Root_x_rs() {}
};
}



namespace Vi {

System_bus::System_bus(Inhibitor_mux *mux)
: Inhibitor_provider(mux), _sw_icu(0)
{
  add_feature(this);
  add_resource(new Root_resource(Resource::Irq_res, new Root_irq_rs(this)));
  Resource_space *x = new Root_x_rs(this);
  add_resource(new Root_resource(Resource::Mmio_res, x));
  add_resource(new Root_resource(Resource::Mmio_res | Resource::F_prefetchable, x));
  add_resource(new Root_resource(Resource::Io_res, x));
  typedef Root_resource_factory RF;
  for (RF::Factory_list::Const_iterator i = RF::_factories.begin();
      i != RF::_factories.end();
      ++i)
    add_resource((*i)->create(this));
}

System_bus::~System_bus()
{
  registry->unregister_obj(this);
  // FIXME: must delete all devices
}

/**
 * \brief Add the region described by \a r to the VBUS resources.
 *
 * Adds the resource to the resources available through this V-BUS.
 * This also supports overlapping resources by merging them together.
 */
bool
System_bus::add_resource_to_bus(Resource *r)
{
  auto x = _resources.find(r);
  if (x == _resources.end())
    {
      _resources.insert(r);
      return true;
    }

  // overlapping resource entry already found
  if (typeid (*r) != typeid (**x))
    {
      if (dlevel(DBG_ERR))
        {
          printf("error: overlapping incompatible resources for vbus\n");
          printf("       new:   "); r->dump(); puts(" conflicts with");
          printf("       found: "); (*x)->dump(); puts("");
        }
      return false;
    }

  if ((*x)->contains(*r))
    // already fully included
    return true;

  if (r->contains(**x))
    {
      _resources.erase(x);
      _resources.insert(r);
      return true;
    }

  if (dlevel(DBG_ERR))
    {
      printf("error: oddly overlaping resources for vbus\n");
      printf("       new:   "); r->dump(); puts(" confilicts with");
      printf("       found: "); (*x)->dump(); puts("");
    }
  return false;
}

bool
System_bus::resource_allocated(Resource const *r) const
{
  if (r->disabled())
    return false;

  auto i = _resources.find(const_cast<Resource *>(r));
  if (i == _resources.end())
    return false;

  return (*i)->contains(*r);
}

int
System_bus::pm_suspend()
{
  inhibitor_release(L4VBUS_INHIBITOR_SUSPEND);

  return 0;
}

int
System_bus::pm_resume()
{
  inhibitor_acquire(L4VBUS_INHIBITOR_SUSPEND, "vbus active");

  return 0;
}

void
System_bus::dump_resources() const
{
  for (auto i = _resources.begin();
       i != _resources.end(); ++i)
    (*i)->dump();
}

int
System_bus::request_resource(L4::Ipc::Iostream &ios)
{
  l4vbus_resource_t res;
  ios.get(res);

  Resource ires(res.type, res.start, res.end);
  if (dlevel(DBG_DEBUG2))
    {
      printf("request resource: ");
      ires.dump();
      puts("");
    }

  auto i = _resources.find(&ires);

  if (0)
    dump_resources();

  if (i == _resources.end() || !(*i)->contains(ires))
    return -L4_ENOENT;

  if (0)
    if (dlevel(DBG_DEBUG))
      {
        printf("  found resource: ");
        (*i)->dump();
        puts("");
      }

  if (res.type == L4VBUS_RESOURCE_PORT)
    {
      l4_uint64_t sz = res.end + 1 - res.start;

      int szl2 = 0;
      while ((1UL << szl2) < sz)
        ++szl2;

      if ((1UL << szl2) > sz)
        --szl2;

      ios << L4::Ipc::Snd_fpage::io(res.start, szl2, L4_FPAGE_RWX);
      return L4_EOK;
    }


  return -L4_ENOENT;
}

int
System_bus::request_iomem(L4::Ipc::Iostream &ios)
{
  L4::Opcode op;
  ios >> op;
  switch (op)
    {
    case L4Re::Dataspace_::Map:
        {
          l4_addr_t offset, spot;
          unsigned long flags;
          ios >> offset >> spot >> flags;

          Resource pivot(L4VBUS_RESOURCE_MEM, offset, offset);
          auto r = _resources.find(&pivot);

          if (r == _resources.end())
            {
              if (dlevel(DBG_INFO))
                {
                  printf("Request: No resource at %lx\n", offset);
                  printf("Available resources:\n");
                  dump_resources();
                }
              return -L4_ERANGE;
            }

          offset = l4_trunc_page(offset);

          l4_addr_t st = l4_trunc_page((*r)->start());
          l4_addr_t adr = (*r)->map_iomem();

          if (!adr)
            return -L4_ENOMEM;

          adr = l4_trunc_page(adr);

          l4_addr_t addr = offset - st + adr;
          unsigned char order
            = l4_fpage_max_order(L4_PAGESHIFT,
                                 addr, addr, addr + (*r)->size(), spot);

          // we also might want to do WB instead of UNCACHED...
          ios << L4::Ipc::Snd_fpage::mem(l4_trunc_size(addr, order), order,
                                    L4_FPAGE_RWX, l4_trunc_page(spot),
                                    L4::Ipc::Snd_fpage::Map,
                                    L4::Ipc::Snd_fpage::Uncached);
          return L4_EOK;
        }
    }
  return -L4_ENOSYS;
};

int
System_bus::inhibitor_dispatch(L4::Ipc::Iostream &ios)
{
  L4::Opcode op;
  ios >> op;

  switch (op)
    {
    case L4Re::Inhibitor_::Acquire:
      inhibitor_acquire(ios);
      return L4_EOK;
    case L4Re::Inhibitor_::Release:
        {
          l4_umword_t id;
          ios >> id;
          inhibitor_release(id);
          return L4_EOK;
        }
    case L4Re::Inhibitor_::Next_lock_info:
        {
          l4_mword_t id;
          ios >> id;
          ++id;
          if (id >= L4VBUS_INHIBITOR_MAX)
            return -L4_ENODEV;

          static char const *const names[] =
          {
            /* [L4VBUS_INHIBITOR_SUSPEND]  = */ "suspend",
            /* [L4VBUS_INHIBITOR_SHUTDOWN] = */ "shutdown",
            /* [L4VBUS_INHIBITOR_WAKEUP]   = */ "wakeup"
          };

          ios << (l4_umword_t)id << names[id];
          return L4_EOK;
        }
    default:
      return -L4_ENOSYS;
    }
}

int
System_bus::dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios)
{
  l4_msgtag_t tag;
  ios >> tag;

  switch (tag.label())
    {
    case L4_PROTO_IRQ: // The ICU for event source
    case L4Re::Protocol::Event:
      return Vbus_event_source::dispatch(obj, ios);
    case L4Re::Protocol::Inhibitor:
      return inhibitor_dispatch(ios);

    case L4Re::Protocol::Dataspace:
      return request_iomem(ios);

    case 0:
        {
          l4vbus_device_handle_t devid;
          l4_uint32_t func;
          ios >> devid >> func;
          Device *dev = get_dev_by_id(devid);
          if (!dev)
            return -L4_ENODEV;
          return dev->vdevice_dispatch(obj, func, ios);
        }

    default:
      return -L4_EBADPROTO;
    }

}

int
System_bus::dispatch(l4_umword_t, l4_uint32_t func, L4::Ipc::Iostream &ios)
{
  switch (func)
    {
    case L4vbus_vbus_request_resource:
      return request_resource(ios);
    default:
      return -L4_ENOSYS;
    }
}

Vbus_event_source::Vbus_event_source()
{
  using L4Re::Util::Auto_cap;
  using L4Re::chkcap;
  using L4Re::chksys;

  Auto_cap<L4Re::Dataspace>::Cap buffer_ds
    = chkcap(L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>(),
             "allocate event-buffer data-space capability");

  chksys(L4Re::Env::env()->mem_alloc()->alloc(L4_PAGESIZE, buffer_ds.get()),
         "allocate event-buffer data-space");

  chksys(buffer.attach(buffer_ds.get(), L4Re::Env::env()->rm()),
         "attach event-buffer data-space");

  _ds = buffer_ds.release();
}

/**
 * \brief Send device notification to client.
 *
 * \param dev device the notification is originating from.
 * \param type event type to be sent, see event_enums.h for types.
 * \param event event ID.
 * \param value value for the event.
 * \param syn trigger an IRQ for the event if true.
 */
bool
System_bus::dev_notify(Device const *dev, unsigned type,
                       unsigned event, unsigned value, bool syn)
{
  Vbus_event_source::Event ev;
  ev.time = l4_kip_clock(l4re_kip());
  ev.payload.type = type;
  ev.payload.code = event;
  ev.payload.value = value;
  ev.payload.stream_id = (l4vbus_device_handle_t)dev;

  return Vbus_event_source::put(ev, syn);
}

int
System_bus::get_stream_info_for_id(l4_umword_t dev_id, L4Re::Event_stream_info *info)
{
  Device *dev = get_dev_by_id(dev_id);
  if (!dev)
    return -L4_ENOSYS;

  Io::Event_source_infos const *_info = dev->get_event_infos();
  if (!_info)
    return -L4_ENOSYS;

  *info = _info->info;
  return 0;
}

int
System_bus::get_stream_state_for_id(l4_umword_t dev_id, L4Re::Event_stream_state *state)
{
  Device *dev = get_dev_by_id(dev_id);
  if (!dev)
    return -L4_ENOSYS;

  Io::Event_source_infos const *_info = dev->get_event_infos();
  if (!_info)
    return -L4_ENOSYS;

  *state = _info->state;
  return 0;
}

L4::Cap<void>
Vbus_event_source::rcv_cap()
{
  return ::rcv_cap;
}

void
System_bus::inhibitor_signal(l4_umword_t id)
{
  // FIXME: need a subscribed flag!
  // only signal if this inhibitor is actually acquired
  if (!inhibitor_acquired(id))
    return;

  Vbus_event_source::Event ev;
  ev.time = l4_kip_clock(l4re_kip());
  ev.payload.type = L4RE_EV_PM;
  ev.payload.code = id;
  ev.payload.value = 1;
  ev.payload.stream_id = (l4vbus_device_handle_t)0;

  put(ev);
}

}
