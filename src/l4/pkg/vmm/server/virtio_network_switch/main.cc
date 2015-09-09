#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/re/debug>

#include <l4/sys/thread>
#include <l4/sys/factory>
#include <l4/sys/compiler.h>

#include <l4/cxx/bitfield>
#include <l4/cxx/ipc_stream>
#include <l4/cxx/ipc_server>

#include <l4/re/dataspace>
#include <l4/re/util/meta>

#include <cstring>
#include <unistd.h>
#include <algorithm>

#include "virtio.h"

#include "pthread.h"

//#define CONFIG_STATS 1
//#define CONFIG_BENCHMARK 1

static L4::Cap<L4::Kobject> rcv_cap[2];

namespace Virtio {
class Mmio_remote : public L4::Kobject_t<Mmio_remote, L4::Kobject>
{
  L4_KOBJECT(Mmio_remote)
};
}

template< typename T >
class Irq_object : public L4::Server_object
{
private:
  int dispatch(l4_umword_t, L4::Ipc::Iostream &)
  {
    static_cast<T*>(this)->kick();
    return 0;
  }

public:
  L4::Cap<L4::Irq> irq_obj_cap() const
  { return L4::cap_cast<L4::Irq>(obj_cap()); }
};

template<typename T>
class Mmio_device_t : public L4::Server_object
{

  int dispatch(l4_umword_t, L4::Ipc::Iostream &ios)
  {
    l4_msgtag_t tag;
    ios >> tag;

    switch (tag.label())
      {
      case L4::Meta::Protocol:
        return L4Re::Util::handle_meta_request<Virtio::Mmio_remote>(ios);

      case 0:
        break;

      default:
        return -L4_EBADPROTO;
      }

    L4::Opcode op;
    ios >> op;
    switch (op)
      {
      case 0:
          {
            unsigned reg;
            char size;
            ios >> reg >> size;
            l4_uint32_t val = static_cast<T*>(this)->read(reg, size);
            ios << val;
            return 0;
          }

      case 1:
          {
            unsigned reg;
            char size;
            l4_uint32_t val;
            ios >> reg >> size >> val;
            static_cast<T*>(this)->write(reg, size, val);
            return 0;
          }

      case 2:
          {
            L4::Ipc::Snd_fpage irq_cap_fp, ds_cap_fp;
            l4_addr_t ds_base;

            if (!tag.items())
              return -L4_EINVAL;

            ios >> ds_base >> irq_cap_fp >> ds_cap_fp;

            if (!irq_cap_fp.cap_received())
              return -L4_EINVAL;

            kick_guest_irq.get().move(L4::cap_cast<L4::Irq>(rcv_cap[0]));
            static_cast<T*>(this)->check_n_init_shm(ds_cap_fp, rcv_cap[1],
                                                    ds_base);

            ios << static_cast<T*>(this)->irq_obj_cap()
                << static_cast<T*>(this)->device_config_ds.get();
            return 0;
          }
      }

    return 0;
  }

protected:
  L4Re::Util::Auto_cap<L4::Irq>::Cap kick_guest_irq;
};

namespace Virtio {


class Dev : public Mmio_device_t<Dev>, public Irq_object<Dev>
{
public:
  typedef Irq_object<Dev> Irq;
  typedef Mmio_device_t<Dev> Mmio;

  template<typename REG>
  void register_obj(REG *registry, char const *service = 0)
  {
    L4Re::chkcap(registry->register_irq_obj(static_cast<Irq*>(this)));
    if (service)
      L4Re::chkcap(registry->register_obj(static_cast<Mmio*>(this), service));
    else
      L4Re::chkcap(registry->register_obj(static_cast<Mmio*>(this)));

    kick_guest_irq = L4Re::Util::cap_alloc.alloc<L4::Irq>();
    guest_shm = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  }

protected:
  Ring *_current_q;
  Ring *_queues;
  unsigned _num_queues;

  L4Re::Rm::Auto_region<l4_addr_t> shm;
  l4_addr_t shm_base;
  l4_mword_t _shm_offset;
  L4Re::Util::Auto_cap<L4Re::Dataspace>::Cap guest_shm;
  L4Re::Rm::Auto_region<Virtio::Dev_config*> device_config;
public:
  L4Re::Util::Auto_cap<L4Re::Dataspace>::Cap device_config_ds;

public:
  Dev(l4_uint32_t vendor, l4_uint32_t device)
  : _current_q(0), _queues(0), _num_queues(0)
  {
    L4Re::Util::Auto_cap<L4Re::Dataspace>::Cap cfg
      = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>());
    L4Re::chksys(L4Re::Env::env()->mem_alloc()->alloc(L4_PAGESIZE, cfg.get()));
    L4Re::chksys(L4Re::Env::env()->rm()->attach(&device_config, L4_PAGESIZE,
                                                L4Re::Rm::Search_addr,
                                                cfg.get()));
    device_config_ds = cfg;

    device_config->vendor = vendor;
    device_config->device = device;
    device_config->irq_status = 0;
    device_config->status = Dev_config::Status(0);
    device_config->host_features = Dev_config::Features(0);
    device_config->page_size = L4_PAGESIZE;
  }

  template<typename T>
  T *access(Virtio::Ptr<T> p)
  { return (T*)(p.get() + _shm_offset); }

  void check_n_init_shm(L4::Ipc::Snd_fpage shm_cap_fp, L4::Cap<void> rcv_cap,
                        l4_addr_t base)
  {
    if (!shm_cap_fp.cap_received())
      L4Re::chksys(-L4_EINVAL);

    guest_shm.get().move(L4::cap_cast<L4Re::Dataspace>(rcv_cap));
    L4Re::chksys(L4Re::Env::env()->rm()->attach(&shm, guest_shm->size(),
                 L4Re::Rm::Search_addr, guest_shm.get()));

    printf("PORT[%p]: shm @ %lx\n", this, shm.get());

    shm_base = base;
    _shm_offset = shm.get() - base;
  }

  virtual l4_uint32_t read_config(unsigned /*reg*/) { return 0; }
  virtual void write_config(unsigned /*reg*/, l4_uint32_t /*value*/) {}
  virtual void kick() = 0;

  Ring *queue(unsigned idx)
  {
    if (idx < _num_queues)
      return &_queues[idx];
    return 0;
  }

  virtual void reset() {}

  l4_uint32_t read(unsigned reg, char /*size*/)
  {
    if (reg >= 0x100)
      return read_config(reg - 0x100);

    switch (reg >> 2)
      {
      case 0: return *reinterpret_cast<l4_uint32_t const *>("virt");
      case 1: return 1;
      case 2: return device_config->device;
      case 3: return device_config->vendor;
      case 4: return device_config->host_features.raw;
      case 13:
        if (_current_q)
          return _current_q->max_num;
        else
          return 0;

      case 16:
        if (_current_q)
          return (unsigned long)_current_q->desc / device_config->page_size;
        else
          return 0;

      case 24:
        if (0)
          {
            l4_uint32_t tmp = device_config->irq_status;
            device_config->irq_status = 0;
            return tmp;
          }
        return 1;

      case 28: return device_config->status.raw;
      }
    return ~0;
  }

  void write(unsigned reg, char /*size*/, l4_uint32_t value)
  {
    if (reg >= 0x100)
      {
        write_config(reg - 0x100, value);
        return;
      }

    switch (reg >> 2)
      {
      case 4:
        device_config->guest_features.raw = value;
        break;

      case 10:
        device_config->page_size = value;
        break;

      case 12:
        _current_q = queue(value);
        break;

      case 14:
        if (_current_q)
          _current_q->num = _current_q->max_num >= value
                            ? value
                            : _current_q->max_num;
        break;

      case 15:
        if (_current_q)
          _current_q->align = value;
        break;

      case 16:
        if (_current_q)
          _current_q->setup((void *)(shm.get()
                                     + value * device_config->page_size
                                     - shm_base));
        break;

      case 20:
        // TODO: Kick must be implemented in VMM
        break;

      case 28:
        device_config->status.raw = value;
        if (value == 0)
          reset();
        break;
      }
  }
};

}

#ifdef CONFIG_STATS
static void *stats_thread_loop(void *);
#endif

class Virtio_net;

class Switch
{
public:
  struct Hdr_flags
  {
    l4_uint8_t raw;
    CXX_BITFIELD_MEMBER( 0, 0, need_csum, raw);
    CXX_BITFIELD_MEMBER( 1, 1, data_valid, raw);
  };

  struct Hdr
  {
    Hdr_flags flags;
    l4_uint8_t gso_type;
    l4_uint16_t hdr_len;
    l4_uint16_t gso_size;
    l4_uint16_t csum_start;
    l4_uint16_t csum_offset;
    l4_uint16_t num_buffers;
  };

  virtual void tx(Hdr const *, Virtio::Ring::Desc *descs, unsigned pkt,
                  Virtio_net *p) = 0;
};


class Virtio_net : public Virtio::Dev
{
public:
  struct Features : Virtio::Dev_config::Features
  {
    CXX_BITFIELD_MEMBER( 0,  0, csum, raw);       // host handles partial csum
    CXX_BITFIELD_MEMBER( 1,  1, guest_csum, raw); // guest handles partial csum
    CXX_BITFIELD_MEMBER( 5,  5, mac, raw);        // host has given mac
    CXX_BITFIELD_MEMBER( 6,  6, gso, raw);        // host handles packets /w any GSO
    CXX_BITFIELD_MEMBER( 7,  7, guest_tso4, raw); // guest handles TSOv4 in
    CXX_BITFIELD_MEMBER( 8,  8, guest_tso6, raw); // guest handles TSOv6 in
    CXX_BITFIELD_MEMBER( 9,  9, guest_ecn, raw);  // guest handles TSO[6] with ECN in
    CXX_BITFIELD_MEMBER(10, 10, guest_ufo, raw);  // guest handles UFO in
    CXX_BITFIELD_MEMBER(11, 11, host_tso4, raw);  // host handles TSOv4 in
    CXX_BITFIELD_MEMBER(12, 12, host_tso6, raw);  // host handles TSOv6 in
    CXX_BITFIELD_MEMBER(13, 13, host_ecn, raw);   // host handles TSO[6] with ECN in
    CXX_BITFIELD_MEMBER(14, 14, host_ufo, raw);   // host handles UFO
    CXX_BITFIELD_MEMBER(15, 15, mrg_rxbuf, raw);  // host can merge receive buffers
    CXX_BITFIELD_MEMBER(16, 16, status, raw);     // virtio_net_config.status available
    CXX_BITFIELD_MEMBER(17, 17, ctrl_vq, raw);    // Control channel available
    CXX_BITFIELD_MEMBER(18, 18, ctrl_rx, raw);    // Control channel RX mode support
    CXX_BITFIELD_MEMBER(19, 19, ctrl_vlan, raw);  // Control channel VLAN filtering
    CXX_BITFIELD_MEMBER(20, 20, ctrl_rx_extra, raw); // Extra RX mode control support
    CXX_BITFIELD_MEMBER(21, 21, guest_announce, raw); // Guest can announce device on the network
    CXX_BITFIELD_MEMBER(22, 22, mq, raw);         // Device supports Receive Flow Steering
    CXX_BITFIELD_MEMBER(23, 23, ctrl_mac_addr, raw); // Set MAC address
  };

  typedef Switch::Hdr Hdr;

  Features &host_features()
  { return static_cast<Features &>(device_config->host_features); }
  Features host_features() const
  { return static_cast<Features const &>(device_config->host_features); }

  enum
  {
    Rx = 0,
    Tx = 1
  };

#ifdef CONFIG_STATS
  unsigned long num_tx;
  unsigned long num_rx;
  unsigned long num_dropped;
  unsigned long num_irqs;
#endif

  Virtio_net()
  : Virtio::Dev(0x44, 1)
#ifdef CONFIG_STATS
    , num_tx(0), num_rx(0), num_dropped(0), num_irqs(0)
#endif
  {
    _queues = _q;
    device_config->num_queues = _num_queues = 2;
    _q[Rx].max_num = 0x1000;
    _q[Tx].max_num = 0x1000;

    host_features().csum()      = true;
    host_features().host_tso4() = true;
    host_features().host_tso6() = true;
    host_features().host_ufo()  = true;
    host_features().host_ecn()  = true;

    host_features().guest_csum() = true;
    host_features().guest_tso4() = true;
    host_features().guest_tso6() = true;
    host_features().guest_ufo()  = true;
    host_features().guest_ecn()  = true;
  }

  void reset()
  {
    _q[Rx].desc = 0;
    _q[Tx].desc = 0;
  }

  bool process_tx_q()
  {
    if (0)
      printf("%s: process tx queue\n", name);
    Virtio::Ring *q = &_q[Tx];

    l4_uint32_t irqs = 0;

    Virtio::Ring::Desc *d;
    do
      {
        q->used->flags.no_notify() = 1;
        if (0)
          printf("q->avail.idx=%d\n", q->avail->idx);
        while ((d = q->next_avail()))
          {
            if (0)
              d->dump(1);

#ifndef CONFIG_BENCHMARK
            if (L4_UNLIKELY(d->flags.write()))
              {
                printf("PORT[%p]: input buffer in TX queue (skip)\n", this);
                q->consumed(d);
                return true;
              }

            if (L4_UNLIKELY(!d->flags.next()))
              {
                printf("PORT[%p]: input w/o data\n", this);
                q->consumed(d);
                return true;
              }
#endif

            Hdr const *hdr = access(d->buf<Hdr const>());

            Virtio::Ring::Desc *pkt = &q->desc[d->next];

#ifndef CONFIG_BENSCHMARK
            if (L4_UNLIKELY(pkt->flags.write()))
              {
                printf("PORT[%p]: input buffer in TX queue (skip)\n", this);
                q->consumed(d);
                return true;
              }
#endif
#ifdef CONFIG_STATS
            ++num_tx;
#endif
            if (0)
              printf("TX[%s]\n", name ? name : "<unk>");
            net_switch->tx(hdr, q->desc, d->next, this);

            q->consumed(d);
            irqs = true;
          }

        q->used->flags.no_notify() = 0;
      }
    while (q->desc_avail());
    if (0)
      printf("%s: wait\n", name);
    return irqs;
  }

  void notify_guest(Virtio::Ring *queue)
  {
    if (queue->avail->flags.no_irq())
      return;

    // we do not care about this anywhere, so skip
    if (0)
      device_config->irq_status |= 1;

    kick_guest_irq->trigger();
#ifdef CONFIG_STATS
    ++num_irqs;
#endif
  }

  virtual void kick()
  {
    if (process_tx_q())
      notify_guest(&_q[Tx]);
  }

  bool rx(Hdr const *tx_hdr, Virtio::Ring::Desc *tx_descs, unsigned tx_idx,
          Virtio_net *tx_port)
  {
    if (L4_UNLIKELY(!device_config->status.running()))
      return false;

    if (0)
      printf("%s: copy packet\n", name);

    Virtio::Ring *q = &_q[Rx];

    if (0)
      printf("q->avail.idx=%d\n", q->avail->idx);
    Virtio::Ring::Desc *d = q->next_avail();

    if (L4_UNLIKELY(!d))
      return false;

#ifndef CONFIG_BENCHMARK
    if (L4_UNLIKELY(!d->flags.write()))
      {
        printf("PORT[%p]: invalid buffer in RX queue\n", this);
        q->consumed(d);
        notify_guest(q);
        return false;
      }

    if (L4_UNLIKELY(!d->flags.next()))
      {
        printf("PORT[%p]: invalid buffer in RX queue\n", this);
        q->consumed(d);
        notify_guest(q);
        return false;
      }
#endif

    Virtio::Ring::Desc *rxb = q->desc + d->next;
    Virtio::Ring::Desc *txb = tx_descs + tx_idx;
    char *rx_addr = access(rxb->buf<char>());
    char const *tx_addr = tx_port->access(txb->buf<char const>());

    Hdr *rx_hdr = access(d->buf<Hdr>());

    rx_hdr->flags.raw = 0;
    rx_hdr->gso_type = 0;

#ifdef CONFIG_STATS
    ++num_rx;
#endif

    if (tx_hdr->flags.need_csum())
      rx_hdr->flags.data_valid() = 1;

    unsigned long tx_space = txb->len;
    unsigned long rx_space = rxb->len;
    unsigned long rx_bytes = d->len;

    for (;;)
      {
        unsigned long cpy = std::min(tx_space, rx_space);

        memcpy(rx_addr, tx_addr, cpy);

        rx_addr += cpy;
        tx_addr += cpy;

        rx_space -= cpy;
        tx_space -= cpy;

        rx_bytes += cpy;

        if (tx_space == 0)
          {
            if (!txb->flags.next())
              break;

            txb = tx_descs + txb->next;
            tx_addr = tx_port->access(txb->buf<char const>());
            tx_space = txb->len;
          }

        if (rx_space == 0)
          {
            if (!rxb->flags.next())
              break;

            rxb = q->desc + rxb->next;
            rx_addr = access(rxb->buf<char>());
            rx_space = rxb->len;
          }

        if (0)
          printf("more to copy rx=%s\n", name);
      }

#ifndef CONFIG_BENCHMARK
    if (L4_UNLIKELY(tx_space))
      printf("PORT[%p]: truncate packet: %lx bytes left\n", this, tx_space);
#endif

    q->consumed(d, rx_bytes);
    notify_guest(q);

    return true;
  }

  Switch *net_switch;
  char const *name;

private:
  Virtio::Ring _q[2];
  friend void *stats_thread_loop(void *);
};


template<unsigned PORTS>
class Switch_t : public Switch
{
public:
  enum
  {
    Num_ports = PORTS
  };

  Virtio_net port[PORTS];

  Switch_t()
  {
    for (unsigned i = 0; i < PORTS; ++i)
      port[i].net_switch = this;
  }

  void tx(Hdr const *tx_hdr, Virtio::Ring::Desc *descs, unsigned pkt,
          Virtio_net *p)
  {
    for (unsigned i = 0; i < PORTS; ++i)
      if (&port[i] != p)
        {
          if (L4_UNLIKELY(!port[i].rx(tx_hdr, descs, pkt, p)))
            {
#ifdef CONFIG_STATS
              ++port[i].num_dropped;
#endif
            }
        }
  }
};


struct Loop_hooks
: L4::Ipc_svr::Ignore_errors,
  L4::Ipc_svr::Default_timeout,
  L4::Ipc_svr::Compound_reply
{
  void setup_wait(L4::Ipc::Istream &istr, L4::Ipc_svr::Reply_mode)
  {
    istr.reset();
    istr << L4::Ipc::Small_buf(rcv_cap[0].cap(), L4_RCV_ITEM_LOCAL_ID);
    istr << L4::Ipc::Small_buf(rcv_cap[1].cap(), L4_RCV_ITEM_LOCAL_ID);
    l4_utcb_br_u(istr.utcb())->bdr = 0;
  }
};

static L4Re::Util::Object_registry registry;
static L4::Server<Loop_hooks> server(l4_utcb());

static Switch_t<2> net;

#ifdef CONFIG_STATS
static void *stats_thread_loop(void *)
{
  for (;;)
    {
      sleep(1);
      for (unsigned i = 0; i < net.Num_ports; ++i)
        {
          Virtio_net *p = &net.port[i];
          printf("%s: tx:%ld rx:%ld drp:%ld irqs:%ld ri:%d:%d  ",
                 p->name, p->num_tx, p->num_rx, p->num_dropped, p->num_irqs,
                 p->device_config->status.running()
                 ? (unsigned)p->_q[0].avail->idx : -1,
                 (unsigned)p->_q[0].current_avail);
        }
      printf("\n");
    }
  return NULL;
};
#endif

int main()
{
  printf("Hello from virtio server\n");
  rcv_cap[0] = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Kobject>());
  rcv_cap[1] = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Kobject>());

#ifdef CONFIG_STATS
  pthread_t stats_thread;
  pthread_create(&stats_thread, NULL, stats_thread_loop, NULL);
#endif

  net.port[0].register_obj(&registry, "net0");
  net.port[0].name = "net0";
  net.port[1].register_obj(&registry, "net1");
  net.port[1].name = "net1";

  server.loop(registry);
  return 0;
}
