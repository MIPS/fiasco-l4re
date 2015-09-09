/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32 && serial]:

#include "kernel_console.h"
#include "kernel_uart.h"
#include "static_init.h"

// An L4 application is sending the escape-HOME key sequence to the Console.
// When using a serial console, processing the escape-HOME key sequence
// overwrites and loses one screen of log output. To prevent the log output from
// being lost, define L4_CONSOLE_IGNORE_ESC_HOME to use Kconsole as the
// Console.
#define L4_CONSOLE_IGNORE_ESC_HOME

/* Instantiate and initialize the console, and attach the Kernel_uart */
STATIC_INITIALIZE_P(Kconsole, EARLY_INIT_PRIO);
STATIC_INITIALIZER_P(boot_console_init, EARLY_INIT_PRIO);

static void boot_console_init()
{
  Kernel_uart::init(Kernel_uart::Init_after_mmu);
#ifdef L4_CONSOLE_IGNORE_ESC_HOME
  Console::stdout = Kernel_uart::uart();
  Console::stderr = Console::stdout;
  Console::stdin  = Console::stdout;
#endif
}
