/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef _L4_SYS_MEM_LAYOUT_H
#define _L4_SYS_MEM_LAYOUT_H

#define L4RE_KIP_ADDR 0x7fff0000

#ifndef __ASSEMBLY__
namespace Mem_layout
{
  /*
   * For MIPS move the default base from 0xb0000000 -> 0x60000000 to
   * make it user accessible.  Other mips memory defines are located in:
   *  l4re_kernel Makefile: DEFAULT_RELOC_mips
   */
  enum
  {
    Utcb_area_start   = 0x63000000,
    Stack_base        = 0x61000000,
    Loader_vma_start  = 0x60200000,
    Heap_base         = 0x60100000,
    Initial_tls_base  = 0x60000000,
    Kip_address       = L4RE_KIP_ADDR,
  };

}

#endif /* !__ASSEMBLY__ */
#endif /* _L4_SYS_MEM_LAYOUT_H */
