/*
 * (c) 2011 Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#pragma once

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>
#include <l4/vbus/vbus_types.h>

/**
 * \ingroup l4vbus_module
 * \defgroup l4vbus_gpio_module L4vbus GPIO functions
 * \{
 */

__BEGIN_DECLS

/**
 * \brief Constants for generic GPIO functions
 */
enum L4vbus_gpio_generic_func
{
  L4VBUS_GPIO_SETUP_INPUT  = 0x100, ///< Set GPIO pin to input
  L4VBUS_GPIO_SETUP_OUTPUT = 0x200, ///< Set GPIO pin to output
  L4VBUS_GPIO_SETUP_IRQ    = 0x300, ///< Set GPIO pin to IRQ
};

/**
 * \brief Constants for generic GPIO pull up/down resistor configuration
 */
enum L4vbus_gpio_pull_modes
{
  L4VBUS_GPIO_PIN_PULL_NONE = 0x100,  ///< No pull up or pull down resistors
  L4VBUS_GPIO_PIN_PULL_UP   = 0x200,  ///< enable pull up resistor
  L4VBUS_GPIO_PIN_PULL_DOWN = 0x300,  ///< enable pull down resistor
};

/**
 * \brief Configure the function of a GPIO pin
 *
 * \param vbus      V-BUS capability
 * \param handle    Device handle for the GPIO chip
 * \param pin       GPIO pin number
 * \param mode      GPIO function, see #L4vbus_gpio_generic_func for generic
 *                  functions. Hardware specific functions must be provided in
 *                  the lower 8 bits.
 * \param outvalue  Optional value to set the GPIO pin to if it is configured
 *                  as an output pin
 * \return          0 if OK, error code otherwise
 */
int L4_CV
l4vbus_gpio_setup(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                  unsigned pin, unsigned mode, int outvalue);

/**
 * \brief Generic function to set pull up/down mode
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param pin     GPIO pin number
 * \param mode    mode for pull up/down resistors, see #L4vbus_gpio_pull_modes
 * \return        0 if OK, error code otherwise
 */
int L4_CV
l4vbus_gpio_config_pull(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                        unsigned pin, unsigned mode);

/**
 * \brief Hardware specific configuration function
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param pin     GPIO pin number
 * \param func    Hardware specific configuration register, usually offset to
 *                the GPIO chip's base address
 * \param value   Value which is written into the hardware specific
 *                configuration register for the specified pin
 * \return        0 if OK, error code otherwise
 */
int L4_CV
l4vbus_gpio_config_pad(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                       unsigned pin, unsigned func, unsigned value);

/**
 * \brief Read hardware specific configuration
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param pin     GPIO pin number
 * \param func    Hardware specific configuration register to read from.
 *                Usually this is an offset to the GPIO chip's base address.
 * \retval value  The configuration value.
 * \return        0 if OK, error code otherwise
 */
int L4_CV
l4vbus_gpio_config_get(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                       unsigned pin, unsigned func, unsigned *value);

/**
 * \brief Read value of GPIO input pin
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param pin     GPIO pin number to read from
 * \return        Value of GPIO pin (usually 0 or 1), negative error code
 *                otherwise.
 */
int L4_CV
l4vbus_gpio_get(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                unsigned pin);

/**
 * \brief Set GPIO output pin
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param pin     GPIO pin number to write to
 * \param value   Value to write to the GPIO pin (usually 0 or 1)
 * \return        0 if OK, error code otherwise
 */
int L4_CV
l4vbus_gpio_set(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                unsigned pin, int value);

/**
 * \brief Configure function of multiple GPIO pins at once
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param offset  Pin corresponding to the LSB in \a mask.
 *                Note: allowed may be hardware specific.
 * \param mask    Mask of GPIO pins to configure. A bit set to 1 configures
 *                this pin. A maximum of 32 pins can be configured at once.
 *                The real number depends on the hardware and the driver
 *                implementation.
 * \param mode    GPIO function, see #L4vbus_gpio_generic_func for generic
 *                functions. Hardware specific functions must be provided in
 *                the lower 8 bits.
 * \param value   Optional value to set the GPIO pins to if they are
 *                configured as output pins
 * \return        0 if OK, error code otherwise
 */
int L4_CV
l4vbus_gpio_multi_setup(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                        unsigned offset, unsigned mask,
                        unsigned mode, unsigned value);

/**
 * \brief Hardware specific configuration function for multiple GPIO pins
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param offset  Pin corresponding to the LSB in \a mask.
 *                Note: allowed may be hardware specific.
 * \param mask    Mask of GPIO pins to configure. A bit set to 1 configures
 *                this pin. A maximum of 32 pins can be configured at once.
 *                The real number depends on the hardware and the driver
 *                implementation.
 * \param func    Hardware specific configuration register, usually offset to
 *                the GPIO chip's base address.
 * \param value   Value which is written into the hardware specific
 *                configuration register for the specified pins
 * \return        0 if OK, error code otherwise
 */
int L4_CV
l4vbus_gpio_multi_config_pad(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                              unsigned offset, unsigned mask,
                              unsigned func, unsigned value);

/**
 * \brief Read values of multiple GPIO pins at once
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param offset  Pin corresponding to the LSB in \a data.
 *                Note: allowed may be hardware specific.
 * \retval data   Each bit returns the value (0 or 1) for the corresponding
 *                GPIO pin. The value of pins that are not accessible is
 *                undefined.
 * \return        0 if OK, error code otherwise
 */
int L4_CV
l4vbus_gpio_multi_get(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                      unsigned offset, unsigned *data);

/**
 * \brief Set multiple GPIO output pins at once
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param offset  Pin corresponding to the LSB in \a data.
 *                Note: allowed may be hardware specific.
 * \param mask    Mask of GPIO pins to set. A bit set to 1 selects this pin.
 *                A maximum of 32 pins can be set at once. The real number
 *                depends on the hardware and the driver implementation.
 * \param data    Each bit corresponds to the GPIO pin in \a mask. The value
 *                of each bit is written to the GPIO pin if its bit in \a mask
 *                is set.
 * \return        0 if OK, error code otherwise
 */
int L4_CV
l4vbus_gpio_multi_set(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                      unsigned offset, unsigned mask, unsigned data);

/**
 * \brief Create IRQ for GPIO pin
 *
 * \param vbus    V-BUS capability
 * \param handle  Device handle for the GPIO chip
 * \param pin     GPIO pin to create an IRQ for.
 * \return        IRQ number if OK, negative error code otherwise
 */
int L4_CV
l4vbus_gpio_to_irq(l4_cap_idx_t vbus, l4vbus_device_handle_t handle,
                   unsigned pin);

/**\}*/

__END_DECLS
