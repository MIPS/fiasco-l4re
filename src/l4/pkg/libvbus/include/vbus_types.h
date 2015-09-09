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

#include <l4/sys/types.h>

typedef l4_mword_t l4vbus_device_handle_t;
typedef l4_addr_t l4vbus_paddr_t;

typedef struct {
  l4_uint16_t    type;              /**< Resource type, see l4vbus_resource_type_t */
  l4_uint16_t    flags;             /**< Flags */
  l4vbus_paddr_t start;             /**< Start of res. range */
  l4vbus_paddr_t end;               /**< (inclusive) end of res. range */
  l4vbus_device_handle_t provider;  /**< Device handle of the provider of the resource */
  l4_uint32_t id;                   /**< Resource ID (4 bytes), usually a 4
                                         letter ASCII name is used */
} l4vbus_resource_t;

enum l4vbus_resource_type_t {
  L4VBUS_RESOURCE_INVALID = 0, /**< Invalid type */
  L4VBUS_RESOURCE_IRQ,         /**< Interrupt resource */
  L4VBUS_RESOURCE_MEM,         /**< I/O memory resource */
  L4VBUS_RESOURCE_PORT,        /**< I/O port resource (x86 only) */
  L4VBUS_RESOURCE_BUS,         /**< Bus resource */
  L4VBUS_RESOURCE_GPIO,        /**< Gpio resource */
  L4VBUS_RESOURCE_MAX,         /**< Maximum resource id */
};

enum l4vbus_consts_t {
  L4VBUS_DEV_NAME_LEN = 64,
  L4VBUS_MAX_DEPTH = 100,
};

typedef struct {
  int           type;                      /**< Type */
  char          name[L4VBUS_DEV_NAME_LEN]; /**< Name */
  unsigned      num_resources;             /**< Resources count */
  unsigned      flags;                     /**< Flags */
} l4vbus_device_t;

enum l4vbus_device_flags_t {
  L4VBUS_DEVICE_F_CHILDREN = 0x10,
};
