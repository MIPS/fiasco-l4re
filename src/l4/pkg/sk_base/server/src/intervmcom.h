/*

Copyright (c) 2015, Elliptic Technologies Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

