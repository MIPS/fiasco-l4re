/*
 * Example for mapping a capability from a client to a server - server part.
 *
 * (c) 2014 Steffen Liebergeld <steffen.liebergeld@kernkonzept.com>
 *
 * This file is licensed under the terms of the GNU General Public License 2.
 * See file COPYING-GPL-2 for details.
 */
#include <l4/re/error_helper>
#include <l4/cxx/ipc_server>
#include <l4/sys/irq>
#include <l4/re/env>
#include <l4/sys/factory>
#include <l4/re/util/object_registry>

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>

#include "shared.h"

// Those are error helpers that simplify our code by hiding error checking.
// Errors flagged through C++ exceptions.
using L4Re::chksys;
using L4Re::chkcap;

// This capability slot will be used as receive window.
L4::Cap<void> rcv_cap;

// This class reacts on notifications from Server
class Trigger : public L4::Server_object
{
public:
  int dispatch(l4_umword_t, L4::Ipc::Iostream &);

  void num_triggers(unsigned num);
  void irq(L4::Cap<L4::Irq> irq);

  void trigger() { _irq->trigger(); }

private:
  L4::Cap<L4::Irq> _irq;
  unsigned _nrs;
};

void Trigger::num_triggers(unsigned num)
{
  _nrs = num;
}

void Trigger::irq(L4::Cap<L4::Irq> irq)
{
  _irq = irq;
}

int Trigger::dispatch(l4_umword_t, L4::Ipc::Iostream &)
{
  for (unsigned i = 0; i < _nrs; ++i)
    {
      printf("Triggering IRQ.\n");
      _irq->trigger();
      sleep(1);
    }
  printf("ex_map_irq_server finished.\n");
  exit(0);
}

// This class handles the reception of the IRQ cap from the client.
class Server : public L4::Server_object
{
public:
  Server(L4::Cap<L4::Irq> irq, Trigger *trigger)
  : L4::Server_object(), _trigger_notification(irq), _trigger(trigger)
  {}

  int dispatch(l4_umword_t, L4::Ipc::Iostream &ios);

private:
  void do_client_trigger(unsigned);

  L4::Cap<L4::Irq> _trigger_notification;
  Trigger *_trigger;
};

int Server::dispatch(l4_umword_t /*obj*/, L4::Ipc::Iostream &ios)
{
  l4_msgtag_t tag;
  ios >> tag;

  if (tag.label() != 0)
    return -L4_EBADPROTO;

  L4::Opcode op;
  ios >> op;

  switch (op)
    {
    case Map_irq_opcodes::Map_irq:
        {
          L4::Ipc::Snd_fpage cap;
          ios >> cap;
          if (cap.cap_received())
            {
              // We use rcv_cap for receiving only.
              // After receiving the cap, we tell the kernel to associate
              // the kernel object that rcv_cap points to with irq.

              // allocate new cap
              L4::Cap<void> tmp =
                L4Re::chkcap(L4Re::Util::cap_alloc.alloc<void>(),
                    "Could not find a free cap slot");

              // tell the kernel to associate IRQ object with another cap
              tmp.move(rcv_cap);
              if (!tmp.is_valid())
                {
                  printf("Error receiving the IRQ.\n");
                  return -L4_EINVAL;
                }

              _trigger->irq(L4::cap_cast<L4::Irq>(tmp));
            }
          printf("Received IRQ from client.\n");
          return L4_EOK;
        }
    case Map_irq_opcodes::Start:
        {
          unsigned nrs;
          ios >> nrs;
          _trigger->num_triggers(nrs);
          _trigger_notification->trigger();
          return L4_EOK;
        }

    default:
      break;
    }
  return -L4_ENOSYS;
}


// Receiving a capability requires a receive flexpage that contains a free
// capability slot on the receiving side. In C++ such a flexpage is set up by
// putting an L4::Ipc::Small_buf with the target capability slot (must be a
// valid cap) into the input stream before the map IPC is done. To do so in a
// L4Re::Util::registry server, we supply a custom Loop_hooks class.
class Loop_hooks :
  public L4::Ipc_svr::Ignore_errors,
  public L4::Ipc_svr::Default_timeout,
  public L4::Ipc_svr::Compound_reply
{
public:
  void setup_wait(L4::Ipc::Istream &istr, L4::Ipc_svr::Reply_mode)
  {
    istr.reset();
    istr << L4::Ipc::Small_buf(rcv_cap.cap(), 0);
    l4_utcb_br_u(istr.utcb())->bdr = 0;
  }
};

static L4Re::Util::Registry_server<Loop_hooks> server;

static int run()
{
  printf("Hello from ex_map_irq_server.\n");

  // allocate a cap slot that will be used as the receive cap
  rcv_cap = chkcap(L4Re::Util::cap_alloc.alloc<void>(),
                   "no free cap slot for receive window");

  Trigger trigger;
  L4::Cap<L4::Irq> notification_irq;

  notification_irq = chkcap(server.registry()->register_irq_obj(&trigger),
                            "could not register notification trigger");

  Server map_irq_srv(notification_irq, &trigger);

  L4Re::chkcap(server.registry()->register_obj(&map_irq_srv, "ex_map_irq"),
               "could not register service side");

  server.loop();
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
