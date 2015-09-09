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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include <l4/re/util/cap_alloc>
#include <l4/re/dataspace>
#include <l4/re/rm>
#include <l4/re/env>
#include <l4/sys/cache.h>
#include <cstdio>
#include <l4/util/rand.h>

#include <sk_swld.h>
#include <l4/sk_ipc/sk_types.h>
#include <l4/sk_ipc/tee_trustlet_iface.h>



extern "C" {

    void sw_main(int, char*);

extern void ELP_ERROR(void);
void ELP_ERROR(void)
{
}

Trustlet trustlet1("trustlet1", 0);
Trustlet trustlet2("trustlet2", 1);

//
// Testing only - Proper trustlet required
//
SK_TA_STATUS SKTAPP_init_iface(void* session)
{
   return 0;
}

SK_TA_STATUS SKTAPP_close_iface(void* session)
{
   return 0;
}

static const uint8_t taids[3][SK_TAPI_SESSION_TAID_SIZE] = { 
  { 0x12, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
  { 0x13, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
};

SK_TA_STATUS SKTAPP_receive_operation_iface(uint8_t *taid, void* session, uint32_t cmd, sk_operation* op)
{
   sk_operation local_op;
   l4_addr_t off;
   unsigned n;
   Trustlet *tfoo;

   tfoo = NULL;
   if (!memcmp(taid, taids[0], SK_TAPI_SESSION_TAID_SIZE)) {
      tfoo = &trustlet1;
   } else if (!memcmp(taid, taids[1], SK_TAPI_SESSION_TAID_SIZE)) {
      tfoo = &trustlet2;
   } else {
      printf("Invalid unsupported and generally just bad TAID.\n");
      return -1;
   }

   // we need to clone the op and then memcpy data to shared buffers...
   off = tfoo->get_shared_mem();
   local_op = *op;
   for (n = 0; n < op->num_param; n++) {
      intptr_t val = off;
      if (val & 3) { 
         val += 4 - (val & 3);
         off = val;
      }
      local_op.param[n].mem.addr = off; 
      memcpy((void*)local_op.param[n].mem.addr, (void*)op->param[n].mem.addr, op->param[n].mem.memory_size);
      off += op->param[n].mem.memory_size;
   }

   // do call
   tfoo->receive_operation_iface(cmd, &local_op);
  
   // copy out
   for (n = 0; n < op->num_param; n++) {
      memcpy((void*)op->param[n].mem.addr, (void*)local_op.param[n].mem.addr, local_op.param[n].mem.memory_size);
      op->param[n].mem.memory_size = local_op.param[n].mem.memory_size;
   }

   // Always return SUCCESS otherwise SK TAPI exits
   return 0;
}
   
void SKTAPP_getapi(sktapp_func_def *api)
{
   api->SKTAPP_init_iface = SKTAPP_init_iface;
   api->SKTAPP_close_iface = SKTAPP_close_iface;
   api->SKTAPP_receive_operation_iface = SKTAPP_receive_operation_iface;
}

}

#include "sl.c"

int main(void)
{
   for (;;) sw_main(0, NULL);
}
