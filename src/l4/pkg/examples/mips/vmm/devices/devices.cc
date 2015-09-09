/*
 * devices.cc - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * devices.cc
 *
 *  Created on: 14.02.2011
 *      Author: janis
 */

#include "devices.hpp"
#include "device_interface.h"

namespace device {

static L4_vcpu::vcpu_specifics_key_t VCPU_LOCAL_DEVICES_KEY = L4_vcpu::specifics_create_key();

DevicesList & getLocalDevices(){
    return *static_cast<DevicesList*>(current_vcpu().getSpecific(VCPU_LOCAL_DEVICES_KEY));
}

void initLocalDevices(){
    current_vcpu().setSpecific(VCPU_LOCAL_DEVICES_KEY, new DevicesList());
}

class DummyDevice : public IDevice {
public:
    virtual ~DummyDevice(){

    }
    virtual void hypercall(HypercallPayload & hypercall){
        printf("not implemented id: 0x%lx\n", hypercall.deviceID());
        hypercall.dump();
    }
};

static DummyDevice dummyDevice;

IDevice & lookupDevice(unsigned long id){
    IDevice * retval = &util::Singleton<DevicesList>::get().lookupDevice(id);
    if(!retval){
        retval = &getLocalDevices().lookupDevice(id);
    }
    if(!retval){
        return dummyDevice;
    }
    return *retval;
}

}
