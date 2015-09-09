/*
 * devices.hpp - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * devices.hpp
 *
 *  Created on: 14.02.2011
 *      Author: janis
 */

#ifndef DEVICES_HPP_
#define DEVICES_HPP_

#include "../util/singleton.hpp"
#include "../l4_vcpu.h"

#define MAX_DEVICES 256

namespace device {
class IDevice;
class DevicesList;

extern DevicesList & getLocalDevices();
extern void initLocalDevices();

extern IDevice & lookupDevice(unsigned long id);

class DevicesList : public L4_vcpu::Specific{
private:
    typedef IDevice *device_table_t[MAX_DEVICES];
    device_table_t _device_table;

public:
    DevicesList(){
        for(unsigned long i(0); i != MAX_DEVICES; ++i){
            _device_table[i] = NULL;
        }
    }
    virtual ~DevicesList(){
    }

    IDevice & lookupDevice(unsigned long id) const{
        return *_device_table[id];
    }
    void setDevice(unsigned long id, IDevice * device){
        _device_table[id] = device;
    }
};



#define DEVICE_ID(name) KARMA_HYC_##name

template <unsigned int DEV_ID, typename IMPL>
class LocalDeviceTraits{
public:
    enum { id = DEV_ID };
    typedef IMPL impl_t;
    inline static impl_t * create(){
        return new impl_t();
    }
};

template <unsigned int DEV_ID, typename IMPL>
class GlobalDeviceTraits{
public:
    enum { id = DEV_ID };
    typedef IMPL impl_t;
    inline static impl_t * create(){
        static impl_t obj;
        return & obj;
    }
};

template <unsigned int DEV_ID, typename IMPL>
class NULLDeviceTraits{
public:
    enum { id = DEV_ID };
    typedef IMPL impl_t;
    inline static impl_t * create(){
        return NULL;
    }
};

template <unsigned int DEV_ID>
class InternalLocalDevTraits{

};
template <unsigned int DEV_ID>
class InternalGlobalDevTraits{

};

template <unsigned int DEV_ID, template <unsigned int _DEV_ID> class INTERNAL_DEV_TRAITS >
class DevicesInitializer{
private:
    typedef INTERNAL_DEV_TRAITS<DEV_ID - 1> dev_traits_t;
    typedef DevicesInitializer<DEV_ID - 1, INTERNAL_DEV_TRAITS> next_t;
public:
    static DevicesList & init(){
        DevicesList & devs = next_t::init();
        devs.setDevice(DEV_ID - 1, dev_traits_t::create());
        return devs;
    }
};

template <>
class DevicesInitializer<0, InternalLocalDevTraits>{
public:
    static DevicesList & init(){
        return getLocalDevices();
    }
};
template <>
class DevicesInitializer<0, InternalGlobalDevTraits>{
public:
    static DevicesList & init(){
        return util::Singleton<DevicesList>::get();
    }
};
template <unsigned long id>
struct _long_type{
    enum { value = id };
};
template <typename TypeID>
struct DeviceImplementationType{

};
#define LOCAL_DEVICES_INIT \
    device::initLocalDevices();\
    device::DevicesInitializer<(unsigned int)MAX_HYC, device::InternalLocalDevTraits>::init()

#define GLOBAL_DEVICES_INIT \
    device::DevicesInitializer<(unsigned int)MAX_HYC, device::InternalGlobalDevTraits>::init()

#define DEFINE_LOCAL_DEVICE(name, device_impl)\
    namespace device{\
    template<>\
    struct DeviceImplementationType<_long_type<DEVICE_ID(name)> >{\
        typedef device_impl type;\
    };\
    template<>\
    class InternalLocalDevTraits<DEVICE_ID(name)>\
    :   public LocalDeviceTraits<DEVICE_ID(name), device_impl>{\
    };\
    template<>\
    class InternalGlobalDevTraits<DEVICE_ID(name)>\
    :   public NULLDeviceTraits<DEVICE_ID(name), device_impl>\
    {\
    };}

#define DEFINE_GLOBAL_DEVICE(name, device_impl)\
    namespace device{\
    template<>\
    struct DeviceImplementationType<_long_type<DEVICE_ID(name)> >{\
        typedef device_impl type;\
    };\
    template<>\
    class InternalLocalDevTraits<DEVICE_ID(name)>\
    :   public NULLDeviceTraits<DEVICE_ID(name), device_impl>{\
    };\
    template<>\
    class InternalGlobalDevTraits<DEVICE_ID(name)>\
    :   public GlobalDeviceTraits<DEVICE_ID(name), device_impl>\
    {\
    };}

//class DummyDevicex{};
#define DEFINE_DUMMY_DEVICE(name)\
    namespace device{\
    template<>\
    struct DeviceImplementationType<_long_type<DEVICE_ID(name)> >{\
        typedef IDevice type;\
    };\
    template<>\
    class InternalLocalDevTraits<DEVICE_ID(name)>\
    :   public NULLDeviceTraits<DEVICE_ID(name), IDevice>{\
    };\
    template<>\
    class InternalGlobalDevTraits<DEVICE_ID(name)>\
    :   public NULLDeviceTraits<DEVICE_ID(name), IDevice>{\
    };}

} // namespace device

#endif /* DEVICES_HPP_ */
