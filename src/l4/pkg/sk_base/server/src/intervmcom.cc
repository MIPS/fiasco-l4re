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
#include <stdio.h>

#include <l4/re/env>
#include <l4/re/mem_alloc>
#include <l4/re/dataspace>
#include <l4/re/util/cap_alloc>
#include <l4/sys/ipc.h>
#include <l4/sys/capability>
#include <l4/re/rm>
#include <l4/karma/karma_devices.h>
#include <l4/intervm/intervmcom.h>

#if 0
class IntervmCom {

    public:
        IntervmCom(unsigned long size_in_bytes);
        void karma_intervm_write(unsigned long opcode, unsigned long val);
        unsigned long karma_intervm_read(unsigned long opcode);
        void intervm_blocking_read(int bytes_to_read, char *buffer);

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

#endif


IntervmCom::IntervmCom(unsigned long size_in_bytes)
{
    unsigned long flags;
    int r;

    virt = 0;

    memory_size = size_in_bytes;

    flags = L4Re::Mem_alloc::Continuous|L4Re::Mem_alloc::Pinned;
    _ds_cap = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();

    if(!_ds_cap.is_valid())
    {
        printf("error allocating capability\n");
    }

    if((r = L4Re::Env::env()->mem_alloc()->alloc(size_in_bytes*2, _ds_cap,
            flags)))
    {
        printf("Error allocating enough memory for intervm: %d\n", r);
    }

    if ((r = L4Re::Env::env()->rm()->attach(&virt, size_in_bytes*2, L4Re::Rm::Search_addr, 
             _ds_cap, 0, flags))){
        printf("attach memory failed\n");
    }

    kintervm = L4Re::Env::env()->get_cap<void>("kintervm");

    _ds_cap->phys(0, physical_address, region_size);
    _ds_cap->phys(size_in_bytes, physical_address2, region_size);

    l4_intervm_send2_u(kintervm.cap(), karma_shmem_ds_init, physical_address, physical_address2, l4_utcb());
}

void IntervmCom::karma_intervm_write(unsigned long opcode, unsigned long val){
    l4_intervm_send_u(kintervm.cap(), opcode, val, l4_utcb());
}

void* IntervmCom::get_write_channel()
{
    return virt;
}

void* IntervmCom::get_read_channel()
{
   return (virt + memory_size);
}

unsigned long IntervmCom::karma_intervm_read(unsigned long opcode)
{
    unsigned long ret = 0;
    
    l4_intervm_read_u(kintervm.cap(), opcode, ret, l4_utcb());

    return ret; 
}

void IntervmCom::intervm_blocking_read(int bytes_to_read, char *buffer)
{
    l4_intervm_send_u(kintervm.cap(), karma_shmem_ds_shared_mem_read_baremetal, bytes_to_read, l4_utcb());
    __builtin_memcpy(buffer, virt + memory_size, bytes_to_read);
}

L4_INLINE l4_msgtag_t
IntervmCom::l4_intervm_send_u(l4_cap_idx_t intervm, unsigned long opcode, unsigned long address1, l4_utcb_t *utcb) L4_NOTHROW
{
   l4_msg_regs_t *mr = l4_utcb_mr_u(utcb);
   mr->mr[0] = 0;
   mr->mr[1] = opcode;
   mr->mr[2] = address1;
   return l4_ipc_send(intervm, utcb,
                     l4_msgtag(L4_PROTO_VM,
                               3,
                               0, L4_MSGTAG_SCHEDULE),
                               L4_IPC_NEVER);

}

L4_INLINE l4_msgtag_t
IntervmCom::l4_intervm_send2_u(l4_cap_idx_t intervm, unsigned long opcode, unsigned long address1, unsigned long address2, l4_utcb_t *utcb) L4_NOTHROW
{
   l4_msg_regs_t *mr = l4_utcb_mr_u(utcb);
   mr->mr[0] = 1;
   mr->mr[1] = opcode;
   mr->mr[2] = address1;
   mr->mr[3] = address2;
  return l4_ipc_send(intervm, utcb,
                     l4_msgtag(L4_PROTO_VM,
                               4,
                               0, L4_MSGTAG_SCHEDULE),
                               L4_IPC_NEVER);

}

L4_INLINE l4_msgtag_t
IntervmCom::l4_intervm_read_u(l4_cap_idx_t intervm, unsigned long opcode, unsigned long &read_val, l4_utcb_t *utcb) L4_NOTHROW
{
   l4_msgtag_t res;
   l4_msg_regs_t *mr = l4_utcb_mr_u(utcb);

    mr->mr[0] = 2;
    mr->mr[1] = opcode;

    res = l4_ipc_call(intervm, utcb,
                       l4_msgtag(L4_PROTO_VM,
                                  2,
                                  0, 0),
                                  L4_IPC_NEVER);

    read_val = mr->mr[2];

    return res;

}

