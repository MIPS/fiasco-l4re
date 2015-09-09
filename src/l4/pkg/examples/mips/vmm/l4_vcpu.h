/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/*
 * l4_vcpu.h - TODO enter description
 * 
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * l4_vcpu.h
 *
 *  Created on: 24.09.2010
 *      Author: janis
 */

#ifndef L4_VCPU_H_
#define L4_VCPU_H_

#include <l4/sys/thread>
#include <l4/sys/vcpu.h>
#include "thread.hpp"
#include <l4/sys/vm>
#include <vector>
#include "l4_lirq.h"
//#include "util/local_statistics.hpp"

// KYMAXXX serial - hacked to only keep vcpu_specifics_key_t used by devices.cc
class L4_lirq;
class L4_vcpu {
public:
    class Specific {
    public:
        virtual ~Specific(){}
    };
    typedef unsigned long vcpu_specifics_key_t;
protected:
    L4::Cap<L4::Thread> _vcpu_thread;
    l4_vcpu_state_t * _vcpu_state;
    l4_addr_t _ext_state;

    typedef std::vector<Specific *> specifics_list_t;
    specifics_list_t _specifics;
    static vcpu_specifics_key_t _specifics_first_free_key;
public:
    static vcpu_specifics_key_t specifics_create_key();
    Specific * getSpecific(const vcpu_specifics_key_t key);
    void setSpecific(const vcpu_specifics_key_t key, Specific * value);
public:
    L4_vcpu(unsigned prio, l4_umword_t affinity = -1);
    virtual ~L4_vcpu();

};

extern const pthread_key_t PTHREAD_VCPU_THIS;

inline static L4_vcpu & current_vcpu(){
    return *reinterpret_cast<L4_vcpu *>(pthread_getspecific(PTHREAD_VCPU_THIS));
}


#endif /* L4_VCPU_H_ */
