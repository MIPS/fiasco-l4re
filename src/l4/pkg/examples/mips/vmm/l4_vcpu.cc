/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/*
 * l4_vcpu.cc - TODO enter description
 * 
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * l4_vcpu.cc
 *
 *  Created on: 24.09.2010
 *      Author: janis
 */

// KYMAXXX serial - hacked to only keep vcpu_specifics_key_t used by devices.cc
#include "l4_vcpu.h"

L4_vcpu::vcpu_specifics_key_t L4_vcpu::_specifics_first_free_key = 0;

L4_vcpu::vcpu_specifics_key_t L4_vcpu::specifics_create_key(){
    return _specifics_first_free_key++;
}
L4_vcpu::Specific *  L4_vcpu::getSpecific(const vcpu_specifics_key_t key){
    return _specifics[key];
}
void  L4_vcpu::setSpecific(const vcpu_specifics_key_t key, Specific * value){
    _specifics[key] = value;
}

static void vcpu_destroy(void * _this){
    delete reinterpret_cast<L4_vcpu*>(_this);
}

static pthread_key_t init_PTHREAD_VCPU_THIS(){
    pthread_key_t key;
    pthread_key_create(&key, vcpu_destroy);
    return key;
}

const pthread_key_t PTHREAD_VCPU_THIS = init_PTHREAD_VCPU_THIS();


L4_vcpu::L4_vcpu(unsigned prio, l4_umword_t affinity)
:   _specifics(10)
{
    (void)prio;
    (void)affinity;
}

L4_vcpu::~L4_vcpu() {
    // TODO Auto-generated destructor stub
}
