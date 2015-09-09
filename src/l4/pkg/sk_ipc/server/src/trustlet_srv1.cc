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

#include <l4/re/env>
#include <l4/re/namespace>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/re/dataspace>
#include <l4/cxx/ipc_server>

#include <l4/sys/typeinfo_svr>

#include <cstdio>

#include <l4/sk_ipc/trustlet_srv1.h>


int Trustlet_server_obj::dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios)
{
  
  sk_operation my_sk_operation;
  l4_umword_t cmd;  

  l4_msgtag_t t;
  ios >> t;

  switch (t.label())
  {
    case L4::Meta::Protocol:
        // handle the meta protocol requrests, implementing the
        // runtime dynamic type system for L4 objects.
        return L4::Util::handle_meta_request<Sktapp_interface>(ios);
        break;

    case Protocol::Tee_trustlet:

        L4::Opcode opcode;
        ios >> opcode;

        switch (opcode)
        {
          case Opcode::init_iface:
              SKTAPP_init_iface();
              ios << _shm;
            return L4_EOK;
            break;

          case Opcode::close_iface:

            return L4_EOK;
            break;

          case Opcode::receive_operation_iface:
              ios >> cmd;

              ios.get(my_sk_operation);
#ifdef DEBUG
              printf("cmd: %d\n", cmd);
              printf("num_param: %d\n", my_sk_operation.num_param);
              printf("param[0].mem.addr: 0x%08x\n", my_sk_operation.param[0].mem.addr);
              printf("param[0].mem.memory_size: %d\n", my_sk_operation.param[0].mem.memory_size);
              printf("param[1].mem.addr: 0x%08x\n", my_sk_operation.param[1].mem.addr);
              printf("param[1].mem.memory_size: %d\n", my_sk_operation.param[1].mem.memory_size);
              printf("param[2].mem.addr: 0x%08x\n", my_sk_operation.param[2].mem.addr);
              printf("param[2].mem.memory_size: %d\n", my_sk_operation.param[2].mem.memory_size);
              printf("param[3].mem.addr: 0x%08x\n", my_sk_operation.param[3].mem.addr);
              printf("param[3].mem.memory_size: %d\n", my_sk_operation.param[3].mem.memory_size);
#endif
              SKTAPP_receive_operation_iface(cmd, &my_sk_operation);            
            return L4_EOK;
            break;
       
          default:
    
            return -L4_ENOSYS;
        }

    default: 
        // every other protocol is not supported.
        return -L4_EBADPROTO;
  }

}


static L4Re::Util::Registry_server<> server;

#if 0
enum
{
  DS_SIZE = 4 << 12,
};
#endif

static l4_addr_t get_ds(L4::Cap<L4Re::Dataspace> *_ds, l4_addr_t _addr)
{
  *_ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!(*_ds).is_valid())
  {
    printf("Dataspace allocation failed.\n");
    return 0;
  }

  int err = L4Re::Env::env()->mem_alloc()->alloc(DS_SIZE, *_ds, 0);
  if (err < 0)
  {
    printf("mem_alloc->alloc() failed.\n");
    L4Re::Util::cap_alloc.free(*_ds);
    return 0;
  }

  // Attach DS to local address space
  err = L4Re::Env::env()->rm()->attach(&_addr, (*_ds)->size(),
                                       L4Re::Rm::In_area,
                                       *_ds);
  if (err < 0)
  {
    printf("Error attaching data space: %s\n", l4sys_errtostr(err));
    L4Re::Util::cap_alloc.free(*_ds);
    return 0;
  }

  return _addr; 
}

int main()
{
  L4::Cap<L4Re::Dataspace> ds;
  l4_addr_t addr;
  l4_addr_t a = ADDRESS1;

  l4_umword_t r = L4Re::Env::env()->rm()->reserve_area(&a, L4_PAGESIZE, L4Re::Rm::In_area|L4Re::Rm::Reserved, L4_PAGESHIFT);

  if (r != 0 )
  {
    printf("reserve area failed\n");
  }
  else
  {
#ifdef DEBUG
    printf("reserved address: 0x%08x\n", a);
#endif
  }
  

  if (!(addr = get_ds(&ds, a)))
    return 2;


  Trustlet_server_obj trustlet_server(ds);

  server.registry()->register_obj(&trustlet_server, "trustlet1");

  server.loop();
}

