#pragma once

#include <l4/re/dataspace>
#include <l4/util/util.h>

#include "arm_mmio_device.h"

class Ds_handler : public Vmm::Mmio_device
{
  L4::Cap<L4Re::Dataspace> _ds;
  l4_addr_t _offset;
  bool access(l4_addr_t pfa, l4_addr_t offset, l4_vcpu_state_t *vcpu,
              L4::Cap<L4::Task> vm_task, l4_addr_t min, l4_addr_t max)
  {
    Vmm::Arm::Hsr hsr(vcpu->r.err);
    long res;
#ifdef MAP_OTHER
    res = _ds->map(offset + _offset,
                   hsr.pf_write() ? L4Re::Dataspace::Map_rw : 0,
                   pfa, min, max, vm_task);
#else
    (void)vm_task;
    unsigned char ps = L4_PAGESHIFT;

    if (l4_trunc_size(pfa, L4_SUPERPAGESHIFT) >= min
        && l4_round_size(pfa, L4_SUPERPAGESHIFT) <= max)
      ps = L4_SUPERPAGESHIFT;

    res = L4Re::chksys(vm_task->map(L4Re::This_task,
                                    l4_fpage(l4_trunc_size(_local_start + offset, ps),
                                             ps,
                                             hsr.pf_write()
                                               ? L4_FPAGE_RWX : L4_FPAGE_RX),
                                    l4_trunc_size(pfa, ps)));
#endif

    if (res < 0)
      {
        Err().printf("cannot handle VM memory access @ %lx ip=%lx r=%ld\n",
                     pfa, vcpu->r.ip, res);
        l4_sleep_forever();
      }
    return true;
  }

#ifndef MAP_OTHER
  l4_addr_t _local_start;
#endif

public:
  explicit Ds_handler(L4::Cap<L4Re::Dataspace> ds,
                      l4_addr_t local_start,
                      l4_size_t size = 0,
                      l4_addr_t offset = 0)
    : _ds(ds), _offset(offset)
#ifndef MAP_OTHER
      , _local_start(local_start)
#endif
  {
#ifndef MAP_OTHER
    if (local_start == 0)
      L4Re::chksys(L4Re::Env::env()->rm()->attach(&_local_start,
                                                  size
                                                    ? size
                                                    : L4Re::chksys(ds->size()),
                                                  L4Re::Rm::Search_addr
                                                  | L4Re::Rm::Eager_map,
                                                  ds, offset, L4_SUPERPAGESHIFT));
#endif
  }
};
