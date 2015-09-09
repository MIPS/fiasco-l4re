/*
 * (c) 2014 Alexander Warg <warg@os.inf.tu-dresden.de>
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "hw_root_bus.h"
#include "platform_control.h"
#include "server.h"
#include "debug.h"

#include <l4/cxx/static_container>
#include <l4/sys/platform_control>


namespace {
struct Suspend_timeout : L4::Ipc_svr::Timeout_queue::Timeout
{
  Platform_control *ctl;
  explicit Suspend_timeout(Platform_control *ctl) : ctl(ctl) {}

  void expired()
  {
    d_printf(DBG_ERR, "error: timeout during suspend, abort suspend\n");
    ctl->print_lock_holders(L4VBUS_INHIBITOR_SUSPEND);
    ctl->inhibitor_signal(L4VBUS_INHIBITOR_WAKEUP);
    ctl->cancel_op();
  }
};

struct Shutdown_timeout : L4::Ipc_svr::Timeout_queue::Timeout
{
  Platform_control *ctl;
  explicit Shutdown_timeout(Platform_control *ctl) : ctl(ctl) {}

  void expired()
  {
    d_printf(DBG_ERR,
             "error: timeout during shutdown / reboot, force operation\n");
    ctl->print_lock_holders(L4VBUS_INHIBITOR_SHUTDOWN);
    // this will do a reboot if the operation in progress is reboot
    ctl->all_inhibitors_free(L4VBUS_INHIBITOR_SHUTDOWN);
    ctl->cancel_op();
  }
};

union Timeouts
{
  cxx::Static_container<Suspend_timeout> suspend;
  cxx::Static_container<Shutdown_timeout> shutdown;
} op_timeout;

}

int
Platform_control::start_operation(unsigned op)
{
  using L4::Ipc_svr::Timeout_queue;

  if (in_progress_ops())
    return -L4_EBUSY;

  unsigned inhibitor;
  Timeout_queue::Timeout *timeout;
  switch (op)
    {
    case Suspend_in_progress:
      inhibitor = L4VBUS_INHIBITOR_SUSPEND;
      op_timeout.suspend.construct(this);
      timeout   = op_timeout.suspend;
      break;

    case Shutdown_in_progress: /* fall through */
    case Reboot_in_progress:
      inhibitor = L4VBUS_INHIBITOR_SHUTDOWN;
      op_timeout.shutdown.construct(this);
      timeout   = op_timeout.shutdown;
      break;

    default:
      return -L4_EINVAL;
    }

  _state |= op;
  if (inhibitors_free(inhibitor))
    {
      all_inhibitors_free(inhibitor);
      return L4_EOK;
    }
  inhibitor_signal(inhibitor);
  timeouts->add(timeout, l4_kip_clock(l4re_kip()) + 10000000);
  return L4_EOK;
}

void
Platform_control::all_inhibitors_free(l4_umword_t id)
{
  if (!_hw_root->supports_pm())
    return;

  unsigned in_progress = in_progress_ops();

  if (!in_progress)
    return;

  switch (id)
    {
    default:
      return;

    case L4VBUS_INHIBITOR_SUSPEND:
      if (in_progress & Suspend_in_progress)
        {
          timeouts->remove(op_timeout.suspend);
          _hw_root->suspend();
          _state &= ~Suspend_in_progress;
          inhibitor_signal(L4VBUS_INHIBITOR_WAKEUP);
        }
      return;

    case L4VBUS_INHIBITOR_SHUTDOWN:
      // reboot overrides shutdown (HMM: this is policy)
      timeouts->remove(op_timeout.shutdown);
      if (in_progress & Reboot_in_progress)
        {
          _hw_root->reboot();
          d_printf(DBG_ERR, "fatal: platform reboot returned\n");
          exit(255);
        }

      if (in_progress & Shutdown_in_progress)
        {
          _hw_root->shutdown();
          d_printf(DBG_ERR, "fatal: platform shutdown returned\n");
          exit(255);
        }
      return;
    }
}

int
Platform_control::dispatch(l4_umword_t, L4::Ipc::Iostream &ios)
{
  L4::Opcode op;
  ios >> op;
  switch (op)
    {
    default:
      return -L4_ENOSYS;

    case L4::Platform_control::Suspend:
      return start_operation(Suspend_in_progress);
    case L4::Platform_control::Shutdown:
      if (L4::Ipc::read<l4_umword_t>(ios))
        return start_operation(Reboot_in_progress);
      return start_operation(Shutdown_in_progress);
    }
}
