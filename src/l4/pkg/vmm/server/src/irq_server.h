#pragma once

#include <l4/cxx/ipc_server>
#include "gic.h"

struct Irq_svr : L4::Server_object, Gic::Irq_array::Eoi_handler
{
  Irq_svr() : gic(0) {}

  explicit Irq_svr(Gic::Dist *gic, unsigned irq) : gic(gic), irq(irq)
  {
    gic->spi(irq).set_eoi(this);
  }

  void gic_bind(Gic::Dist *gic, unsigned irq)
  {
    this->gic = gic;
    this->irq = irq;
    gic->spi(irq).set_eoi(this);
  }

  int dispatch(l4_umword_t, L4::Ipc::Iostream &)
  {
    gic->inject_spi(irq, vmm_current_cpu_id);
    return 0;
  }

  void eoi()
  {
    L4::cap_reinterpret_cast<L4::Irq>(obj_cap())->unmask();
  }

  Gic::Dist *gic;
  unsigned irq;
};
