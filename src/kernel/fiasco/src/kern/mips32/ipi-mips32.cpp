/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mp]:

#include "cpu.h"
#include "pic.h"
#include "gic.h"
#include "processor.h"

#define IPI_DEBUG 0

EXTENSION class Ipi
{
public:
  enum Message
  {
    Global_request = 0,
    Request,
    Debug,
    Ipi_last,
  };
private:
  Cpu_phys_id _phys_id;

  struct _xlate_ipi_to_irq {
    int irq;
  } _xlate[Ipi_last];

  bool _init_done;
};


PUBLIC inline
Ipi::Ipi() : _phys_id(Cpu_phys_id(~0)), _init_done(false)
{}

IMPLEMENT
void
Ipi::init(Cpu_number cpu)
{
  struct _xlate_ipi_to_irq *xlate = _ipi.cpu(cpu)._xlate;
  Unsigned32 cpu_val = Cpu_number::val(cpu);
  unsigned gic_base = Pic::gic_irq_base();

  _ipi.cpu(cpu)._phys_id = Proc::cpu_id();

  if (!Pic::gic_present())
    return;

  xlate[Global_request].irq =
    Pic::gic->plat_ipi_globalrequest_int_xlate(cpu_val) + gic_base;
  xlate[Request].irq =
    Pic::gic->plat_ipi_request_int_xlate(cpu_val) + gic_base;
  xlate[Debug].irq =
    Pic::gic->plat_ipi_debug_int_xlate(cpu_val) + gic_base;

#if IPI_DEBUG
  for (int mm = Global_request; mm < Ipi_last; mm++) {
    printf("Ipi::init m %d cpu %d irq %d\n", mm, Cpu_number::val(cpu),
        _ipi.cpu(cpu)._xlate[mm].irq);
  }
#endif
  _ipi.cpu(cpu)._init_done = true;
}

PUBLIC static
void Ipi::ipi_call_debug_arch()
{}

PUBLIC static inline
int Ipi::irq_xlate(Message m, Cpu_number on_cpu)
{
  assert (m < Ipi::Ipi_last);
  assert (_ipi.cpu(on_cpu)._init_done);

  return _ipi.cpu(on_cpu)._xlate[m].irq;
}

PUBLIC static inline
void Ipi::eoi(Message m, Cpu_number on_cpu)
{
  int irq = irq_xlate(m, on_cpu);

  Pic::gic->eoi(irq);
  stat_received(on_cpu);
}

PUBLIC static inline NEEDS["pic.h"]
void Ipi::send(Message m, Cpu_number from_cpu, Cpu_number to_cpu)
{
  assert (cpu_lock.test());

  int irq = irq_xlate(m, to_cpu);

#if IPI_DEBUG
  printf("Ipi::send m %d irq %d from_cpu %d to_cpu %d\n", m, irq,
        Cpu_number::val(from_cpu),
        Cpu_number::val(to_cpu));
#endif

  Pic::gic->gic_send_ipi(irq);
  stat_sent(from_cpu);
}

PUBLIC static inline
void
Ipi::bcast(Message m, Cpu_number from_cpu)
{
  for (Cpu_number to_cpu = Cpu_number::first(); to_cpu < Config::max_num_cpus(); ++to_cpu)
    {
      if (Cpu::online(to_cpu))
        Ipi::send(m, from_cpu, to_cpu);
    }
}
