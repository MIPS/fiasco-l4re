#pragma once

#include "arm_mmio_device.h"
#include "virtio.h"
#include "vm_ram.h"
#include "gic.h"

namespace Virtio {

class Dev : public Vmm::Mmio_device_t<Dev>
{
public:
  struct Status
  {
    l4_uint32_t raw;
    Status() = default;
    explicit Status(l4_uint32_t v) : raw(v) {}

    CXX_BITFIELD_MEMBER( 0,  0, acked, raw);
    CXX_BITFIELD_MEMBER( 1,  1, driver, raw);
    CXX_BITFIELD_MEMBER( 2,  2, driver_ok, raw);
    CXX_BITFIELD_MEMBER( 7,  7, failed, raw);
  };

  struct Features
  {
    l4_uint32_t raw;
    Features() = default;
    explicit Features(l4_uint32_t v) : raw(v) {}

    CXX_BITFIELD_MEMBER(28, 28, ring_indirect_desc, raw);
    CXX_BITFIELD_MEMBER(29, 29, ring_event_idx, raw);
  };

protected:
  l4_uint32_t _vendor;
  l4_uint32_t _device;
  l4_uint32_t _irq_status;
  Status _status;
  Features _host_features;
  Features _guest_features;
  l4_uint32_t _page_size;

  Ring *_current_q;
  Ring *_queues;
  unsigned _num_queues;

  unsigned _irq;
  Gic::Dist *_gic;
  Vmm::Vm_ram *_iommu;

public:
  Dev(Vmm::Vm_ram *iommu, l4_uint32_t vendor, l4_uint32_t device,
      Gic::Dist *gic, unsigned irq)
  : _vendor(vendor), _device(device), _irq_status(0), _status(0),
    _host_features(0), _page_size(1 << 12), _current_q(0), _queues(0),
    _num_queues(0), _irq(irq), _gic(gic), _iommu(iommu)
  {}

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

  l4_uint32_t read(unsigned reg, char /*size*/, Vmm::Cpu *)
  {
    if (reg >= 0x100)
      return read_config(reg - 0x100);

    switch (reg >> 2)
      {
      case 0: return *reinterpret_cast<l4_uint32_t const *>("virt");
      case 1: return 1;
      case 2: return _device;
      case 3: return _vendor;
      case 4: return _host_features.raw;
      case 13:
        if (_current_q)
          return _current_q->max_num;
        else
          return 0;

      case 16:
        if (_current_q)
          return (unsigned long)_current_q->desc / _page_size;
        else
          return 0;

      case 24:
        // hack: we have always a queue IRQ
        return 1;
        if (0)
          {
            l4_uint32_t tmp = _irq_status;
            _irq_status = 0;
            return tmp;
          }
      case 28: return _status.raw;
      }
    return ~0;
  }

  void write(unsigned reg, char /*size*/, l4_uint32_t value, Vmm::Cpu *)
  {
    if (reg >= 0x100)
      {
        write_config(reg - 0x100, value);
        return;
      }

    switch (reg >> 2)
      {
      case 4:
        _guest_features.raw = value;
        break;

      case 10:
        _page_size = value;
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
          _current_q->setup(_iommu->access(Virtio::Ptr<void>(value * _page_size)));
        break;

      case 20:
        kick();
        break;

      case 25:
        break;

      case 28:
        _status.raw = value;
        if (value == 0)
          reset();
        break;
      }
  }
};

}
