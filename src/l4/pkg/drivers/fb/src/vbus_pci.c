/*
 * Handling of vbus PCI devices
 *
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <stdio.h>
#include <string.h>
#include <l4/re/c/namespace.h>
#include <l4/re/c/util/cap_alloc.h>
#include <l4/sys/err.h>

#include "vbus_pci.h"

#define MAX_DRIVERS 4

static struct vbus_pci_driver vbus_pci_drv[MAX_DRIVERS];

int vbus_pci_register(struct vbus_pci_driver *drv_register)
{
  for (int i = 0; i < MAX_DRIVERS; i++)
  {
    if (vbus_pci_drv[i].probe == 0)
    {
      memcpy(&vbus_pci_drv[i], drv_register, sizeof(struct vbus_pci_driver));
      return 0;
    }
  }

  printf("Too many drivers registered, increase MAX_DRIVERS!!\n");
  return -L4_ENODEV;
}

l4_cap_idx_t vbus = L4_INVALID_CAP;
l4vbus_device_handle_t root_bridge = 0;

int
vbus_pci_probe(unsigned class_id_match /*, con_accel_t *accel*/)
{
  long err;
  int ret;

  vbus = l4re_env_get_cap("vbus");
  if (l4_is_invalid_cap(vbus))
    {
      printf("PCI: query vbus service failed, no PCI\n");
      return -L4_ENODEV;
    }

  err = l4vbus_get_device_by_hid(vbus, 0, &root_bridge, "PNP0A03", L4VBUS_MAX_DEPTH, 0);

  if (err < 0) {
      printf("PCI: no vbus root bridge found, no PCI\n");
      return -L4_ENODEV;
  }

  unsigned bus = 0;
  unsigned slot, fn = 0;
  /* only scan possible devices and subdevices on the first bus */
  for (slot = 0; slot < 32; slot++)
    //for (fn = 0; fn < 8; fn ++)
      {
	unsigned devfn = (slot << 16) | fn;
	struct vbus_pci_driver *drv;
	const vbus_pci_device_id *dev;

	/* only scan for devices matching the class_id */
	unsigned class_id = 0;
	VBUS_PCIBIOS_READ_CONFIG_WORD(bus, devfn, PCI_CLASS_DEVICE, &class_id);
	if (class_id != class_id_match)
	  continue;
	printf("PCI: Found matching vbus device class_id %#04x\n", class_id);

        for (int i = 0; i < MAX_DRIVERS; i++)
	  {
            drv = &vbus_pci_drv[i];
            if (drv->probe == 0)
              break;

	    for (dev = drv->id_table; dev->vendor; dev++)
	      {
		unsigned vendor = 0, device = 0, sub_vendor = 0, sub_device = 0;
		VBUS_PCIBIOS_READ_CONFIG_WORD(bus, devfn, PCI_VENDOR_ID, &vendor);
		VBUS_PCIBIOS_READ_CONFIG_WORD(bus, devfn, PCI_DEVICE_ID, &device);
		VBUS_PCIBIOS_READ_CONFIG_WORD(bus, devfn, PCI_SUBSYSTEM_VENDOR_ID, &sub_vendor);
		VBUS_PCIBIOS_READ_CONFIG_WORD(bus, devfn, PCI_SUBSYSTEM_ID, &sub_device);
		if (dev->vendor != vendor)
		  continue;
		if (dev->device != 0)
		  if (dev->device != device)
		    continue;
		if (dev->subvendor != PCI_ANY_ID)
		  if ((dev->subvendor != sub_vendor) ||
		      (dev->subdevice != sub_device))
		    continue;

		/* found appropriate driver ... */
		if ((ret = drv->probe(bus, devfn, dev/*, accel*/)) != 0)
		  /* ... YES */
		  continue;

		return L4_EOK;
	      }
	  }
      }

  return -L4_ENODEV;
}

int
vbus_pci_probe_vga(void)
{
  /* only scan for graphics cards */
  printf("Scanning vbus PCI for VGA device class_id %#04x\n", PCI_CLASS_DISPLAY_VGA);
  return vbus_pci_probe(PCI_CLASS_DISPLAY_VGA);
}

void
vbus_pci_resource(unsigned int bus, unsigned int devfn, 
	     int num, l4_addr_t *addr, l4_size_t *size)
{
  unsigned l, sz, reg;

  switch (num)
    {
    case 0:  reg = PCI_BASE_ADDRESS_0; break;
    case 1:  reg = PCI_BASE_ADDRESS_1; break;
    case 2:  reg = PCI_BASE_ADDRESS_2; break;
    default: return;
    }

  VBUS_PCIBIOS_READ_CONFIG_DWORD (bus, devfn, reg, &l);
  VBUS_PCIBIOS_WRITE_CONFIG_DWORD(bus, devfn, reg, ~0);
  VBUS_PCIBIOS_READ_CONFIG_DWORD (bus, devfn, reg, &sz);
  VBUS_PCIBIOS_WRITE_CONFIG_DWORD(bus, devfn, reg, l);
  if ((l & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_MEMORY)
    {
      *addr = l & PCI_BASE_ADDRESS_MEM_MASK;
      sz   &= PCI_BASE_ADDRESS_MEM_MASK;
      *size = sz & ~(sz - 1);
    }
  else
    {
      *addr = l & PCI_BASE_ADDRESS_IO_MASK;
      sz   &= PCI_BASE_ADDRESS_IO_MASK & 0xffff;
      *size = sz & ~(sz - 1);
    }
}

void vbus_pci_enable_memory(unsigned int bus, unsigned int devfn)
{
  unsigned int cmd;
  VBUS_PCIBIOS_READ_CONFIG_WORD(bus, devfn, PCI_COMMAND, &cmd);
  if (!(cmd & PCI_COMMAND_MEMORY)) 
    {
      cmd |= PCI_COMMAND_MEMORY;
      VBUS_PCIBIOS_WRITE_CONFIG_WORD(bus, devfn, PCI_COMMAND, cmd);
    }
}

void vbus_pci_enable_io(unsigned int bus, unsigned int devfn)
{
  unsigned int cmd;
  VBUS_PCIBIOS_READ_CONFIG_WORD(bus, devfn, PCI_COMMAND, &cmd);
  if (!(cmd & PCI_COMMAND_IO)) 
    {
      cmd |= PCI_COMMAND_IO;
      VBUS_PCIBIOS_WRITE_CONFIG_WORD(bus, devfn, PCI_COMMAND, cmd);
    }
}

void vbus_pci_enable_busmaster(unsigned int bus, unsigned int devfn)
{
  unsigned int cmd;
  VBUS_PCIBIOS_READ_CONFIG_WORD(bus, devfn, PCI_COMMAND, &cmd);
  if (!(cmd & PCI_COMMAND_MASTER)) 
    {
      cmd |= PCI_COMMAND_MASTER;
      VBUS_PCIBIOS_WRITE_CONFIG_WORD(bus, devfn, PCI_COMMAND, cmd);
    }
}

void vbus_dump_pci_vbus(void)
{
  l4io_device_handle_t devhandle = l4io_get_root_device();
  l4io_device_t dev;
  l4io_resource_handle_t reshandle;

  while (!l4io_iterate_devices(&devhandle, &dev, &reshandle))
    {
      l4io_resource_t res;
      printf("device: type=%x  name=%s numres=%d flags=%x\n",
             dev.type, dev.name, dev.num_resources, dev.flags);
      while (!l4io_lookup_resource(devhandle, L4IO_RESOURCE_ANY,
                                   &reshandle, &res))
        {
          printf("   resource: %d %x %lx-%lx\n",
                 res.type, res.flags, res.start, res.end);
        }
    }
}
