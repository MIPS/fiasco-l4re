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
#include <l4/re/util/cap_alloc>
#include <cstdio>

#include <l4/sk_ipc/tee_trustlet_iface.h>

l4_addr_t const Trustlet::shared_addresses[] = { ADDRESS1, ADDRESS2, ADDRESS3 };

Trustlet::Trustlet(char const *trustlet_name, int mail_box)
{
  //printf("L4_PAGESIZE: %d\n", L4_PAGESIZE);
  //printf("DS_SIZE: %d\n", DS_SIZE); 
  _addr = shared_addresses[mail_box];
  //l4_addr_t _addr2;
  l4_umword_t r = L4Re::Env::env()->rm()->reserve_area(&_addr, DS_SIZE, L4Re::Rm::In_area|L4Re::Rm::Reserved, L4_PAGESHIFT);

  if (r != 0 ) 
  {
    printf("reserve area failed\n");
  }
  else
  {
#ifdef DEBUG   
     printf("reserved address: 0x%08x[%lu]\n", _addr, DS_SIZE);
#endif
  }
 #if 0
  int i;
  for (i=0; i<3; i++)
  {
    r = L4Re::Env::env()->rm()->reserve_area(&_addr2, L4_PAGESIZE, L4Re::Rm::Search_addr|L4Re::Rm::Reserved, L4_PAGESHIFT);

    if (r != 0 ) 
    {
      printf("reserve second area failed\n");
    }
    else
    {
#ifdef DEBUG   
       printf("reserved second address: 0x%08x\n", _addr2);
#endif
    }
  }
#endif
  _ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
  if (!_ds.is_valid())
  {
    printf("Could not get capability slot!\n");
    //return 1;
  }

  svr = L4Re::Env::env()->get_cap<Sktapp_interface>(trustlet_name);
  if(!svr.is_valid())
  {
      printf("Could not get server capability for %s\n", trustlet_name);
      //return 1;
  }

  if (svr->sktapp_init_iface(_ds))
  {
    printf("sktapp_init_iface() failed\n");
    //return 1;
  }

#ifdef DEBUG
  printf("Attempting to attach dataspace of size: %d at 0x%08x\n", _ds->size(), _addr);
#endif
  int err = L4Re::Env::env()->rm()->attach(&_addr, _ds->size(),
                                           L4Re::Rm::In_area, _ds);  
 /* 
  int err = L4Re::Env::env()->rm()->attach(&_addr, _ds->size(),
                                           L4Re::Rm::Search_addr, _ds);  
  */
  if (err < 0)
  {
    printf("Error attaching data space: %s\n", l4sys_errtostr(err));
    //return 1;
  }
  else
  {
#ifdef DEBUG
    printf("Dataspace attached at virtual address: 0x%08x\n",_addr);
#endif
  }

}


Trustlet::~Trustlet()
{

  int err;

  
  err = L4Re::Env::env()->rm()->detach(_addr, 0);
  if (err)
     printf("Failed to detach region\n");

  L4Re::Util::cap_alloc.free(_ds, L4Re::This_task);
}

int Trustlet::receive_operation_iface(l4_umword_t cmd, sk_operation* op) throw()
{
  svr->sktapp_receive_operation_iface(cmd, op);  
}
