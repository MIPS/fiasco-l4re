#include "irqs.h"
#include "main.h"

int
Kernel_irq_pin::unbind()
{
  int err = l4_error(system_icu()->icu->unbind(_idx, irq()));
  set_shareable(false);
  return err;
}

int
Kernel_irq_pin::bind(L4::Cap<L4::Irq> irq, unsigned mode)
{
  if (mode != L4_IRQ_F_NONE)
    {
      //printf(" IRQ[%x]: mode=%x ... ", n, mode);
      int err = l4_error(system_icu()->icu->set_mode(_idx, mode));
      //printf("result=%d\n", err);

      if (err < 0)
        return err;
    }

  int err = l4_error(system_icu()->icu->bind(_idx, irq));

  // allow sharing if IRQ must be acknowledged via the IRQ object 
  if (err == 0)
    set_shareable(true);

  if (err < 0)
    return err;

  return err;
}

int
Kernel_irq_pin::unmask()
{
  system_icu()->icu->unmask(_idx);
  return -L4_EINVAL;
}


int
Kernel_irq_pin::set_mode(unsigned mode)
{
  return l4_error(system_icu()->icu->set_mode(_idx, mode));
}

int
Io_irq_pin::clear()
{
  int cnt = 0;
  while (!l4_error(l4_ipc_receive(irq().cap(), l4_utcb(), L4_IPC_BOTH_TIMEOUT_0)))
    ++cnt;

  return cnt;
}

int
Kernel_irq_pin::mask()
{
  return l4_error(system_icu()->icu->mask(_idx));
}

