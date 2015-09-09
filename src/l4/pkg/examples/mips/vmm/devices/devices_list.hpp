/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/*
 * devices_list.hpp - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * devices_list.hpp
 *
 *  Created on: 14.02.2011
 *      Author: janis
 */

#ifndef DEVICES_LIST_HPP_
#define DEVICES_LIST_HPP_

#if 1 // KYMAXXX serial

#include "devices.hpp"
#include "devices_list.h"
#include "device_interface.h"
#include "../l4_ser.h"

DEFINE_GLOBAL_DEVICE(ser, L4_ser)

#else // KYMAXXX serial

#include "devices.hpp"
#include "devices_list.h"
#include "device_interface.h"
#include "../l4_vm.h"
#include "../l4_apic.h"
#include "../l4_ahci.h"
#include "../l4_bd.h"
#include "../l4_pci.h"
#include "../l4_fb.h"
#include "../l4_gic.h"
#include "../l4_mem.h"
#include "../l4_ser.h"
#include "../l4_timer.h"
#include "../l4_net.h"
#include <cstring>
#include <stdio.h>
#include "../l4_cpu.h"
#include "../util/cas.hpp"
#if defined(ARCH_x86)
#include <l4/sys/ktrace.h>
#endif

class CPUSpawner : public device::IDevice{
public:
    virtual void hypercall(HypercallPayload & hp){
        GET_VM.spawn_cpu(hp.reg(0), hp.reg(1), hp.reg(2));
    }

};

class KarmaDevice : public device::IDevice{
    unsigned int _jiffies;
    unsigned int _reports;
    unsigned long long _last_report_tsc;
public:
    KarmaDevice(): _jiffies(0), _reports(0), _last_report_tsc(util::tsc()){}
    void jiffiesUpdate(unsigned int jiffies){
        _jiffies += jiffies;
        ++_reports;
        fiasco_tbuf_log_3val("jiffy_report", 0, 0, 0);
        if(_jiffies >= 1000){
            unsigned long long tsc_diff, now = util::tsc();
            tsc_diff = now - _last_report_tsc;
            _last_report_tsc = now;
            double seconds = (double)tsc_diff / (double)(l4re_kip()->frequency_cpu * 1000);
            double report_size = (double)_jiffies / (double) _reports;
            if(seconds > 0){
                printf("jiffy freq %lf Hz in the last %lf seconds average report %lf\n", (double)_jiffies / seconds, seconds, report_size);
            }
            _jiffies = 0;
            _reports = 0;
        }
    }
    virtual void hypercall(HypercallPayload & hp){
        const char * str_ptr;
        l4_uint32_t tmp;
        switch(hp.address()) {
        case karma_df_printk:
            try {
                str_ptr = (char*)GET_VM.mem().base_ptr() + hp.reg(0);
                if(strlen(str_ptr) <= 2)
                    printf("%s", str_ptr);
                else
                    printf("CPU%d: %s", current_cpu().id(), str_ptr);
            }catch(...){} // TODO what's with the try catch block ??? //nullpointers
            break;
        case karma_df_panic:
            try {
                printf("Guest panic: %s\n", (char*)(GET_VM.mem().base_ptr() + hp.reg(0)));
            } catch(...) {}//nullpointers
            throw L4_PANIC_EXCEPTION;
            break;
        case karma_df_debug:
            printf("DEBUG: addr=%lx\n", hp.reg(0));
            break;
        case karma_df_get_time:
            l4rtc_get_seconds_since_1970(&tmp);
            hp.reg(0) = tmp;
            break;
        case karma_df_get_khz_cpu:
            hp.reg(0) = l4re_kip()->frequency_cpu;
            break;
        case karma_df_get_khz_bus:
            hp.reg(0) = l4re_kip()->frequency_bus;
            break;
        case karma_df_oops:
            printf("Guest reports Oops\n");
            break;
        case karma_df_report_jiffies_update:
            jiffiesUpdate(hp.reg(0));
            break;
        case karma_df_log3val:
            fiasco_tbuf_log_3val("guest_log", hp.reg(0), hp.reg(1), hp.reg(2));
            break;
        default:
            hp.reg(0) = -1UL;
            break;
        }
    }
};

DEFINE_GLOBAL_DEVICE(karma, KarmaDevice)
DEFINE_GLOBAL_DEVICE(mem, L4_mem)
DEFINE_GLOBAL_DEVICE(vm, CPUSpawner)
DEFINE_LOCAL_DEVICE(apic, L4_apic)
DEFINE_GLOBAL_DEVICE(ahci, L4_ahci)
DEFINE_GLOBAL_DEVICE(bd, L4_bd)
DEFINE_GLOBAL_DEVICE(pci, L4_pci)
DEFINE_GLOBAL_DEVICE(fb, L4_fb)
DEFINE_GLOBAL_DEVICE(gic, L4_gic)
DEFINE_GLOBAL_DEVICE(ser, L4_ser)
DEFINE_GLOBAL_DEVICE(timer, L4_timer)
DEFINE_GLOBAL_DEVICE(net, L4_net)

// Dummy definition of "arch"-device
// architecture specifics will be intercepted directly by the vm_driver
DEFINE_DUMMY_DEVICE(arch)
#endif // KYMAXXX serial

#endif /* DEVICES_LIST_HPP_ */
