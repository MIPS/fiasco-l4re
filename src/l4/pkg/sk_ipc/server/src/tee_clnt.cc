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
#include <l4/re/dataspace>
#include <l4/re/rm>
#include <l4/re/env>
#include <l4/sys/cache.h>
#include <cstdio>
#include <l4/util/rand.h>

#include <l4/sk_ipc/sk_types.h>

#include <l4/sk_ipc/tee_trustlet_iface.h>


void init_bytes(l4_uint32_t *bytes, unsigned long long size, l4_uint32_t seed);

void init_bytes(l4_uint32_t *bytes, unsigned long long size, l4_uint32_t seed)
{
    unsigned long long i;

    for(i = 0; i < size; i++) 
    {    
        bytes[i] = i+seed;
    }    
}



int main()
{

  int i;

  sk_operation my_sk_operation;

  Trustlet trustlet1("trustlet1", 0);
  
  Trustlet trustlet2("trustlet2", 1);
  
  Trustlet trustlet3("trustlet3", 2);


  my_sk_operation.num_param = 4;

  l4_uint32_t packet_size = (l4util_rand() % 4095) + 1;  
  my_sk_operation.param[0].mem.addr = trustlet1.get_shared_mem();
  my_sk_operation.param[0].mem.memory_size = packet_size;
  
  l4_uint32_t *mydata = ((l4_uint32_t *)trustlet1.get_shared_mem());  
  l4_uint32_t *mydata2 = ((l4_uint32_t *)trustlet2.get_shared_mem());
  
  l4_uint32_t *mydata3 = ((l4_uint32_t *)trustlet3.get_shared_mem());

  init_bytes(mydata, packet_size/4, l4util_rand() % 4096); 
 
  for(i=0;i<(packet_size/16); i++) 
  {    
      printf("%08x:  0x%08x 0x%08x 0x%08x 0x%08x\n", 
              i,
              mydata[0+4*i], 
              mydata[1+4*i],
              mydata[2+4*i],
              mydata[3+4*i]);
  }
    
  trustlet1.receive_operation_iface(2, &my_sk_operation);  

  printf("Return data:\n");

  for(i=0;i<(packet_size/16); i++) 
  {    
      printf("%08x:  0x%08x 0x%08x 0x%08x 0x%08x\n", 
              i,
              mydata[0+4*i], 
              mydata[1+4*i],
              mydata[2+4*i],
              mydata[3+4*i]);
  }    


  printf("\n \nSending to trustlet2.....\n \n");
  packet_size = (l4util_rand() % 4095) + 1;  
  my_sk_operation.param[0].mem.addr = trustlet2.get_shared_mem();
  my_sk_operation.param[0].mem.memory_size = packet_size;
  

  init_bytes(mydata2, packet_size/4, l4util_rand() % 4096); 
 
  for(i=0;i<(packet_size/16); i++) 
  {    
      printf("%08x:  0x%08x 0x%08x 0x%08x 0x%08x\n", 
              i,
              mydata2[0+4*i], 
              mydata2[1+4*i],
              mydata2[2+4*i],
              mydata2[3+4*i]);
  }

  trustlet2.receive_operation_iface(2, &my_sk_operation);

  
  printf("Return data:\n");

  for(i=0;i<(packet_size/16); i++) 
  {    
      printf("%08x:  0x%08x 0x%08x 0x%08x 0x%08x\n", 
              i,
              mydata2[0+4*i], 
              mydata2[1+4*i],
              mydata2[2+4*i],
              mydata2[3+4*i]);
  }    

  
  printf("\n \nSending to trustlet3.....\n \n");
  packet_size = (l4util_rand() % 4095) + 1;  
  my_sk_operation.param[0].mem.addr = trustlet3.get_shared_mem();
  my_sk_operation.param[0].mem.memory_size = packet_size;
  

  init_bytes(mydata3, packet_size/4, l4util_rand() % 4096); 
 
  for(i=0;i<(packet_size/16); i++) 
  {    
      printf("%08x:  0x%08x 0x%08x 0x%08x 0x%08x\n", 
              i,
              mydata3[0+4*i], 
              mydata3[1+4*i],
              mydata3[2+4*i],
              mydata3[3+4*i]);
  }

  trustlet3.receive_operation_iface(2, &my_sk_operation);

  
  printf("Return data:\n");

  for(i=0;i<(packet_size/16); i++) 
  {    
      printf("%08x:  0x%08x 0x%08x 0x%08x 0x%08x\n", 
              i,
              mydata3[0+4*i], 
              mydata3[1+4*i],
              mydata3[2+4*i],
              mydata3[3+4*i]);
  }    

  return 0;

}
