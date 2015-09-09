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

#include <stdio.h>
#include <stdlib.h>
#include <cstdio>

#include <l4/re/env>
#include <l4/re/mem_alloc>
#include <l4/re/dataspace>
#include <l4/re/util/cap_alloc>
#include <l4/sys/ipc.h>
#include <l4/sys/capability>
#include <l4/karma/karma_devices.h>
#include <l4/util/util.h>
#include <l4/re/rm>
#include <l4/intervm/intervmcom.h>


L4_INLINE l4_msgtag_t
  l4_intervm_send_u(l4_cap_idx_t intervm, unsigned long opcode, unsigned long address1, l4_utcb_t *utcb) L4_NOTHROW;


L4_INLINE l4_msgtag_t
  l4_intervm_send2_u(l4_cap_idx_t intervm, unsigned long opcode, unsigned long address1, unsigned long address2, l4_utcb_t *utcb) L4_NOTHROW;


L4_INLINE l4_msgtag_t
  l4_intervm_read_u(l4_cap_idx_t intervm, unsigned long opcode, unsigned long &read_val, l4_utcb_t *utcb) L4_NOTHROW;

char my_buf[20] = "aaaaaaaa";
char hello_string[] = "hello world\n";

#define _4_BYTE_XOR_PATTERN 0xAABBCCDD

l4_uint32_t packet_size;
l4_uint32_t bytes_read_buf[1024];
 
int
main(void)
{

  int k;

  IntervmCom intervmcom(4096);

  for(;;)
  {
  intervmcom.intervm_blocking_read(sizeof(packet_size), (char*)&packet_size); 

  if(packet_size <= 4096)
  {  

      intervmcom.intervm_blocking_read(packet_size, (char*)bytes_read_buf);
      for(k=0;k<(packet_size/4);k++)
      {
          bytes_read_buf[k] ^= _4_BYTE_XOR_PATTERN;
      }

      __builtin_memcpy(intervmcom.get_write_channel(), bytes_read_buf, packet_size);
      intervmcom.karma_intervm_write(karma_shmem_ds_write, packet_size);
   }
  
  }
#if 0
  unsigned long number_of_bytes = sizeof(hello_string);
  //__builtin_memcpy(my_buf, &number_of_bytes, sizeof(number_of_bytes));

  __builtin_memcpy(intervmcom.get_write_channel(), hello_string, number_of_bytes);

  intervmcom.karma_intervm_write(karma_shmem_ds_write, number_of_bytes);
#endif
#if 0
  l4_intervm_send_u(kintervm.cap(), karma_shmem_ds_write, sizeof(hello_string), l4_utcb());

  l4_intervm_read_u(kintervm.cap(), karma_shmem_ds_read, read_value, l4_utcb());

  printf("read_value: 0x%08lx\n", read_value);

#endif
 
  exit(0);

}



