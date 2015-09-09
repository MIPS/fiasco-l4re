#pragma once

#include "virtio.h"
#include "virtio_dev.h"

#include <l4/cxx/ipc_server>
#include <l4/cxx/ipc_stream>

#include <l4/sys/vcon>

#include <l4/re/error_helper>


class Virtio_console : public Virtio::Dev, private L4::Server_object
{
public:
  struct Features : Virtio::Dev::Features
  {
    CXX_BITFIELD_MEMBER(0, 0, console_size, raw);
    CXX_BITFIELD_MEMBER(1, 1, console_multiport, raw);
  };

  Virtio_console(Vmm::Vm_ram *iommu, L4::Cap<L4::Vcon> con,
                 Gic::Dist *gic, unsigned irq)
  : Virtio::Dev(iommu, 0x44, 3, gic, irq), con(con)
  {
    _queues = _q;
    _num_queues = 2;
    _q[0].max_num = 0x100;
    _q[1].max_num = 0x100;

    l4_vcon_attr_t attr;
    L4Re::chksys(con->get_attr(&attr), "console get_attr");
    attr.l_flags &= ~L4_VCON_ECHO;
    attr.o_flags &= ~L4_VCON_ONLRET;
    L4Re::chksys(con->set_attr(&attr), "console set_attr");
  }

  void reset()
  {
    _q[0].desc = 0;
    _q[1].desc = 0;
  }

  virtual void kick()
  {
    handle_input();
    Virtio::Ring *q = &_q[1];

    Virtio::Ring::Desc *d = q->next_avail();
    if (d)
      {
        con->write((char const *)_iommu->access(d->addr), d->len);
        q->consumed(d);
        _irq_status |= 1;
        _gic->inject_spi(_irq - 32, vmm_current_cpu_id);
      }
  }


  void handle_input()
  {
    Virtio::Ring *q = &_q[0];

    l4_uint32_t irqs = 0;

    while (1)
      {
        int r = con->read(NULL, 0);

        if (r <= 0)
          break; // empty

        Virtio::Ring::Desc *d = q->next_avail();

        if (!d)
          break;

        if (!d->flags.write())
          {
            Err().printf("Virtio_console: error read-only buffer in input queue\n");
            break;
          }

        char *buf = (char *)_iommu->access(cxx::access_once(&d->addr));
        unsigned len = cxx::access_once(&d->len);
        r = con->read(buf, len);
        if (r < 0)
          {
            Err().printf("Virtio_console: read error: %d\n", r);
            break;
          }

        if ((unsigned)r <= len)
          {
            q->consumed(d, r);
            irqs = true;
            break;
          }

        q->consumed(d, len);
        irqs = true;
      }

    if (irqs)
      {
        _irq_status |= 1;
        _gic->inject_spi(_irq - 32, vmm_current_cpu_id);
      }
  }

  template<typename REG>
  void register_obj(REG *registry)
  {
    con->bind(0, L4Re::chkcap(registry->register_irq_obj(this)));
  }

  int dispatch(l4_umword_t /*obj*/, L4::Ipc::Iostream &/*ios*/)
  {
    handle_input();
    return 0;
  }

private:
  Virtio::Ring _q[2];
  L4::Cap<L4::Vcon> con;
};
