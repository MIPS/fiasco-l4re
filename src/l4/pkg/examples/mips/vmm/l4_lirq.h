/*
 * l4_lirq.h - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
#pragma once

#include <l4/re/env>
#include <l4/re/util/cap_alloc>
#include <l4/sys/factory>

#include "l4_exceptions.h"
#include <l4/re/error_helper>
using L4Re::chksys;

class L4_lirq{
protected:
    L4::Cap<L4::Irq> _lirq;

public:
    L4_lirq(){
        _lirq = L4Re::Util::cap_alloc.alloc<L4::Irq>();
        if(!_lirq.is_valid()) throw L4_CAPABILITY_EXCEPTION;
        chksys(L4Re::Env::env()->factory()->create_irq(_lirq));
    }
    ~L4_lirq(){
        L4Re::Util::cap_alloc.free(_lirq, L4Re::This_task);
        _lirq.invalidate();
    }
    L4::Cap<L4::Irq> & lirq() { return _lirq; }
};


