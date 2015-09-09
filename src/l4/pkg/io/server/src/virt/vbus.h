/*
 * (c) 2010 Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <set>
#include <l4/cxx/ipc_server>
#include <l4/cxx/hlist>

#include <l4/vbus/vbus_types.h>
#include <l4/re/util/event_svr>
#include <l4/re/util/event_buffer>

#include "vdevice.h"
#include "device.h"
#include "inhibitor_mux.h"

namespace Vi {
class Sw_icu;

struct Vbus_event_source : L4Re::Util::Event_svr<Vbus_event_source>
{
  typedef L4Re::Util::Event_buffer::Event Event;
  L4Re::Util::Event_buffer buffer;
  void reset_event_buffer() { buffer.reset(); }
  static L4::Cap<void> rcv_cap();

  Vbus_event_source();

  bool put(Event const &ev, bool trigger = true)
  {
    bool res = buffer.put(ev);
    if (res && trigger)
      _irq.trigger();
    return res;
  }

  virtual int get_stream_info_for_id(l4_umword_t, L4Re::Event_stream_info *) = 0;
  virtual int get_stream_state_for_id(l4_umword_t, L4Re::Event_stream_state *) = 0;
};

class System_bus :
  public Device,
  public Dev_feature,
  public L4::Server_object,
  public Inhibitor_provider,
  private Vbus_event_source
{
public:
  // Factory to create root resource objects
  struct Root_resource_factory : cxx::H_list_item_t<Root_resource_factory>
  {
    virtual Root_resource *create(System_bus *bus) const = 0;
    Root_resource_factory()
    { _factories.push_front(this); }

    typedef cxx::H_list<Root_resource_factory> Factory_list;
    static Factory_list _factories;
  };

  // template to create a factory for root objects of type
  // \a TYPE.
  template< unsigned TYPE, typename RS >
  struct Root_resource_factory_t : Root_resource_factory
  {
    Root_resource *create(System_bus *bus) const
    {
      return new Root_resource(TYPE, new RS(bus));
    }
  };

  // comparison operation to sort resources
  struct Res_cmp
  {
    bool operator () (Resource const *a, Resource const *b) const
    {
      if (a->type() == b->type())
        return a->lt_compare(b);
      return a->type() < b->type();
    }
  };

  // Set of all resources of all types
  typedef std::set<Resource*, Res_cmp> Resource_set;


  // Vi::System_bus interface
  explicit System_bus(Inhibitor_mux *hw_bus);
  ~System_bus();

  void dump_resources() const;
  bool add_resource_to_bus(Resource *r);
  Resource_set const *resource_set() const { return &_resources; }
  Sw_icu *sw_icu() const { return _sw_icu; }
  void sw_icu(Sw_icu *icu) { _sw_icu = icu; }

  char const *inhibitor_name() const
  { return Device::name(); }

  // L4::Server_object interface
  int dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios);


  // Vi::Device interface
  int dispatch(l4_umword_t, l4_uint32_t func, L4::Ipc::Iostream &ios);
  bool match_hw_feature(Hw::Dev_feature const *) const { return false; }


  // Dev_feature interface
  void set_host(Device *d) { _host = d; }
  Device *host() const { return _host; }


  // Device::pm_* interface
  int pm_suspend();
  int pm_resume();


  // Device interface
  bool resource_allocated(Resource const *) const;


  // Pm_client interface
  void pm_notify(unsigned, unsigned) {}

  bool dev_notify(Device const *dev, unsigned type,
                  unsigned event, unsigned value, bool syn);

  void inhibitor_signal(l4_umword_t id);
private:
  int request_resource(L4::Ipc::Iostream &ios);
  int request_iomem(L4::Ipc::Iostream &ios);
  int inhibitor_dispatch(L4::Ipc::Iostream &ios);

  int get_stream_info_for_id(l4_umword_t, L4Re::Event_stream_info *);
  int get_stream_state_for_id(l4_umword_t, L4Re::Event_stream_state *);

  Resource_set _resources;
  Device *_host;
  Sw_icu *_sw_icu;
};

}
