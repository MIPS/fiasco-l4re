/*
 * (c) 2014 Alexander Warg <warg@os.inf.tu-dresden.de>
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include "inhibitor_mux.h"
#include <l4/cxx/ipc_server>

namespace Hw { class Root_bus; }

class Platform_control
: public Inhibitor_mux,
  public L4::Server_object
{
public:
  explicit Platform_control(Hw::Root_bus *hw_root)
  : _state(0), _hw_root(hw_root) {}

  void all_inhibitors_free(l4_umword_t id);
  void cancel_op() { _state &= ~Op_in_progress_mask; }

  int dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios);

private:
  enum State_bits
  {
    Suspend_in_progress  = 1,
    Shutdown_in_progress = 2,
    Reboot_in_progress   = 4,
    Op_in_progress_mask  = Suspend_in_progress
                           | Shutdown_in_progress
                           | Reboot_in_progress
  };

  l4_umword_t _state;
  Hw::Root_bus *_hw_root;


  unsigned in_progress_ops() const { return _state & Op_in_progress_mask; }
  int start_operation(unsigned op);
};
