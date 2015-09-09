#pragma once

#include <l4/vbus/vbus>
#include <l4/re/dataspace>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>

#include "debug.h"
#include "vm_memmap.h"
#include "irq_server.h"
#include "ds_mmio_mapper.h"

namespace Vmm {

class Virt_bus
{
private:
  Dbg dbg;

public:
  explicit Virt_bus(L4::Cap<L4vbus::Vbus> bus)
  : dbg(Dbg::Vm_bus, "vmbus"), _bus(bus)
  {
    L4vbus::Icu dev;
    L4Re::chksys(_bus->root().device_by_hid(&dev, "L40009"),
                 "requesting ICU");
    _icu = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Icu>(),
                        "allocate ICU cap");
    L4Re::chksys(dev.vicu(_icu), "requesting ICU cap");
  }

  l4vbus_resource_t
  dev_get_resource(char const *dbg_name, char const *hid, unsigned res_id)
  {
    L4vbus::Device vbus_root = _bus->root();
    L4vbus::Device dev;
    L4Re::chksys(vbus_root.device_by_hid(&dev, hid), hid);

    l4vbus_resource_t res;
    L4Re::chksys(dev.get_resource(res_id, &res), hid);

    dbg.printf("%s: %08lx-%08lx\n", dbg_name, res.start, res.end);
    return res;
  }

  void map_mmio(Vm_mem &mem, char const *dbg_name, char const *hid,
                l4_addr_t addr, unsigned res_id = 0)
  {
    l4vbus_resource_t res = dev_get_resource(dbg_name, hid, res_id);

    l4_addr_t size = l4_round_page(res.end - res.start + 1);
    mem[Region::ss(addr, size)] = new Ds_handler(io_ds(), 0, size, res.start);
  }

  template< typename REG >
  void map_irq(REG *registry, Gic::Dist *gic_dist, char const *dbg_name,
               char const *hid, l4_addr_t addr, unsigned res_id = 0)
  {
    l4vbus_resource_t res = dev_get_resource(dbg_name, hid, res_id);
    Irq_svr *s = new Irq_svr(gic_dist, addr);
    dbg.printf("Bind virtual SPI %ld to PHYS IRQ %ld (irq=%p)\n",
               addr, res.start, s);
    L4Re::chksys(_icu->bind(res.start,
                            L4Re::chkcap(registry->register_irq_obj(s))));
    s->eoi();
  }

  template< typename REG >
  bool map_irq(REG *registry, Gic::Dist *gic_dist, Irq_svr *svr,
               unsigned virtual_spi, unsigned gsi)
  {
    if (!svr->obj_cap())
      L4Re::chkcap(registry->register_irq_obj(svr));

    int res = l4_error(_icu->bind(gsi, L4::cap_cast<L4::Irq>(svr->obj_cap())));
    if (res >= 0)
      {
        svr->gic_bind(gic_dist, virtual_spi);
        svr->eoi();
        dbg.printf("Bound virtual SPI %d to PHYS IRQ %d (irq=%p)\n",
                   virtual_spi, gsi, svr);
      }

    return res >= 0;
  }

  L4::Cap<L4Re::Dataspace> io_ds() const
  { return L4::cap_reinterpret_cast<L4Re::Dataspace>(_bus); }

  unsigned num_irqs() const
  {
    L4::Icu::Info i;
    if (l4_error(_icu->info(&i)))
      {
        dbg.printf("Failed to get ICU info\n");
        return 0;
      }
    return i.nr_irqs;
  }

private:
  L4::Cap<L4vbus::Vbus> _bus;
  L4::Cap<L4::Icu> _icu;
};

}
