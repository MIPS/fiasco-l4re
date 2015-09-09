/*
 * (c) 2012-2013 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include "vcon_fe_base.h"
#include <l4/re/error_helper>

Vcon_fe_base::Vcon_fe_base(L4::Cap<L4::Vcon> con,
                           L4Re::Util::Object_registry *r)
: _vcon(con)
{
  r->register_irq_obj(this);
}

int
Vcon_fe_base::do_write(char const *buf, unsigned sz)
{
  for (unsigned s = sz; s; )
    {
      int l = _vcon->write(buf, s);
      if (l < 0)
        break;
      s -= l;
      buf += l;
    }
  return sz;
}

void
Vcon_fe_base::handle_pending_input()
{
  char data[8];
  while (_vcon->read(data, sizeof(data)) > (int)sizeof(data))
    ;
}

int
Vcon_fe_base::dispatch(l4_umword_t, L4::Ipc::Iostream &ios)
{
  l4_msgtag_t tag;
  ios >> tag;

  switch (tag.label())
    {
    case 0:
        {
          char buf[30];
          const int sz = sizeof(buf);
          int r;

          do
            {
              r = _vcon->read(buf, sz);
              if (_input && r > 0)
                _input->input(cxx::String(buf, r > sz ? sz : r));
            }
          while (r > sz);
        }
      return L4_EOK;
    default:
      return -L4_EBADPROTO;
    }
}
