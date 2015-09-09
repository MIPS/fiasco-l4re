/**
 *
 * Copyright  (c) 2015 Elliptic Technologies
 *
 * \author  Jason Butler jbutler@elliptictech.com
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 */

#pragma once

#include <l4/sys/ipc.h>
#include <l4/sys/capability>
#include <l4/re/rm>
#include <l4/karma/karma_devices.h>


class IntervmCom {

    public:
        IntervmCom(unsigned long size_in_bytes);
        void karma_intervm_write(unsigned long opcode, unsigned long val);
        unsigned long karma_intervm_read(unsigned long opcode);
        void intervm_blocking_read(int bytes_to_read, char *buffer);
        void* get_write_channel();
        void* get_read_channel(); 

    private:
        L4::Cap<L4Re::Dataspace> _ds_cap;
        void *virt;
        L4::Cap<void> kintervm;
        l4_size_t region_size;
        l4_addr_t physical_address, physical_address2;
        unsigned long memory_size;

        L4_INLINE l4_msgtag_t
          l4_intervm_send_u(l4_cap_idx_t intervm, unsigned long opcode, unsigned long address1, l4_utcb_t *utcb) L4_NOTHROW;

        L4_INLINE l4_msgtag_t
            l4_intervm_send2_u(l4_cap_idx_t intervm, unsigned long opcode, unsigned long address1, unsigned long address2, l4_utcb_t *utcb) L4_NOTHROW;

        L4_INLINE l4_msgtag_t
            l4_intervm_read_u(l4_cap_idx_t intervm, unsigned long opcode, unsigned long &read_val, l4_utcb_t *utcb) L4_NOTHROW;

};

