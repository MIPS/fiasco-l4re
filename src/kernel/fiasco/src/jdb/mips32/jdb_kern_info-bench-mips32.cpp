IMPLEMENTATION[mips32]:

IMPLEMENT
void
Jdb_kern_info_bench::show_arch()
{}

#include "kip.h"

IMPLEMENT inline NEEDS["kip.h"]
Unsigned64
Jdb_kern_info_bench::get_time_now()
{ return Kip::k()->clock; }
