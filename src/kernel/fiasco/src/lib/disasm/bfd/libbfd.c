#include "libbfd.h"
#include "bfd.h"

bfd_vma
bfd_getb32 (const void *p)
{
  const bfd_byte *addr = p;
  unsigned long v;

  v = (unsigned long) addr[0] << 24;
  v |= (unsigned long) addr[1] << 16;
  v |= (unsigned long) addr[2] << 8;
  v |= (unsigned long) addr[3];
  return v;
}

bfd_vma
bfd_getl32 (const void *p)
{
  const bfd_byte *addr = p;
  unsigned long v;

  v = (unsigned long) addr[0];
  v |= (unsigned long) addr[1] << 8;
  v |= (unsigned long) addr[2] << 16;
  v |= (unsigned long) addr[3] << 24;
  return v;
}

bfd_vma
bfd_getb16 (const void *p)
{
  const bfd_byte *addr = p;
  return (addr[0] << 8) | addr[1];
}

bfd_vma
bfd_getl16 (const void *p)
{
  const bfd_byte *addr = p;
  return (addr[1] << 8) | addr[0];
}

