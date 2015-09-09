/*
 * Example for mapping a capability from a client to a server - client part.
 *
 * (c) 2014 Steffen Liebergeld <steffen.liebergeld@kernkonzept.com>
 *
 * This file is licensed under the terms of the GNU General Public License 2.
 * See file COPYING-GPL-2 for details.
 */
#include <l4/sys/factory>
#include <l4/sys/irq>
#include <l4/sys/ipc_gate>
#include <l4/cxx/ipc_stream>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/util/cap_alloc>

#include <cstdio>

#include "shared.h"

enum { Nr_of_triggers = 3 };

static int run()
{
  using L4Re::chksys;
  using L4Re::chkcap;

  L4::Cap<L4::Irq> irq;
  L4::Cap<L4::Ipc_gate> server;

  printf("Hello from ex_map_irq_client.\n");

  // allocate cap for IRQ
  irq = chkcap(L4Re::Util::cap_alloc.alloc<L4::Irq>(),
               "could not find a free cap slot");
  // create IRQ kernel object
  chksys(L4Re::Env::env()->factory()->create_irq(irq),
         "could not create a new IRQ kernel object");

  // look out for server
  server = chkcap(L4Re::Env::env()->get_cap<L4::Ipc_gate>("ex_map_irq"),
                  "could not find 'ex_map_irq' in namespace");

  // map irq to server
  printf("Mapping IRQ cap to server.\n");
  L4::Ipc::Iostream s(l4_utcb());
  s << L4::Opcode(Map_irq_opcodes::Map_irq) << irq;
  chksys(s.call(server.cap()), "request failed");

  // tell the server to start triggering us and how many times it should
  // trigger the IRQ
  s.reset();
  s << L4::Opcode(Map_irq_opcodes::Start) << Nr_of_triggers;
  chksys(s.call(server.cap()), "request failed");

  // attach to IRQ and wait for the server to trigger it
  chksys(irq->attach(0, L4Re::Env::env()->main_thread()),
         "could not attach to IRQ");

  for (int i = 0; i < Nr_of_triggers; ++i)
    {
      chksys(irq->receive(), "receive failed");
      printf("Received IRQ.\n");
    }

  printf("ex_map_irq_client finished.\n");
  return 0;
}

int main()
{
  try
    {
      return run();
    }
  catch (L4::Runtime_error &e)
    {
      printf("Runtime error: %s.\n", e.str());
    }
  catch (...)
    {
      printf("Uncaught error.\n");
    }
  return 1;
}

