/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 */

#include <string.h>
#include "asm/l4.h"

// KYMAXXX based on karma linux ./drivers/tty/serial/l4ser.c
static char _shared_mem[1024] __attribute__((aligned(4096)));

static void l4ser_write(unsigned long addr, unsigned long val){
    KARMA_WRITE_IMPL(ser, addr, val);
}

static int
l4ser_console_write(/*struct console *co,*/ const char *s, unsigned int count)
{
    if (count > sizeof(_shared_mem))
        count = sizeof(_shared_mem);

    memcpy(_shared_mem, s, count);
    l4ser_write(L4_SER_WRITLN, count);
    l4ser_write(L4_SER_FLUSH, 0);
    return count;
}

int main(void)
{
  const char msg[] = "MIPS VM Guest: Hello World!\n";
  const char exit_msg[] = "MIPS VM Guest: Exiting!\n";

  karma_hypercall_init();
  l4ser_write(L4_SER_INIT, (unsigned long)&_shared_mem);
  l4ser_console_write(msg, sizeof(msg));
  l4ser_console_write(exit_msg, sizeof(exit_msg));

  // never returns
  karma_hypercall_exit_op();

  return 0;
}
