IMPLEMENTATION:

#include <climits>
#include <cstring>
#include <cstdio>

#include "jdb.h"
#include "jdb_core.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_kobject.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "factory.h"
#include "simpleio.h"
#include "static_init.h"

class Jdb_factory : public Jdb_kobject_handler
{
public:
  Jdb_factory() FIASCO_INIT;
};

IMPLEMENT
Jdb_factory::Jdb_factory()
  : Jdb_kobject_handler(Factory::static_kobj_type)
{
  Jdb_kobject::module()->register_handler(this);
}

PUBLIC
bool
Jdb_factory::show_kobject(Kobject_common *, int )
{
  return true;
}

PUBLIC
char const *
Jdb_factory::kobject_type() const
{
  return JDB_ANSI_COLOR(yellow) "Factory" JDB_ANSI_COLOR(default);
}

PUBLIC
void
Jdb_factory::show_kobject_short(String_buffer *buf, Kobject_common *o)
{
  Factory *t = Kobject::dcast<Factory*>(o);
  buf->printf(" c=%ld l=%ld", t->current(), t->limit());
}

static Jdb_factory jdb_factory INIT_PRIORITY(JDB_MODULE_INIT_PRIO);
