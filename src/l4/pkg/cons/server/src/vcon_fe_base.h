/*
 * (c) 2012-2013 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include "frontend.h"

#include <l4/sys/vcon>
#include <l4/re/util/object_registry>
#include <l4/cxx/ipc_server>

class Vcon_fe_base : public Frontend, public L4::Server_object
{
public:
  Vcon_fe_base(L4::Cap<L4::Vcon> con, L4Re::Util::Object_registry *r);
  int dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios);

protected:
  int do_write(char const *buf, unsigned sz);
  bool check_input() { return _vcon->read(0, 0) > 0; }
  void handle_pending_input();

  L4::Cap<L4::Vcon> _vcon; // FIXME: could be an auto cap
};
