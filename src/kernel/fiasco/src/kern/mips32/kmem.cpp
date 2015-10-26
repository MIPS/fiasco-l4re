/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "paging.h"

class Page_table;

class Kmem : public Mem_layout
{
public:
  static Pdir *kdir();
  static Pdir *dir();

  static Mword *kernel_sp();
  static void kernel_sp(Mword *);

  static Mword is_kmem_page_fault( Mword pfa, Mword error );
  static Mword is_io_bitmap_page_fault( Mword pfa );

  static Address virt_to_phys(const void *addr);
private:
  static Pdir *_kdir;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "paging.h"
#include "panic.h"

char kernel_page_directory[sizeof(Pdir)];
Pdir *Kmem::_kdir = (Pdir *)&kernel_page_directory;

IMPLEMENT inline
Pdir *Kmem::kdir()
{ return _kdir; }

IMPLEMENT inline
Pdir *Kmem::dir()
{ return _kdir; }

IMPLEMENT inline
Mword *Kmem::kernel_sp()
{ return reinterpret_cast<Mword*>(read_c0_errorepc()); }

IMPLEMENT inline
void Kmem::kernel_sp(Mword *sp)
{ write_c0_errorepc(sp); }

IMPLEMENT inline NEEDS["paging.h"]
Address Kmem::virt_to_phys(const void *addr)
{
  Address a = reinterpret_cast<Address>(addr);

  if (EXPECT_TRUE(Mem_layout::in_pmem(a)))
    return a;

  return kdir()->virt_to_phys(a);
}

IMPLEMENT inline
Mword Kmem::is_kmem_page_fault(Mword pfa, Mword /*error*/)
{
  return in_kernel(pfa);
}

IMPLEMENT inline
Mword Kmem::is_io_bitmap_page_fault( Mword /*pfa*/ )
{
  return 0;
}

PUBLIC static
Address
Kmem::mmio_remap(Address phys)
{
  (void)phys;
#warning mips32 arch Kmem::mmio_remap not implemented
  // implement me
  return phys;
}
