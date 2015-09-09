/*
 * (c) 2012-2013 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include "registry.h"

#include <cstdio>
#include <l4/re/error_helper>

namespace {
  class Trash : public L4::Server_object
  {
    int dispatch(l4_umword_t, L4::Ipc::Iostream &)
    {
      printf("GOT: stale request, drop it\n");
      return -L4_EINVAL;
    }
  };

  static Trash _trash;

  class Del_handler : public L4::Server_object
  {
  private:
    Registry *_r;

  public:
    explicit Del_handler(Registry *r) : _r(r) {}

    int dispatch(l4_umword_t, L4::Ipc::Iostream &)
    {
      _r->gc_step();
      return -L4_ENOREPLY;
    }
  };
}

Registry::Registry()
{
  using L4Re::chkcap;
  L4::Cap<L4::Irq> _del_irq = chkcap(register_irq_obj(new Del_handler(this)));
  _server->register_del_irq(_del_irq);
};

L4::Cap<void>
Registry::register_obj(Server_object *o, char const *service)
{
  using L4::Kobject;
  using L4::cap_cast;
  using L4::Cap;

  Cap<Kobject> r
    = cap_cast<Kobject>(L4Re::Util::Object_registry::register_obj(o, service));
  if (!r)
    return r;

  _life.add(o);
  r->dec_refcnt(1);

  return r;
}

L4::Cap<void>
Registry::register_obj(Server_object *o)
{
  using L4::Kobject;
  using L4::cap_cast;
  using L4::Cap;

  Cap<Kobject> r = cap_cast<Kobject>(L4Re::Util::Object_registry::register_obj(o));
  if (!r)
    return r;

  _life.add(o);
  r->dec_refcnt(1);
  return r;
}

void
Registry::gc_step()
{
  if (0)
    printf("GC: step this=%p _life = %p\n", this, *_life.begin());
  for (Obj_iter n = _life.begin(); n != _life.end(); )
    {
      if (!n->obj_cap() || !n->obj_cap().validate().label())
        {
          if (0)
            printf("GC: object=%p\n", *n);
          L4::Thread::Modify_senders todo;
          todo.add(~3UL, reinterpret_cast<l4_umword_t>((L4::Server_object*)*n),
                   ~0UL, reinterpret_cast<l4_umword_t>((L4::Server_object*)&_trash));
          L4::Cap<L4::Thread>()->modify_senders(todo);
          Server_object *o = *n;
          n = _life.erase(n);
          if (o->collected())
            delete o;
        }
      else
        ++n;
    }
}

void
Registry::gc_sweep()
{
}
