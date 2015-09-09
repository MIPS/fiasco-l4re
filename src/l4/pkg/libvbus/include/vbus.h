/*
 * (c) 2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>,
 *          Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
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

enum {
  L4VBUS_NULL = 0,
  L4VBUS_ROOT_BUS = 0,
};

/**
 * \defgroup l4vbus_module L4 V-BUS functions
 * \{
 */

__BEGIN_DECLS

/**
 * \brief Find a device by the HID ACPI conforming or L4Io static name.
 *
 * \param  vbus         Capability of the system bus
 * \param  parent       Handle to the parent to start the search
 * \retval child        Handle to the found device
 * \param  hid          HID name of the device
 * \param  depth        Depth to look for
 * \retval devinfo      Device information structure (might be NULL)
 *
 * \return 0 on success, else failure
 */
int L4_CV
l4vbus_get_device_by_hid(l4_cap_idx_t vbus, l4vbus_device_handle_t parent,
                         l4vbus_device_handle_t *child, char const *hid,
                         int depth, l4vbus_device_t *devinfo);

/**
 * \brief Find next child following \a child.
 *
 * \param  vbus         Capability of the system bus
 * \param  parent       Handle to the parent device (use 0 for the system bus)
 * \param  child        Handle to the child device (use 0 to get the first
 *                      child)
 * \param  depth        Depth to look for
 * \retval devinfo      device information (might be NULL)
 *
 * \return 0 on success, else failure
 */
int L4_CV
l4vbus_get_next_device(l4_cap_idx_t vbus, l4vbus_device_handle_t parent,
                       l4vbus_device_handle_t *child, int depth,
                       l4vbus_device_t *devinfo);

/**
 * \brief Iterate over the resources of a device
 *
 * \param  vbus         Capability of the system bus
 * \param  dev          Handle of the device
 * \retval res_idx      Index of the resource, the number of resources is
 *                      available in the devinfo from get device functions.
 * \retval res          Descriptor of the resource
 *
 * \return 0 on success, else failure
 */
int L4_CV
l4vbus_get_resource(l4_cap_idx_t vbus, l4vbus_device_handle_t dev,
                    int res_idx, l4vbus_resource_t *res);


/**
 * \brief Check if the given device has a compatibility ID (CID) or HID that
 *        matches \a cid.
 *
 * \param  vbus V-BUS capability
 * \param  dev  device handle for which the CID shall be tested
 * \param  cid  the compatibility ID to test
 * \return 1 when the given ID (\a cid) matches the given device (\a dev),
 *         0 when the given ID does not match,
 *         <0 on error.
 */
int L4_CV
l4vbus_is_compatible(l4_cap_idx_t vbus, l4vbus_device_handle_t dev,
                     char const *cid);

/**
 * \brief Get the HID (hardware identifier) if a device
 *
 * \param  vbus         Capability of the system bus
 * \param  dev          Handle of the device
 * \param  hid          Pointer to a buffer for the HID string
 * \param  max_len      The size of the buffer (\a hid)
 *
 * \return the length of the HID string on success, else failure
 */
int L4_CV
l4vbus_get_hid(l4_cap_idx_t vbus, l4vbus_device_handle_t dev, char *hid,
               unsigned long max_len);

/**
 * \brief Request a resource of a specific type
 *
 * \param  vbus         Capability of the system bus
 * \param  res          Descriptor of the resource
 * \param  flags        Optional flags
 *
 * \return 0 on success, else failure
 *
 * If any resource is found that contains the requested
 * type and addresses this resource is returned.
 *
 * Flags are only relevant to control the memory caching.
 * If io-memory is requested.
 *
 * \return 0 on success, else failure
 */
int L4_CV
l4vbus_request_resource(l4_cap_idx_t vbus, l4vbus_resource_t *res,
                        int flags);

/**
 * \brief Release a previously requested resource
 *
 * \param  vbus         Capability of the system bus.
 * \param  res          Descriptor of the resource.
 *
 * \return 0 on success, else failure
 */
int L4_CV
l4vbus_release_resource(l4_cap_idx_t vbus, l4vbus_resource_t *res);

/**
 * \brief Get capability of ICU.
 *
 * \param  vbus         Capability of the system bus.
 * \param  icu          ICU device handle.
 * \param  cap          Capability slot for the capability.
 *
 * \return 0 on success, else failure
 */
int L4_CV
l4vbus_vicu_get_cap(l4_cap_idx_t vbus, l4vbus_device_handle_t icu,
                    l4_cap_idx_t cap);

__END_DECLS

/** \} */
