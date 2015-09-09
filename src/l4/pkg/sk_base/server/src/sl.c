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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/types.h>  // temporary, for test fat trustlet (dead code)
#include <sys/socket.h> // temporary, for test fat trustlet (dead code)

#define STDOUT_FILE        1
#define STDERR_FILE        2
#define MAX_PRINTF_LEN     2048

#define DATABASE_FD        10
#define TOC_FD             11
#define TX_DEV_FD          12
#define RX_DEV_FD          13
#define TX_OUT_FD          14

#define READ_BUF_LEN 4096

#include "intervmcom.cc"

static IntervmCom ivm(READ_BUF_LEN);

//------------------------------------------------------------------------
//                        Hypervisor Calls
//------------------------------------------------------------------------

static int read_counter = 0, write_counter = 0;

static void intervm_blocking_read(int bytes_to_read, void *buffer)
{
   int x;
   unsigned char *p = (unsigned char*)buffer;
   ivm.intervm_blocking_read(bytes_to_read, (char *)buffer); 
#if 0
   printf("Read #%d of %d bytes...\n", ++read_counter, bytes_to_read);
   for (x = 0; x < bytes_to_read; ) {
     printf("%02x ", p[x]);
     if (!(++x & 15)) { printf("\n"); }
   }
   printf("\n");
#endif
}

static void intervm_write(int bytes_to_write, const void *buffer)
{
  int x;
  unsigned char *p = (unsigned char *)buffer;
#if 0
  printf("Writing #%d of %d bytes...\n", ++write_counter, bytes_to_write);
  for (x = 0; x < bytes_to_write; ) {
      printf("%02x ", p[x]);
      if (!(++x & 15)) { printf("\n"); }
  }
  printf("\n");
#endif
   __builtin_memcpy(ivm.get_write_channel(), buffer, bytes_to_write);
  ivm.karma_intervm_write(karma_shmem_ds_write, bytes_to_write);
}

extern "C" {

// TODO
static char *tx_dev_path = "/dev/karma_chr_intervm_tx";
static char *rx_dev_path = "/dev/karma_chr_intervm_rx";


void _IO_putc(int ch, void *foo)
{
   char t[2] = { ch, 0 }; 
   printf(t);
}

int close2(int fd)
{
   // Nothing to be done
   
   return 0;
}

int open2(const char *pathname, int flags, ...)
{
   if (!strcmp(pathname, tx_dev_path))
   {
      return RX_DEV_FD;
   }
   else if (!strcmp(pathname, rx_dev_path))
   {
      return TX_DEV_FD;
   }

   return -1;
}

ssize_t read2(int fd, void *buf, size_t count)
{
   if (fd != RX_DEV_FD)
   {
      return -1;
   }

   //printf("Reading %u bytes (inter-vm)...\n", count);
   intervm_blocking_read(count, buf);
   //printf("Read %u bytes...\n", count);

   return count;
}

ssize_t write2(int fd, const void *buf, size_t count)
{
   if (fd != TX_DEV_FD)
   {
      return -1;
   }

   //printf("Writing %u bytes (inter-vm)...\n", count);
   intervm_write(count, buf);
   //printf("Wrote %u bytes...\n", count);

   return count;
}

}
