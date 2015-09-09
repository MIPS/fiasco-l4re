/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/*
 * devices_list.h - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * devices_list.h
 *
 *  Created on: 14.02.2011
 *      Author: janis
 */

#ifndef DEVICES_LIST_H_
#define DEVICES_LIST_H_

#define KARMA_DEVICE_ID(name) KARMA_HYC_##name
#define KARMA_HYPERCALL_DEVICES_LIST typedef enum{
#define KARMA_DECLARE_HYPERCALL_DEVICE(name) KARMA_DEVICE_ID(name),
#define KARMA_HYPERCALL_DEVICES_LIST_END MAX_HYC} KARMA_HYC_items_t;

KARMA_HYPERCALL_DEVICES_LIST
KARMA_DECLARE_HYPERCALL_DEVICE(ser)
KARMA_HYPERCALL_DEVICES_LIST_END

#define L4_SER_BASE 0x0
#define L4_SER_INIT                 L4_SER_BASE + 0x0
#define L4_SER_READ                 L4_SER_BASE + 0x4
#define L4_SER_WRITE                L4_SER_BASE + 0x8
#define L4_SER_FLUSH                L4_SER_BASE + 0xc
#define L4_SER_WRITLN               L4_SER_BASE + 0x6

#define KARMA_MAKE_COMMAND(device, addr)\
        (unsigned long)(((addr) & 0xffff) | (((device) & 0xff) << 16) | 0x80000000)

#endif /* DEVICES_LIST_H_ */
