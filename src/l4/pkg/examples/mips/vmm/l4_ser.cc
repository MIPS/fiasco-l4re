/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/*
 * l4_ser.cc - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 * (c) 2011 Steffen Liebergeld <steffen@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */

#include <l4/re/env>
#include <l4/re/namespace>
#include <l4/re/util/cap_alloc>

#include <l4/sys/types.h>
#include <l4/sys/irq>
#include <l4/sys/icu>
#include <l4/sys/vcon>
#include <l4/sys/err.h>

#include <string.h>


#include "l4_ser.h"
#include "mipsregs.h"
// KYMAXXX serial
#include "debug.h"
//#include "l4_vm.h"
//#include "l4_gic.h"

L4_ser::L4_ser()
        : _shared_mem(0) {}

#if 1 // KYMAXXX serial
#undef L4SER_READ_IMPLEMENTED
void L4_ser::init(unsigned int irq, l4_addr_t base_ptr, l4_umword_t base_sz, l4_addr_t shared_mem_base) {
    (void)irq;
    _base_ptr = base_ptr;
    _base_sz = base_sz;
    _shared_mem_base = shared_mem_base;
#ifdef L4SER_READ_IMPLEMENTED
    //GET_VM.gic().attach(_ser_irq.lirq(), irq);
    start();
#endif
}
#else
void L4_ser::init(unsigned int irq){
    _base_ptr = GET_VM.mem().base_ptr();
    GET_VM.gic().attach(_ser_irq.lirq(), irq);
    start();
}
#endif

void
L4_ser::get_write_buffer(Buffer<1024> **buffer)
{
    *buffer = &_write_buf;
}

void
L4_ser::get_read_buffer(Buffer<1024> **buffer)
{
    *buffer = &_read_buf;
}

void
L4_ser::flush(void)
{
   strncpy(str, _write_buf.ptr(), _write_buf.count());
   str[_write_buf.count()] = 0;
   printf("%s", str);
   fflush(0);
}

void L4_ser::hypercall(HypercallPayload & payload){
    if(payload.address() & 1){
        write(payload.address() & ~1L, payload.reg(0));
    } else {
        payload.reg(0) = read(payload.address());
    }
}

l4_umword_t L4_ser::read(l4_umword_t addr)
{
    counter++;
    karma_log(DEBUG, "L4_ser: read(addr=%lx)\n", addr);
    switch (addr)
    {
        case L4_ser_read:
            char c = 0;
            if (_read_buf.count())
            {
                if (_read_buf.get(&c))
                {
                    if(c)
                    {
                    karma_log(DEBUG, "L4_ser: READ: \"%c\"\n", c);
                    return c;
                    }
                }
                else
                    return -1;
            }
    }

    return ~0UL;
}

#define VADDR_TO_OFFSET(base,addr,offset) ((base) + MIPS_KSEG0_TO_PHYS((addr)) - (offset))

l4_addr_t L4_ser::guest_to_shared_mem(l4_addr_t guest_buf_addr)
{
  guest_buf_addr = VADDR_TO_OFFSET(_base_ptr, guest_buf_addr, _shared_mem_base);

  if (guest_buf_addr & (sizeof(l4_addr_t)-1))
    return 0;

  if ((guest_buf_addr < _base_ptr) || (guest_buf_addr >= (_base_ptr + _base_sz)))
    return 0;

  return guest_buf_addr;
}

void L4_ser::write(l4_umword_t addr, l4_umword_t val)
{
    counter++;
    karma_log(DEBUG, "L4_ser: write %lx %lx\n", addr, val);

    switch (addr)
    {
        case L4_ser_init:
            karma_log(DEBUG, "L4_ser: init\n");
            _shared_mem = guest_to_shared_mem(val);
            karma_log(DEBUG, "L4_ser: _shared_mem = %lx\n", _shared_mem);
            break;
        case L4_ser_write:
            karma_log(DEBUG, "L4_ser: write\n");
            _write_buf.put(val);
            break;
        case L4_ser_writln:
            karma_log(DEBUG, "L4_ser: writln val=%ld:\n", val);
            if(!_shared_mem)
                break;
            for(unsigned int i= 0;i<val; i++)
                _write_buf.put(*(char*)(_shared_mem+i));
            break;
        case L4_ser_flush:
            karma_log(DEBUG, "L4_ser: flush\n");
            strncpy(str, _write_buf.ptr(), _write_buf.count());
            str[_write_buf.count()] = 0;
            printf("%s", str);
            fflush(0);
            _write_buf.flush();
            break;
    }
}

void L4_ser::run(L4::Cap<L4::Thread> & thread)
{
#ifndef L4SER_READ_IMPLEMENTED
    (void)thread;
#else
    l4_debugger_set_object_name(pthread_getl4cap(pthread_self()), "L4 SER IRQ");
    char *buf = (char*)malloc(1024);
    karma_log(DEBUG, "Hello from L4_ser irq thread!\n");
    long result;

    result = l4_error(L4Re::Env::env()->log()->bind(0, _irq.lirq()));
    if (result)
    {
        karma_log(WARN, "L4_ser: Did not get the irq capability. Error: %s\n",
            l4sys_errtostr(result));
    } else {
        karma_log(INFO, "L4_ser: successfully bound irq capability to karma_log\n");
    }

    int len=2;
    int ret=0;
    l4_msgtag_t tag;
    if ((ret = l4_error(_irq.lirq()->attach(12, thread))))
    {
        karma_log(ERROR, "_irq->attach error %d\n", ret);
        exit(1);
    }
    while(1)
    {
        karma_log(DEBUG, "L4_ser while...\n");
        if ((l4_ipc_error(tag = _irq.lirq()->receive(), l4_utcb())))
            karma_log(ERROR, "_irq->receive ipc error: %s\n", l4sys_errtostr(l4_error(tag)));
//        else if (!tag.is_irq())
//            printf("Did not receive an IRQ\n");

        ret = L4Re::Env::env()->log()->read((char *)buf, len);
        if(ret > 0 && buf[0]>0)
        {
            for(int i=0; i<ret; i++)
                _read_buf.put(buf[i]);
            _ser_irq.lirq()->trigger();
        }
    }
    _irq.lirq()->detach();
#endif // L4SER_READ_IMPLEMENTED
}
