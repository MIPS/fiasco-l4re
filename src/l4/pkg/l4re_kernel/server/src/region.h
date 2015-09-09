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
#include <l4/re/dataspace>
#include <l4/re/util/region_mapping>
#include <l4/cxx/ipc_server>

#include "slab_alloc.h"
#if 0
class Rm_dataspace : public L4::Capability
{
public:
  Rm_dataspace(L4::Capability const &ds) : L4::Capability(ds) {}

  Rm_dataspace() : L4::Capability(L4_INVALID_CAP) {}

  int map(unsigned long offset, unsigned long flags,
          l4_addr_t local_addr, l4_addr_t min, l4_addr_t max) const;
};
#endif

class Region_ops;

typedef L4Re::Util::Region_handler<L4::Cap<L4Re::Dataspace>, Region_ops> Region_handler;

class Region_ops
{
public:
  typedef l4_umword_t Map_result;
  static int map(Region_handler const *h, l4_addr_t addr,
                 L4Re::Util::Region const &r, bool writable,
                 l4_umword_t *result);

  static void unmap(Region_handler const *h, l4_addr_t vaddr,
                    l4_addr_t offs, unsigned long size);

  static void free(Region_handler const *h, l4_addr_t start, unsigned long size);

  static void take(Region_handler const *h);
  static void release(Region_handler const *h);
};


class Region_map
: public L4Re::Util::Region_map<Region_handler, Slab_alloc>
{
private:
  typedef L4Re::Util::Region_map<Region_handler, Slab_alloc> Base;

public:
  Region_map();
  //void setup_wait(L4::Ipc::Istream &istr);
  int handle_pagefault(L4::Ipc::Iostream &ios);
  int handle_rm_request(L4::Ipc::Iostream &ios);
  virtual ~Region_map() {}

  void init();

  void debug_dump(unsigned long function) const;
private:
  int reply_err(L4::Ipc::Iostream &ios);
};


