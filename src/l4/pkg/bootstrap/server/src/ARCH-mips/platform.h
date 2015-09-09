/*
 * \brief  Support for the malta platform
 *
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Prajna Dasgupta <prajna@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef _MIPS_PLATFORM_H_
#define _MIPS_PLATFORM_H_

/*
 * Base register definitions for various MIPS system controllers
 */
#define MIPS_SYSCON_GT64XX_REG_BASE    0Xbbe00000
#define MIPS_SYSCON_MSC_REG_BASE       0xbbd00000
#define MIPS_SYSCON_BONITO_REG_BASE    0xbfe00000

/*
 * Revision register.
 */
#define MIPS_REVISION_REG                  0xBFC00010
#define MIPS_REVISION_CORID_QED_RM5261     0
#define MIPS_REVISION_CORID_CORE_LV        1
#define MIPS_REVISION_CORID_BONITO64       2
#define MIPS_REVISION_CORID_CORE_20K       3
#define MIPS_REVISION_CORID_CORE_FPGA      4
#define MIPS_REVISION_CORID_CORE_MSC       5
#define MIPS_REVISION_CORID_CORE_EMUL      6
#define MIPS_REVISION_CORID_CORE_FPGA2     7
#define MIPS_REVISION_CORID_CORE_FPGAR2    8
#define MIPS_REVISION_CORID_CORE_FPGA3     9
#define MIPS_REVISION_CORID_CORE_24K       10
#define MIPS_REVISION_CORID_CORE_FPGA4     11
#define MIPS_REVISION_CORID_CORE_FPGA5     12

/* Artificial CoreId Defines */
/*
 *  CoreEMUL with   Bonito   System Controller is treated like a Core20K
 *  CoreEMUL with SOC-it 101 System Controller is treated like a CoreMSC
 */
#define MIPS_REVISION_CORID_CORE_EMUL_BON  -1
#define MIPS_REVISION_CORID_CORE_EMUL_MSC  -2

#define MIPS_REVISION_CORID (((*(volatile unsigned long *)MIPS_REVISION_REG) >> 10) & 0x3f)

#define MIPS_REVISION_SCON_OTHER     0
#define MIPS_REVISION_SCON_SOCITSC     1
#define MIPS_REVISION_SCON_SOCITSCP    2

/* Artificial SCON defines for MIPS_REVISION_SCON_OTHER */
#define MIPS_REVISION_SCON_UNKNOWN     -1
#define MIPS_REVISION_SCON_GT64120     -2
#define MIPS_REVISION_SCON_BONITO    -3
#define MIPS_REVISION_SCON_BRTL      -4
#define MIPS_REVISION_SCON_SOCIT     -5
#define MIPS_REVISION_SCON_ROCIT     -6

#define MIPS_REVISION_SCONID (((*(volatile unsigned long *)MIPS_REVISION_REG) >> 24) & 0xff)

#define mb()        asm volatile ("sync")
#define wmb()       asm volatile ("sync")

/* PCI IO resource flags */
#define IORESOURCE_TYPE_BITS    0x00001f00      /* Resource type */
#define IORESOURCE_IO           0x00000100      /* PCI/ISA I/O ports */
#define IORESOURCE_MEM          0x00000200
#define IORESOURCE_REG          0x00000300      /* Register offsets */
#define IORESOURCE_IRQ          0x00000400
#define IORESOURCE_DMA          0x00000800
#define IORESOURCE_BUS          0x00001000

/* from linux/arch/mips/include/asm/io.h */
#define IO_SPACE_LIMIT 0xffff

/*
 * Bonito System Controller Definitions
 */
#define BONITO_REG_BASE         MIPS_SYSCON_BONITO_REG_BASE
#define BONITO(x)   *(volatile unsigned long *)(BONITO_REG_BASE + (x))
#define BONITO_PCICONFIGBASE    0x00
#define BONITO_REGBASE          0x100

/* Bonito PCI Configuration Registers */
#define BONITO_PCI_REG(x) BONITO(BONITO_PCICONFIGBASE + (x))
#define BONITO_PCIDID     BONITO_PCI_REG(0x00)

/*
 * MIPS System Controller Definitions
 */
#define MIPS_MSC01_PCI_REG_BASE   MIPS_SYSCON_MSC_REG_BASE
#define MSC01_PCI_REG_BASE        MIPS_MSC01_PCI_REG_BASE

/* MIPS PCI Configuration Registers */
#define MSC_WRITE(reg, data)  do { *(volatile unsigned long *)(reg) = data; } while (0)
#define MSC_READ(reg, data) do { data = *(volatile unsigned long *)(reg); } while (0)

#include "msc01_pci.h"

/*
 * GT64xx System Controller Definitions
 */
#define MIPS_GT_BASE        MIPS_SYSCON_GT64XX_REG_BASE
#define GT64120_BASE        MIPS_GT_BASE

/* GT64xx PCI Configuration Registers */
#define __GT_READ(ofs) (*(volatile unsigned long *)(GT64120_BASE+(ofs)))
#define GT_READ(ofs) __GT_READ(ofs)

#define __GT_WRITE(ofs, data) \
  do { *(volatile unsigned long *)(GT64120_BASE+(ofs)) = (data); } while (0)
#define GT_WRITE(ofs, data) __GT_WRITE(ofs, (data))

#include "gt64120.h"

#endif /* _MIPS_PLATFORM_H_ */
