/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 */

#ifndef _LOADER_H
#define _LOADER_H

bool loader_elf_is_exec(l4_uint8_t * load, l4_uint32_t loadlen);
int loader_elf_load(l4_uint8_t * load, l4_uint32_t loadlen, l4_uint8_t *mem,
              l4_addr_t vaddr_offset, l4_addr_t *entrypoint);

#endif /* _LOADER_H */

