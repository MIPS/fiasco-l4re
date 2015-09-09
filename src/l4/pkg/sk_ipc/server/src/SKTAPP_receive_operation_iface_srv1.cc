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

#include <cstdio>

#include <l4/sk_ipc/trustlet_srv1.h>

#define _4_BYTE_XOR_PATTERN 0xAABBCCDD


int Trustlet_server_obj::SKTAPP_receive_operation_iface(l4_umword_t cmd, sk_operation *op)
{
  int k;  
#ifdef DEBUG
  printf("%s\n", __func__);

  printf("op->param[0].mem.addr: 0x%08x, 0x%08x\n", op->param[0].mem.addr, *((l4_uint32_t*)op->param[0].mem.addr));
#endif  
  l4_uint32_t packet_size = op->param[0].mem.memory_size;
  l4_uint32_t *data = (l4_uint32_t *)op->param[0].mem.addr;

  if(packet_size <= 4096)
  {
     for(k=0;k<(packet_size/4);k++)
     {
       data[k] ^= _4_BYTE_XOR_PATTERN; 
     }
  }  

  return 0;
}

int Trustlet_server_obj::SKTAPP_init_iface()
{
#ifdef DEBUG
  printf("%s\n", __func__);
#endif
}


int Trustlet_server_obj::SKTAPP_receive_operation_iface2(l4_umword_t cmd, sk_operation *op)
{
  printf("%s\n", __func__);

  int k;  
#ifdef DEBUG
  printf("%s\n", __func__);

  printf("op->param[0].mem.addr: 0x%08x, 0x%08x\n", op->param[0].mem.addr, *((l4_uint32_t*)op->param[0].mem.addr));
#endif  
  l4_uint32_t packet_size = op->param[0].mem.memory_size;
  l4_uint32_t *data = (l4_uint32_t *)op->param[0].mem.addr;

  if(packet_size <= 4096)
  {
     for(k=0;k<(packet_size/4);k++)
     {
       data[k] ^= _4_BYTE_XOR_PATTERN; 
     }
  }  
  return 0;
}


int Trustlet_server_obj::SKTAPP_receive_operation_iface3(l4_umword_t cmd, sk_operation *op)
{
  printf("%s\n", __func__);

  int k;  
#ifdef DEBUG
  printf("%s\n", __func__);

  printf("op->param[0].mem.addr: 0x%08x, 0x%08x\n", op->param[0].mem.addr, *((l4_uint32_t*)op->param[0].mem.addr));
#endif  
  l4_uint32_t packet_size = op->param[0].mem.memory_size;
  l4_uint32_t *data = (l4_uint32_t *)op->param[0].mem.addr;

  if(packet_size <= 4096)
  {
     for(k=0;k<(packet_size/4);k++)
     {
       data[k] ^= _4_BYTE_XOR_PATTERN; 
     }
  }  
  return 0;
}
