/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "boot_info.h"
#include <cstring>


EXTENSION class Config
{
public:
  enum
  {
    // cannot access user memory directly
    //Access_user_mem = Access_user_mem_direct,
    Access_user_mem = No_access_user_mem,
    //Access_user_mem = Must_access_user_mem_direct,

    PAGE_SHIFT = ARCH_PAGE_SHIFT,
    PAGE_SIZE  = 1 << PAGE_SHIFT,
    PAGE_MASK  = ~(PAGE_SIZE - 1),
    SUPERPAGE_SHIFT = 22,
    SUPERPAGE_SIZE  = 1 << SUPERPAGE_SHIFT,
    SUPERPAGE_MASK  = ~(SUPERPAGE_SIZE -1),

    // XXXKYMA: No large pages/TLBs yet, update paging-mips32 when enabled
    have_superpages = 0,
    hlt_works_ok = 1,
    Irq_shortcut = 1,
  };

  enum
  {
    Kmem_size     = 8*1024*1024, 	//8 MB
  };

  enum
  {
    Scheduler_one_shot		= 0,
    Scheduler_granularity	= 1000UL,
    Default_time_slice	        = 10 * Scheduler_granularity,
    cpu_frequency               = CONFIG_CPU_FREQUENCY,
  };

  enum mips_sys_controller_type
  {
    Sys_Controller_NONE = 0,
    Sys_Controller_GT = 1,
    Sys_Controller_ROCIT =2,
  };

  enum mips_sys_controller_base
  {
    Sys_Controller_Base_GT = 0xbbe00000,
    Sys_Controller_Base_ROCIT = 0xbbd00000,
  };

  enum mips_sys_controller_iodev_base
  {
    Sys_Controller_Iodev_Base_GT = 0xb8000000,
    Sys_Controller_Iodev_Base_ROCIT = 0xbb000000,
  };

  static mips_sys_controller_type sys_controller;
  static Address pci_port_base;
  static Address iodevs_port_base;
  static Address console_uart_base;

  /*
   * Kyma: default_console_uart doesn't seem to be really used for 
   * initing our console but we need it since it is used by Kernel_uart
   */
  static unsigned const default_console_uart = 0xb80003f8;
  static unsigned const default_console_uart_baudrate = 115200;
  static const char char_micro;

private:
  static void init_arch_platform();
};


//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "kip.h"

char const Config::char_micro = '\265';
const char *const Config::kernel_warn_config_string = 0;
Config::mips_sys_controller_type Config::sys_controller;
Address Config::pci_port_base;
Address Config::iodevs_port_base;
Address Config::console_uart_base;

IMPLEMENT FIASCO_INIT
void
Config::init_arch()
{
  // set a smaller default for JDB trace buffers
  Config::tbuf_entries = 1024;

#if defined(CONFIG_JDB) && defined(CONFIG_SERIAL)
  // for convenience, automatically enable serial_esc as if serial-esc was set
  if (   !Koptions::o()->opt(Koptions::F_noserial)
      && !Koptions::o()->opt(Koptions::F_nojdb))
    {
      serial_esc = SERIAL_ESC_IRQ;
    }
#endif

  init_arch_platform();
}

IMPLEMENT_DEFAULT FIASCO_INIT
void
Config::init_arch_platform()
{
}

PUBLIC static
void Config::probe_sys_controller()
{
  /*
   * Kip contains the board name set up by the bootstrap
   * program. Extrapolate the system controller type and 
   * set up platform specific information based on the type
   */
  Kip *kinfo = Kip::k();

  if ((strncmp(kinfo->platform_info.name, "qemu", 4)) == 0) {
    sys_controller = Sys_Controller_GT;
    pci_port_base = Sys_Controller_Base_GT;
    iodevs_port_base = Sys_Controller_Iodev_Base_GT;
    console_uart_base = iodevs_port_base + 0x3F8;
    return;
  }

  if ((strncmp(kinfo->platform_info.name, "malta", 5)) == 0) {
    sys_controller = Sys_Controller_ROCIT;
    pci_port_base = Sys_Controller_Base_ROCIT;
    iodevs_port_base = Sys_Controller_Iodev_Base_ROCIT;
    console_uart_base = iodevs_port_base + 0x3F8;
    return;
  }

  //TBD - panic?, assert?
}
