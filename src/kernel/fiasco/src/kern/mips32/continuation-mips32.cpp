/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE[mips32]:

#include "entry_frame.h"
#include "member_offs.h"
#include "types.h"
#include "processor.h"

class Continuation
{
  MEMBER_OFFSET();

private:
  Address _ip;
  Mword   _status;

public:
  Continuation() : _ip(~0UL) {}

  typedef Return_frame User_return_frame;

  bool valid() const
  { return _ip != ~0UL; }

  Address ip() const { return _ip; }
  void ip(Address ip) { _ip = ip; }

  Mword flags(Return_frame const *) const { return _status; }
  void flags(Return_frame *, Mword status) { _status = status; }

  Mword sp(Return_frame const *o) const { return o->usp; }
  void sp(Return_frame *o, Mword sp) { o->usp = sp; }

  void save(Return_frame const *regs)
  {
    _ip  = regs->ip();
    _status = regs->status;
  }

  void activate(Return_frame *regs, void *cont_func)
  {
    save(regs);
    regs->epc = Mword(cont_func);
    regs->status = Cp0_Status::status_eret_to_kern_di(regs->status); // kernel mode disable interrupts
  }

  void set(Return_frame *dst, User_return_frame const *src)
  {
    dst->usp = src->usp;
    _ip = src->epc;
    _status = src->status;
  }

  void get(User_return_frame *dst, Return_frame const *src) const
  {
    dst->usp = src->usp;
    dst->epc = _ip;
    dst->status = _status;
  }

  void clear() { _ip = ~0UL; }

  void restore(Return_frame *regs)
  {
    regs->epc = _ip;
    regs->status = _status;
    clear();
  }

};

