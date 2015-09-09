/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef HYPERCALL_H_
#define HYPERCALL_H_

//#include <l4/sys/compiler.h>
#include "karma/karma_devices.h"

/* Some karma devices set the lsb for write ops, some use another scheme */
#define KARMA_WRITE_IMPL(dev, addr, val)\
        karma_hypercall1(KARMA_MAKE_COMMAND(KARMA_DEVICE_ID(dev), ((addr) | 1)), &val)

#define KARMA_READ_IMPL(dev, addr)\
        unsigned long ret = 0;\
        karma_hypercall1(KARMA_MAKE_COMMAND(KARMA_DEVICE_ID(dev), addr), &ret);\
        return ret

//EXTERN_C_BEGIN

void karma_hypercall0(unsigned long cmd);
void karma_hypercall1(unsigned long cmd, unsigned long * reg1);
void karma_hypercall2(unsigned long cmd, unsigned long * reg1, unsigned long * reg2);
void karma_hypercall3(unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3);
void karma_hypercall4(unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3, unsigned long * reg4);

void karma_hypercall_exit_op(void);

//EXTERN_C_END

#endif /* HYPERCALL_H_ */
