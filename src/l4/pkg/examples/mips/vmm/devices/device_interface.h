/*
 * device_interface.h - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * device_interface.h
 *
 *  Created on: 14.02.2011
 *      Author: janis
 */

#ifndef DEVICE_INTERFACE_H_
#define DEVICE_INTERFACE_H_

#include "../vm_driver/hypercall_payload.hpp"

namespace device{

class IDevice{
public:
    virtual ~IDevice(){}

    virtual void hypercall(HypercallPayload &) = 0;

};

}
#endif /* DEVICE_INTERFACE_H_ */
