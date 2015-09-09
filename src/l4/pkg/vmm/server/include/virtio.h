#pragma once

#include <l4/sys/types.h>
#include <l4/cxx/bitfield>
#include <l4/cxx/utils>

#include <cstdio>

static inline void wmb()
{
#ifdef __ARM_ARCH_7A__
  asm volatile ("dmb" : : : "memory");
#else
#warning Missing proper memory write barrier
  asm volatile ("" : : : "memory");
#endif
}

namespace Virtio {

template< typename T >
class Ptr
{
public:
  enum Invalid_type { Invalid };
  Ptr() {}
  Ptr(Invalid_type) : _p(~0ULL) {}

  explicit Ptr(l4_uint64_t vm_addr) : _p(vm_addr) {}
  l4_uint64_t get() const { return _p; }

  bool is_valid() const { return _p != ~0ULL; }

private:
  l4_uint64_t _p;
};

struct Dev_config
{
  struct Status
  {
    l4_uint32_t raw;
    Status() = default;
    explicit Status(l4_uint32_t v) : raw(v) {}

    CXX_BITFIELD_MEMBER( 0,  0, acked, raw);
    CXX_BITFIELD_MEMBER( 1,  1, driver, raw);
    CXX_BITFIELD_MEMBER( 2,  2, driver_ok, raw);
    CXX_BITFIELD_MEMBER( 7,  7, failed, raw);

    bool running() const
    {
      return raw == 7;
    }
  };

  struct Features
  {
    l4_uint32_t raw;
    Features() = default;
    explicit Features(l4_uint32_t v) : raw(v) {}

    CXX_BITFIELD_MEMBER(28, 28, ring_indirect_desc, raw);
    CXX_BITFIELD_MEMBER(29, 29, ring_event_idx, raw);
  };

  l4_uint32_t vendor;
  l4_uint32_t device;
  l4_uint32_t irq_status;
  Status status;
  Features host_features;
  Features guest_features;
  l4_uint32_t page_size;
  unsigned num_queues;
};


class Ring
{
public:
  class Desc
  {
  public:
    struct Flags
    {
      l4_uint16_t raw;
      Flags() = default;
      explicit Flags(l4_uint16_t v) : raw(v) {}

      CXX_BITFIELD_MEMBER( 0,  0, next, raw);
      CXX_BITFIELD_MEMBER( 1,  1, write, raw);
      CXX_BITFIELD_MEMBER( 2,  2, indirect, raw);
    };

    Ptr<void> addr;
    l4_uint32_t len;
    Flags flags;
    l4_uint16_t next;

    template<typename T>
    Ptr<T> buf() const { return Ptr<T>(addr.get()); }

    void dump(unsigned idx) const
    {
      printf("D[%04x]: %08llx (%x) f=%04x n=%04x\n",
             idx, addr.get(), len, (unsigned)flags.raw, (unsigned)next);
    }
  };

  class Avail
  {
  public:
    struct Flags {
      l4_uint16_t raw;
      Flags() = default;
      explicit Flags(l4_uint16_t v) : raw(v) {}

      CXX_BITFIELD_MEMBER( 0,  0, no_irq, raw);
    };

    Flags flags;
    l4_uint16_t idx;
    l4_uint16_t ring[];
  };

  class Used_elem
  {
  public:
    Used_elem() = default;
    Used_elem(l4_uint16_t id, l4_uint32_t len) : id(id), len(len) {}
    l4_uint32_t id;
    l4_uint32_t len;
  };

  class Used
  {
  public:
    struct Flags
    {
      l4_uint16_t raw;
      Flags() = default;
      explicit Flags(l4_uint16_t v) : raw(v) {}

      CXX_BITFIELD_MEMBER( 0,  0, no_notify, raw);
    };

    Flags flags;
    l4_uint16_t idx;
    Used_elem ring[];
  };

  unsigned num;
  unsigned max_num;
  l4_uint32_t pfn;

  unsigned align;

  Desc *desc;
  Avail *avail;
  Used *used;

  l4_uint16_t current_avail;
  l4_uint16_t current_used;

  void setup(void *base)
  {
    desc  = (Desc *)base;
    avail = (Avail *)&desc[num];
    used  = (Used *)((((l4_addr_t)&avail->ring[num])
                      + sizeof(l4_uint16_t) + align - 1) & ~(align - 1));

    current_used = 0;
    current_avail = 0;
    used->idx = 0;

    printf("VQ[%p]: num=%d align=%x d:%p a:%p u:%p\n",
           this, num, align, desc, avail, used);
  }

  Desc *next_avail()
  {
    if (avail && current_avail != avail->idx)
      return &desc[avail->ring[current_avail++ & (num - 1)]];
    return 0;
  }

  bool desc_avail()
  {
    return current_avail != avail->idx;
  }

  void consumed(Desc *d, l4_uint32_t len = 0)
  {
    l4_uint16_t id = d - desc;
    l4_uint16_t i = cxx::access_once(&used->idx);

    used->ring[i & (num -1)] = Used_elem(id, len);
    wmb();
    cxx::write_now(&used->idx, (l4_uint16_t)(i + 1));
  }

  void dump(Desc const *d)
  { d->dump(d - desc); }
};
}
