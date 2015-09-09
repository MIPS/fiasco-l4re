/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <l4/sys/types.h>
#include <l4/re/mem_alloc>
#include <l4/re/util/bitmap_cap_alloc>
#include <l4/re/env>
#include <l4/re/l4aux.h>

class Region_map;

namespace Global
{
  enum
  {
    Max_local_rm_caps = 1024,
    Local_task_cap    = 0,
  };
  typedef L4Re::Util::Cap_alloc<Max_local_rm_caps>  Cap_alloc;
  extern Region_map *local_rm;
  extern L4::Cap<L4Re::Mem_alloc> allocator;
  extern Cap_alloc cap_alloc;
  extern char const *const *argv;
  extern char const *const *envp;
  extern int argc;
  extern l4re_aux_t *l4re_aux;
};
