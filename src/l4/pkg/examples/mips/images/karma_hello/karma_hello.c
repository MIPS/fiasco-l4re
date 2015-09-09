/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <string.h>
#include "asm/l4.h"

// KYMAXXX based on karma linux ./drivers/tty/serial/karma_serial.c
static char _shared_mem[1024] __attribute__((aligned(4096)));

static void karma_ser_write(unsigned long opcode, unsigned long val){
    karma_hypercall1(KARMA_MAKE_COMMAND(KARMA_DEVICE_ID(ser), opcode), &val);
}

static void
l4ser_console_write(/*struct console *co,*/ const char *s, unsigned int count)
{
    if (count > sizeof(_shared_mem))
        count = sizeof(_shared_mem);

    memcpy(_shared_mem, s, count);
    karma_ser_write(karma_ser_df_writeln, count);
}



#if 0
static int karma_ser_read(unsigned long opcode)
{
    const char msg[] = "MIPS VM Guest: Calling read!\n";
    l4ser_console_write(msg, sizeof(msg));

    unsigned long ret = 0;
    while (1) {
        karma_hypercall1(KARMA_MAKE_COMMAND(KARMA_DEVICE_ID(ser), opcode), &ret);
        if (ret != -1 && ret != 0)
            break;
    }
    return ret;
}

#endif


#if 1
static int karma_ser_read(unsigned long opcode)
{
    KARMA_READ_IMPL(ser, opcode);
}

static int
l4ser_getchar(/* struct l4ser_uart_port *l4port */)
{
    int c;

    while (1) {
        c = (int)karma_ser_read(karma_ser_df_read);

        if (c != -1 && c != 0)
            break;

    }
    return c;
}
#endif

int main(void)
{
    const char msg[] = "MIPS VM Guest: Hello World!\n"
                       "Press any key to exit\n";
    const char exit_msg[] = "MIPS VM Guest: Exiting!\n";

    karma_hypercall_init();
    karma_ser_write(karma_ser_df_init, (unsigned long)&_shared_mem);
    l4ser_console_write(msg, sizeof(msg));

#if 0
    int c = karma_ser_read(karma_ser_df_read);
#else
    int c = (int)l4ser_getchar();
#endif

    l4ser_console_write(exit_msg, sizeof(exit_msg));

    // never returns
    karma_hypercall_exit_op();

    return 0;
}
