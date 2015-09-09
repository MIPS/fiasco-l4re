/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/*
 * l4_ser.h - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 * (c) 2011 Steffen Liebergeld <steffen@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */

#ifndef L4_SER_H
#define L4_SER_H

#include "devices/device_interface.h"
#include "buffer.h"
#include "l4_lirq.h"
#include "thread.hpp"
#include "debug.h"

class L4_ser : public device::IDevice, protected karma::Thread
{
private:
    L4_lirq _irq;
    L4_lirq _ser_irq;
    unsigned counter;
    Buffer<1024> _read_buf;
    Buffer<1024> _write_buf;

    char str[1024];
    l4_addr_t _shared_mem;
    l4_addr_t _base_ptr;
    // KYMAXXX serial
    l4_umword_t _base_sz;
    l4_addr_t _shared_mem_base;

    enum
    {
        L4_ser_init = 0x0,
        L4_ser_read = 0x4,
        L4_ser_write = 0x8,
        L4_ser_flush = 0xc,
        L4_ser_writln = 0x6,
    };

public:
    L4_ser();

    // KYMAXXX serial
    //void init(unsigned int);
    void init(unsigned int, l4_addr_t base_ptr, l4_umword_t base_sz, l4_addr_t shared_mem_base);
    void get_write_buffer(Buffer<1024> **buffer);
    void get_read_buffer(Buffer<1024> **buffer);
    void flush(void);

    virtual void hypercall(HypercallPayload &);

    // KYMAXXX serial
    l4_addr_t guest_to_shared_mem(l4_addr_t guest_buf_addr);

protected:
    l4_umword_t read(l4_umword_t addr);
    void write(l4_umword_t addr, l4_umword_t val);
    virtual void run(L4::Cap<L4::Thread> & thread);
};
  
#endif // L4_SER_H
