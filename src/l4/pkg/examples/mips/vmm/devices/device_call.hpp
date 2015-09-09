/*
 * device_call.hpp - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * device_call.hpp
 *
 *  Created on: 14.02.2011
 *      Author: janis
 */

#ifndef DEVICE_CALL_HPP_
#define DEVICE_CALL_HPP_

#include "device_interface.h"
#include "devices.hpp"

#define DO_HYPERCALL(payload) device::lookupDevice(payload.deviceID()).hypercall(payload)

#define HYPERCALL_DEVICE_TYPE(name) device::DeviceImplementationType<device::_long_type<DEVICE_ID(name)> >::type

#define GET_HYPERCALL_DEVICE(name) static_cast<HYPERCALL_DEVICE_TYPE(name)&>(device::lookupDevice(DEVICE_ID(name)))

#endif /* DEVICE_CALL_HPP_ */
