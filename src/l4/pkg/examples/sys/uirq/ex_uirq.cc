/*
 * (c) 2014 Steffen Liebergeld <steffen.liebergeld@kernkonzept.com>
 *
 * This file is licensed under the terms of the GNU General Public License 2.
 * See file COPYING-GPL-2 for details.
 */

/*
 * This example shows how to create a user IRQ and how to receive and trigger
 * them. Triggering and receiving is done in different threads, created with
 * C++11's std::thread.
 */
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>
#include <l4/sys/irq>
#include <l4/sys/factory>

#include <pthread-l4.h>
#include <thread>

#include <cstdio>
#include <unistd.h>

static void thread_fn(L4::Cap<L4::Irq> irq, unsigned return_after)
{
  printf("Hello from receiver thread.\n");

  try
    {
      L4::Cap<L4::Thread> t;
      // As there are no means to get the equivalent with std::thread, we
      // use pthread_self() to get this thread
      t = L4::Cap<L4::Thread>(pthread_getl4cap(pthread_self()));

      // Attach to ourselves to the IRQ to receive its triggers
      L4Re::chksys(irq->attach(0, t), "Could not attach to IRQ.");

      // Now wait for triggers
      while (return_after--)
        {
          L4Re::chksys(irq->receive(), "Receive failed.");
          printf("Received irq!\n");
        }
    }
  catch (L4::Runtime_error &e)
    {
      fprintf(stderr, "Runtime error: %s.\n", e.str());
    }
  catch (...)
    {
      fprintf(stderr, "Unknown exception.\n");
    }
}

int main()
{
  try
    {
      printf("IRQ triggering example.\n");

      L4::Cap<L4::Irq> irq;

      // Allocate a capability to use for the IRQ object
      irq = L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Irq>());

      // Create the IRQ object with the capability, using our default
      // factory
      L4Re::chksys(L4Re::Env::env()->factory()->create_irq(irq),
                   "Failed to create IRQ.");

      enum { Loops = 3 };

      // Create a thread and also tell it about our IRQ capability
      std::thread thread = std::thread([irq](){ thread_fn(irq, Loops); });

      // Trigger the IRQ a couple of times
      for (int i = 0; i < Loops; i++)
        {
          irq->trigger();
          sleep(1);
        }

      // 'Wait' for thread to finish
      thread.join();

      printf("Example finished.\n");
      return 0;
    }
  catch (L4::Runtime_error &e)
    {
      fprintf(stderr, "Runtime error: %s.\n", e.str());
    }
  catch (...)
    {
      fprintf(stderr, "Unknown exception.\n");
    }

  return 1;
}
