/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
#include <l4/re/event>
#include <l4/re/event-sys.h>
#include <l4/re/dataspace>
#include <l4/re/protocols>

#include <l4/sys/err.h>

#include <l4/cxx/ipc_stream>

namespace L4Re
{

using L4::Opcode;

long
Event::get_buffer(L4::Cap<Dataspace> ds) const throw()
{
  L4::Ipc::Iostream io(l4_utcb());
  io << Opcode(Event_::Get);
  io << L4::Ipc::Small_buf(ds.cap());
  return l4_error(io.call(cap(), L4Re::Protocol::Event));
}

long
Event::get_num_streams() const throw()
{
  L4::Ipc::Iostream io(l4_utcb());
  io << Opcode(Event_::Get_num_streams);
  return l4_error(io.call(cap(), L4Re::Protocol::Event));
}

long
Event::get_stream_info(int idx, Event_stream_info *info) const throw()
{
  L4::Ipc::Iostream io(l4_utcb());
  io << Opcode(Event_::Get_stream_info) << idx;
  long res = l4_error(io.call(cap(), L4Re::Protocol::Event));
  if (res < 0)
    return res;

  io.get(*info);
  return 0;
}

long
Event::get_stream_info_for_id(l4_umword_t id, Event_stream_info *info) const throw()
{
  L4::Ipc::Iostream io(l4_utcb());
  io << Opcode(Event_::Get_stream_info_for_id) << id;
  long res = l4_error(io.call(cap(), L4Re::Protocol::Event));
  if (res < 0)
    return res;

  io.get(*info);
  return 0;
}

long
Event::get_axis_info(l4_umword_t id, unsigned naxes, unsigned *axis,
                     Event_absinfo *info) const throw()
{
  L4::Ipc::Iostream io(l4_utcb());
  io << Opcode(Event_::Get_axis_info) << id << L4::Ipc::buf_cp_out(axis, naxes);
  long res = l4_error(io.call(cap(), L4Re::Protocol::Event));
  if (res < 0)
    return res;

  long unsigned a = naxes;
  io >> L4::Ipc::buf_cp_in(info, a);
  return 0;
}

long
Event::get_stream_state_for_id(l4_umword_t stream_id, Event_stream_state *state) const throw()
{
  L4::Ipc::Iostream io(l4_utcb());
  io << Opcode(Event_::Get_stream_state_for_id) << stream_id;
  long res = l4_error(io.call(cap(), L4Re::Protocol::Event));
  if (res < 0)
    return res;

  io.get(*state);
  return 0;
}

}
