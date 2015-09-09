/*
 * \brief  Support for the malta platform
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * Borrowing from linux 3.17 arch/mips/mti-malta/malta-init.c and malta-pci.c
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * PROM library initialisation code.
 *
 * Copyright (C) 1999,2000,2004,2005,2012  MIPS Technologies, Inc.
 * All rights reserved.
 * Authors: Carsten Langgaard <carstenl@mips.com>
 *         Maciej W. Rozycki <macro@mips.com>
 *          Steven J. Hill <sjhill@mips.com>
 */
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <l4/drivers/uart_pxa.h>
#include "support.h"
#include "platform.h"
#include "macros.h"
#include "startup.h"

// PCI resource structures
struct resource {
        const char *name;
        unsigned long start, end;
        unsigned long flags;
};

struct pci_controller {
        struct resource *mem_resource;
        unsigned long mem_offset;
        struct resource *io_resource;
        unsigned long io_offset;
        unsigned long io_map_base;
};

static struct resource gt64120_mem_resource = {
        name  : "GT-64120 PCI MEM",
        start : 0,
        end   : 0,
        flags : IORESOURCE_MEM
};

static struct resource gt64120_io_resource = {
        name  : "GT-64120 PCI I/O",
        start : 0,
        end   : 0,
        flags : IORESOURCE_IO
};

static struct resource msc_mem_resource = {
        name  : "MSC PCI MEM",
        start : 0,
        end   : 0,
        flags : IORESOURCE_MEM
};

static struct resource msc_io_resource = {
        name  : "MSC PCI I/O",
        start : 0,
        end   : 0,
        flags : IORESOURCE_IO
};

struct resource ioport_resource = {
        name  : "PCI IO",
        start : 0,
        end   : IO_SPACE_LIMIT,
        flags : IORESOURCE_IO
};

struct resource iomem_resource = {
        name  : "PCI mem",
        start : 0,
        end   : -1UL,
        flags : IORESOURCE_MEM
};

static struct pci_controller gt64120_controller = {
        mem_resource	: &gt64120_mem_resource,
        mem_offset      : 0,
        io_resource	: &gt64120_io_resource,
        io_offset       : 0,
        io_map_base     : 0
};

static struct pci_controller msc_controller = {
        mem_resource	: &msc_mem_resource,
        mem_offset      : 0,
        io_resource	: &msc_io_resource,
        io_offset       : 0,
        io_map_base     : 0
};

namespace {

class Platform_mips_malta : public Platform_single_region_ram
{
private : 

/* List of supported system controllers */
#define MIPS_PLATFORM_SYSCTRLER_GT    1
#define MIPS_PLATFORM_SYSCTRLER_MIPS  2

  unsigned long mips_sys_controller;
  int mips_revision_corid;
  int mips_revision_sconid;

  bool probe_sys_controller() {

    mips_sys_controller = 0;

    /*
     * Determine the system controller on board
     */
    mips_revision_corid = MIPS_REVISION_CORID;
    if (mips_revision_corid == MIPS_REVISION_CORID_CORE_EMUL) {
      if (BONITO_PCIDID == (unsigned long) 0x0001df53 ||
        BONITO_PCIDID == (unsigned long) 0x0003df53)
        mips_revision_corid = MIPS_REVISION_CORID_CORE_EMUL_BON;
      else
        mips_revision_corid = MIPS_REVISION_CORID_CORE_EMUL_MSC;
    }
    mips_revision_sconid = MIPS_REVISION_SCONID;
    if (mips_revision_sconid == MIPS_REVISION_SCON_OTHER) {
      switch (mips_revision_corid) {
        case MIPS_REVISION_CORID_QED_RM5261:
        case MIPS_REVISION_CORID_CORE_LV:
        case MIPS_REVISION_CORID_CORE_FPGA:
        case MIPS_REVISION_CORID_CORE_FPGAR2:
          mips_revision_sconid = MIPS_REVISION_SCON_GT64120;
        break;

        case MIPS_REVISION_CORID_CORE_EMUL_BON:
        case MIPS_REVISION_CORID_BONITO64:
        case MIPS_REVISION_CORID_CORE_20K:
          mips_revision_sconid = MIPS_REVISION_SCON_BONITO;
        break;

        case MIPS_REVISION_CORID_CORE_MSC:
        case MIPS_REVISION_CORID_CORE_FPGA2:
        case MIPS_REVISION_CORID_CORE_24K:
         /*
          * SOCit/ROCit support is essentially identical
          * but make an attempt to distinguish them
          */
          mips_revision_sconid = MIPS_REVISION_SCON_SOCIT;
          break;

        case MIPS_REVISION_CORID_CORE_FPGA3:
        case MIPS_REVISION_CORID_CORE_FPGA4:
        case MIPS_REVISION_CORID_CORE_FPGA5:
        case MIPS_REVISION_CORID_CORE_EMUL_MSC:
        default:
          /* See above */
          mips_revision_sconid = MIPS_REVISION_SCON_ROCIT;
          break;
      }
    }

    switch (mips_revision_sconid) {
      unsigned long map, start, mask, data;

      case MIPS_REVISION_SCON_GT64120:
        GT_WRITE(GT_PCI0_CMD_OFS, GT_PCI0_CMD_MBYTESWAP_BIT |
          GT_PCI0_CMD_SBYTESWAP_BIT);

        /* Fix up PCI I/O mapping if necessary (for Atlas).  */
        start = GT_READ(GT_PCI0IOLD_OFS);
        map = GT_READ(GT_PCI0IOREMAP_OFS);
        if ((start & map) != 0) {
          map &= ~start;
          GT_WRITE(GT_PCI0IOREMAP_OFS, map);
        }
        mips_sys_controller = MIPS_PLATFORM_SYSCTRLER_GT;
        return true;

      case MIPS_REVISION_SCON_BONITO:
        /* We do NOT support this controller */
        return false;

      case MIPS_REVISION_SCON_SOCIT:
      case MIPS_REVISION_SCON_ROCIT:
mips_pci_controller:
        mb();
        MSC_READ(MSC01_PCI_CFG, data);
        MSC_WRITE(MSC01_PCI_CFG, data & ~MSC01_PCI_CFG_EN_BIT);
        wmb();

        /* Fix up lane swapping.  */
        MSC_WRITE(MSC01_PCI_SWAP, MSC01_PCI_SWAP_NOSWAP);

        /* Fix up target memory mapping.  */
        MSC_READ(MSC01_PCI_BAR0, mask);
        MSC_WRITE(MSC01_PCI_P2SCMSKL, mask & MSC01_PCI_BAR0_SIZE_MSK);

        /* Don't handle target retries indefinitely.  */
        if ((data & MSC01_PCI_CFG_MAXRTRY_MSK) ==
          MSC01_PCI_CFG_MAXRTRY_MSK)
          data = (data & ~(MSC01_PCI_CFG_MAXRTRY_MSK <<
            MSC01_PCI_CFG_MAXRTRY_SHF)) |
            ((MSC01_PCI_CFG_MAXRTRY_MSK - 1) << MSC01_PCI_CFG_MAXRTRY_SHF);

        wmb();
        MSC_WRITE(MSC01_PCI_CFG, data);
        mb();
        mips_sys_controller = MIPS_PLATFORM_SYSCTRLER_MIPS;
        return true;

      case MIPS_REVISION_SCON_SOCITSC:
      case MIPS_REVISION_SCON_SOCITSCP:
        goto mips_pci_controller;

      default:
        /* Unknown system controller */
        return false;
    }
  }

  void mips_pcibios_init(void)
  {
          struct pci_controller *controller = NULL;
          unsigned long start, end, map, start1, end1, map1, mask;

          switch (mips_revision_sconid) {
          case MIPS_REVISION_SCON_GT64120:
                  /*
                   * Due to a bug in the Galileo system controller, we need
                   * to setup the PCI BAR for the Galileo internal registers.
                   * This should be done in the bios/bootprom and will be
                   * fixed in a later revision of YAMON (the MIPS boards
                   * boot prom).
                   */
                  GT_WRITE(GT_PCI0_CFGADDR_OFS,
                           (0 << GT_PCI0_CFGADDR_BUSNUM_SHF) | /* Local bus */
                           (0 << GT_PCI0_CFGADDR_DEVNUM_SHF) | /* GT64120 dev */
                           (0 << GT_PCI0_CFGADDR_FUNCTNUM_SHF) | /* Function 0*/
                           ((0x20/4) << GT_PCI0_CFGADDR_REGNUM_SHF) | /* BAR 4*/
                           GT_PCI0_CFGADDR_CONFIGEN_BIT);

#define CPHYSADDR(a) (((__PTRDIFF_TYPE__)(int) (a)) & 0x1fffffff)
                  /* Perform the write */
                  GT_WRITE(GT_PCI0_CFGDATA_OFS, CPHYSADDR(MIPS_GT_BASE));

                  /* Set up resource ranges from the controller's registers.  */
                  start = GT_READ(GT_PCI0M0LD_OFS);
                  end = GT_READ(GT_PCI0M0HD_OFS);
                  map = GT_READ(GT_PCI0M0REMAP_OFS);
                  end = (end & GT_PCI_HD_MSK) | (start & ~GT_PCI_HD_MSK);
                  start1 = GT_READ(GT_PCI0M1LD_OFS);
                  end1 = GT_READ(GT_PCI0M1HD_OFS);
                  map1 = GT_READ(GT_PCI0M1REMAP_OFS);
                  end1 = (end1 & GT_PCI_HD_MSK) | (start1 & ~GT_PCI_HD_MSK);
                  /* Cannot support multiple windows, use the wider.  */
                  if (end1 - start1 > end - start) {
                          start = start1;
                          end = end1;
                          map = map1;
                  }
                  mask = ~(start ^ end);
#define BUG_ON(x) assert(!(x))
                  /* We don't support remapping with a discontiguous mask.  */
                  BUG_ON((start & GT_PCI_HD_MSK) != (map & GT_PCI_HD_MSK) &&
                         mask != ~((mask & -mask) - 1));
                  gt64120_mem_resource.start = start;
                  gt64120_mem_resource.end = end;
                  gt64120_controller.mem_offset = (start & mask) - (map & mask);
                  /* Addresses are 36-bit, so do shifts in the destinations.  */
                  gt64120_mem_resource.start <<= GT_PCI_DCRM_SHF;
                  gt64120_mem_resource.end <<= GT_PCI_DCRM_SHF;
                  gt64120_mem_resource.end |= (1 << GT_PCI_DCRM_SHF) - 1;
                  gt64120_controller.mem_offset <<= GT_PCI_DCRM_SHF;

                  start = GT_READ(GT_PCI0IOLD_OFS);
                  end = GT_READ(GT_PCI0IOHD_OFS);
                  map = GT_READ(GT_PCI0IOREMAP_OFS);
                  end = (end & GT_PCI_HD_MSK) | (start & ~GT_PCI_HD_MSK);
                  mask = ~(start ^ end);
                  /* We don't support remapping with a discontiguous mask.  */
                  BUG_ON((start & GT_PCI_HD_MSK) != (map & GT_PCI_HD_MSK) &&
                         mask != ~((mask & -mask) - 1));
                  gt64120_io_resource.start = map & mask;
                  gt64120_io_resource.end = (map & mask) | ~mask;
                  gt64120_controller.io_offset = 0;
                  /* Addresses are 36-bit, so do shifts in the destinations.  */
                  gt64120_io_resource.start <<= GT_PCI_DCRM_SHF;
                  gt64120_io_resource.end <<= GT_PCI_DCRM_SHF;
                  gt64120_io_resource.end |= (1 << GT_PCI_DCRM_SHF) - 1;

                  controller = &gt64120_controller;
                  break;

          case MIPS_REVISION_SCON_BONITO:
                  /* We do NOT support this controller */
                  break;

          case MIPS_REVISION_SCON_SOCIT:
          case MIPS_REVISION_SCON_ROCIT:
          case MIPS_REVISION_SCON_SOCITSC:
          case MIPS_REVISION_SCON_SOCITSCP:
                  /* Set up resource ranges from the controller's registers.  */
                  MSC_READ(MSC01_PCI_SC2PMBASL, start);
                  MSC_READ(MSC01_PCI_SC2PMMSKL, mask);
                  MSC_READ(MSC01_PCI_SC2PMMAPL, map);
                  msc_mem_resource.start = start & mask;
                  msc_mem_resource.end = (start & mask) | ~mask;
                  msc_controller.mem_offset = (start & mask) - (map & mask);
                  MSC_READ(MSC01_PCI_SC2PIOBASL, start);
                  MSC_READ(MSC01_PCI_SC2PIOMSKL, mask);
                  MSC_READ(MSC01_PCI_SC2PIOMAPL, map);
                  msc_io_resource.start = map & mask;
                  msc_io_resource.end = (map & mask) | ~mask;
                  msc_controller.io_offset = 0;
                  ioport_resource.end = ~mask;
                  /* If ranges overlap I/O takes precedence.  */
                  start = start & mask;
                  end = start | ~mask;
                  if ((start >= msc_mem_resource.start &&
                       start <= msc_mem_resource.end) ||
                      (end >= msc_mem_resource.start &&
                       end <= msc_mem_resource.end)) {
                          /* Use the larger space.  */
                          start = max(start, msc_mem_resource.start);
                          end = min(end, msc_mem_resource.end);
                          if (start - msc_mem_resource.start >=
                              msc_mem_resource.end - end)
                                  msc_mem_resource.end = start - 1;
                          else
                                  msc_mem_resource.start = end + 1;
                  }

                  controller = &msc_controller;
                  break;

          default:
                  return;
          }

          if (controller == NULL)
            return;

          /* Change start address to avoid conflicts with ACPI and SMB devices */
          if (controller->io_resource->start < 0x00002000UL)
                  controller->io_resource->start = 0x00002000UL;

          iomem_resource.end &= 0xfffffffffULL;			/* 64 GB */
          ioport_resource.end = controller->io_resource->end;

#if 0
          controller->io_map_base = mips_io_port_base;

          register_pci_controller(controller);
#endif

#ifndef VERBOSE
#define VERBOSE 1
#endif
          int verbose = VERBOSE;
          if (verbose) {
            printf("MIPS Bootstrap. System Controller PCI Resources:\n");
            printf("  %s start %#lx end %#lx flags %#lx mem_offset %#lx\n",
                controller->mem_resource->name,
                controller->mem_resource->start,
                controller->mem_resource->end,
                controller->mem_resource->flags,
                controller->mem_offset);
            printf("  %s start %#lx end %#lx flags %#lx io_offset %#lx\n",
                controller->io_resource->name,
                controller->io_resource->start,
                controller->io_resource->end,
                controller->io_resource->flags,
                controller->io_offset);
            printf("  %s start %#lx end %#lx flags %#lx\n",
                ioport_resource.name,
                ioport_resource.start,
                ioport_resource.end,
                ioport_resource.flags);
            printf("  %s start %#lx end %#lx flags %#lx\n",
                iomem_resource.name,
                iomem_resource.start,
                iomem_resource.end,
                iomem_resource.flags);
          }

  }

public :  
  bool probe() { return probe_sys_controller(); }

  enum {
    I8259A_IRQ_BASE = 0,
    I8259A_IRQ_UART_TTY1 = I8259A_IRQ_BASE + 3,
    I8259A_IRQ_UART_TTY0 = I8259A_IRQ_BASE + 4,
    MIPS_CPU_IRQ_BASE = 16,
  };

  void init()
  {
    unsigned long uart_base;

    switch(mips_sys_controller) {
      case MIPS_PLATFORM_SYSCTRLER_MIPS:
        uart_base = 0xBB0003F8;
        break;
      case MIPS_PLATFORM_SYSCTRLER_GT:
        uart_base = 0xB80003F8;
        break;
      default:
        uart_base = 0xB80003F8;
        break;
    }
    static L4::Io_register_block_mmio r(uart_base); // uart0
    static L4::Uart_16550 _uart(L4::Uart_16550::Base_rate_x86);
    _uart.startup(&r);
    _uart.change_mode(L4::Uart_16550::MODE_8N1, 115200);
    set_stdio_uart(&_uart);

    kuart.access_type  = L4_kernel_options::Uart_type_mmio;
    kuart.irqno        = I8259A_IRQ_UART_TTY0;
    kuart.base_address = uart_base;
    kuart.base_baud    = L4::Uart_16550::Base_rate_x86;
    kuart.baud         = 115200;
    kuart_flags       |=   L4_kernel_options::F_uart_base
                         | L4_kernel_options::F_uart_baud;
    kuart_flags |= L4_kernel_options::F_uart_irq;

    mips_pcibios_init();

  }

  void setup_memory_map()
  {
    // let the base class scan for RAM
    Platform_single_region_ram::setup_memory_map();

    // reserve @ 0x0000       : first page containing exception base
    // reserve @ 0x0f00-0x1000: SMP cpulaunch structures to prevent launch flags
    // being zeroed by bootstrap -presetmem=0 command line arguement
    mem_manager->regions->add(Region::n(0, L4_PAGESIZE, ".kernel_resv", Region::Kernel));
  }

  void reboot()
  {
#define SOFTRES_REGISTER 0xBF000500
#define GORESET 0x42
    *(unsigned *)SOFTRES_REGISTER = GORESET;
  }

  const char *get_platform_name(void) {
    switch(mips_sys_controller) {
      case MIPS_PLATFORM_SYSCTRLER_MIPS:
        return "malta";
        break;
      case MIPS_PLATFORM_SYSCTRLER_GT:
        return "qemu";
        break;
      default:
        return "none";
        break;
    }
    return "unkown";
  }
};
}

REGISTER_PLATFORM(Platform_mips_malta);
