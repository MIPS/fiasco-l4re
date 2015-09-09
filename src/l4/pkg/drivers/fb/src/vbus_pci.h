/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/*
 * Handling of vbus PCI devices
 *
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#ifndef __VBUS_PCI_H_
#define __VBUS_PCI_H_

#include <l4/io/io.h>
#include <l4/vbus/vbus.h>
#include <l4/vbus/vbus_pci.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>

typedef struct pci_device_id  vbus_pci_device_id;

struct vbus_pci_driver
{
  const char *name;
  const vbus_pci_device_id *id_table;         /* must be non-NULL for probe to be called */
  int (*probe)(unsigned int bus, unsigned int devfn,
               const vbus_pci_device_id *dev);
};

int vbus_pci_probe(unsigned class_id_match);
int vbus_pci_probe_vga(void);
int vbus_pci_register(struct vbus_pci_driver *drv_register);
void vbus_pci_resource(unsigned int bus, unsigned int devfn, int num, 
		  l4_addr_t *addr, l4_size_t *size);
void vbus_pci_enable_memory(unsigned int bus, unsigned int devfn);
void vbus_pci_enable_io(unsigned int bus, unsigned int devfn);
void vbus_pci_enable_busmaster(unsigned int bus, unsigned int devfn);
void vbus_dump_pci_vbus(void);

extern l4_cap_idx_t vbus;
extern l4vbus_device_handle_t root_bridge;

/* krishna: looks a bit sick, but we want complete L4IO compatibility NOW.

   1. Do not use pcibios_*() or pci_*() from pcilib directly - use these macros.
   2. Test for con_hw_not_use_l4io before real execution

   l4io_pdev_t handle is stored in [bus:devfn].
*/

#define VBUS_PCIBIOS_READ_CONFIG_BYTE(bus, devfn, where, val)	\
	do {							\
	    l4vbus_pci_cfg_read(vbus, root_bridge, bus, devfn, where, (l4_uint32_t*)val, 8);	\
	} while (0)

#define VBUS_PCIBIOS_READ_CONFIG_WORD(bus, devfn, where, val)	\
	do {							\
	    l4vbus_pci_cfg_read(vbus, root_bridge, bus, devfn, where, (l4_uint32_t*)val, 16);	\
	} while (0)

#define VBUS_PCIBIOS_READ_CONFIG_DWORD(bus, devfn, where, val)	\
	do {							\
	    l4vbus_pci_cfg_read(vbus, root_bridge, bus, devfn, where, (l4_uint32_t*)val, 32);	\
	} while (0)

#define VBUS_PCIBIOS_WRITE_CONFIG_BYTE(bus, devfn, where, val)	\
	do {							\
	    l4vbus_pci_cfg_write(vbus, root_bridge, bus, devfn, where, (l4_uint32_t)val, 8);	\
	} while (0)

#define VBUS_PCIBIOS_WRITE_CONFIG_WORD(bus, devfn, where, val)	\
	do {							\
	    l4vbus_pci_cfg_write(vbus, root_bridge, bus, devfn, where, (l4_uint32_t)val, 16);	\
	} while (0)

#define VBUS_PCIBIOS_WRITE_CONFIG_DWORD(bus, devfn, where, val)	\
	do {							\
	    l4vbus_pci_cfg_write(vbus, root_bridge, bus, devfn, where, (l4_uint32_t)val, 32);	\
	} while (0)

#endif /* __VBUS_PCI_H_ */
