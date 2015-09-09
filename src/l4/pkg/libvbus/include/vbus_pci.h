/*
 * (c) 2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <l4/sys/compiler.h>
#include <l4/vbus/vbus_types.h>
#include <l4/sys/types.h>

/**
 * \ingroup l4vbus_module
 * \{
 */


__BEGIN_DECLS

/**
 * \brief Read from the vPCI configuration space.
 *
 * \param  vbus         Capability of the system bus
 * \param  handle       Device handle for the PCI root bridge
 * \param  bus          Bus number
 * \param  devfn        Device id (upper 16bit) and function (lower 16bit)
 * \param  reg          Register in configuration space to read
 * \retval value        Value that has been read
 * \param  width        Width to read in bits (e.g. 8, 16, 32)
 *
 * \return 0 on succes, else failure
 */
int L4_CV
l4vbus_pci_cfg_read(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                    l4_uint32_t bus, l4_uint32_t devfn,
                    l4_uint32_t reg, l4_uint32_t *value, l4_uint32_t width);

/**
 * \brief Write to the vPCI configuration space.
 *
 * \param  vbus         Capability of the system bus
 * \param  handle       Device handle of PCI root bridge
 * \param  bus          Bus number
 * \param  devfn        Device id (upper 16bit) and function (lower 16bit)
 * \param  reg          Register in configuration space to write
 * \param  value        Value to write
 * \param  width        Width to write in bits (e.g. 8, 16, 32)
 */
int L4_CV
l4vbus_pci_cfg_write(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                     l4_uint32_t bus, l4_uint32_t devfn,
                     l4_uint32_t reg, l4_uint32_t value, l4_uint32_t width);

/**
 * \brief Enable PCI interrupt for a specific device
 *
 * \param  vbus         Capability of the system bus
 * \param  handle       Device handle of PCI root bridge
 * \param  bus          Bus number
 * \param  devfn        Device id (upper 16bit) and function (lower 16bit)
 * \param  pin          Interrupt pin (normally as reported in
 *                      configuration register INTR)
 * \retval trigger      True if interrupt is level-triggered
 * \retval polarity     False if interrupt is of low polarity
 *
 * \return On success: Interrupt line to be used,
 *         else failure
 */
int L4_CV
l4vbus_pci_irq_enable(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                      l4_uint32_t bus, l4_uint32_t devfn,
                      int pin, unsigned char *trigger,
                      unsigned char *polarity);

/**\}*/
__END_DECLS
