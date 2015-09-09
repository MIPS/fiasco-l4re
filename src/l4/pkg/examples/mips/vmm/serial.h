/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef _SERIAL_H
#define _SERIAL_H

#include "vmm.h"
#include "devices/devices_list.h"
#include "vm_driver/hypercall_payload.hpp"

void serial_do_hypcall(vz_vmm_t *vmm, HypercallPayload &hp);
int serial_init(vz_vmm_t *vmm);

#endif /* _SERIAL_H */
