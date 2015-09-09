#ifndef _MEM_MAN_H
#define _MEM_MAN_H

#include "vmm.h"

enum memmap_rw {
  memmap_readonly = 0,
  memmap_write = 1,
};

enum memmap_io {
  memmap_not_io = 0,
  memmap_io = 1,
};

int allocate_mem(unsigned long size_in_bytes, unsigned long flags, void **virt_addr);
int free_mem(void *virt_addr);
l4_addr_t map_mem(vz_vmm_t *vmm, l4_addr_t dest, l4_addr_t source,
                  unsigned int size, enum memmap_rw write, enum memmap_io io);

#endif /* _MEM_MAN_H */
