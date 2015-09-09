/*
 * (c) 2008-2010 Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <l4/re/log>
#include <l4/re/env>
#include <l4/re/rm>
#include <l4/sys/scheduler>
#include <l4/re/util/object_registry>
#include <l4/re/util/cap_alloc>

#include <l4/cxx/ipc_server>
#include "server.h"

class App_task : public Ned::Server_object
{
private:
  long _ref_cnt;

public:
  enum State { Initializing, Running, Zombie };

  long remove_ref() { return --_ref_cnt; }
  void add_ref() { ++_ref_cnt; }

  long ref_cnt() const { return _ref_cnt; }

private:
  // hm, missing template typedefs
  template< typename T >
  struct Auto_del_cap : public L4Re::Util::Auto_del_cap<T> {};

  // hm, missing template typedefs
  template< typename T >
  struct Auto_cap : public L4Re::Util::Auto_cap<T> {};

  Ned::Registry *_r;

  Auto_del_cap<L4::Task>::Cap _task;
  Auto_del_cap<L4::Thread>::Cap _thread;
  Auto_del_cap<L4Re::Rm>::Cap _rm;

  State _state;
  unsigned long _exit_code;
  l4_cap_idx_t _observer;

public:
  State state() const { return _state; }
  unsigned long exit_code() const { return _exit_code; }
  l4_cap_idx_t observer() const { return _observer; }
  void observer(l4_cap_idx_t o) { _observer = o; }
  void running()
  {
    _state = Running;
    add_ref();
  }


  App_task(Ned::Registry *r, L4::Cap<L4::Factory> alloc);

  //L4::Cap<L4Re::Mem_alloc> allocator() const { return _ma; }

  int dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios);

  L4::Cap<L4Re::Rm> rm() { return _rm.get(); }
  L4::Cap<L4::Task> task_cap() const { return _task.get(); }
  L4::Cap<L4::Thread> thread_cap() const { return _thread.get(); }

  virtual void terminate();

  virtual ~App_task();
};
