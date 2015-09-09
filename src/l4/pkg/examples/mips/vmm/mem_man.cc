/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/**
 * Sample memory routines based on: examples/libs/l4re/c/ma+rm.c
 */

/**
 * \file
 * \brief  Example of coarse grained memory allocation, in C.
 */
/*
 * (c) 2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <l4/re/c/mem_alloc.h>
#include <l4/re/c/rm.h>
#include <l4/re/c/util/cap_alloc.h>
#include <l4/util/util.h>
#include <l4/sys/task.h>
#include <l4/sys/err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mem_man.h"
#include "debug.h"

/**
 * \brief Allocate memory, given in bytes in the granularity of pages.
 *
 * \param size_in_bytes   Size to allocate, in bytes, will be rounded up to
 *                          whole pages (L4_PAGESIZE).
 * \param flags           Flags to control memory allocation:
 *                          L4RE_MA_CONTINUOUS:  Physically continuous memory
 *                          L4RE_MA_PINNED:      Pinned memory
 *                          L4RE_MA_SUPER_PAGES: Use big pages
 * \retval virt_addr      Virtual address the memory is accessible under,
 *                          undefined if return code != 0
 *
 * \return 0 on success, error code otherwise
 */
int allocate_mem(unsigned long size_in_bytes, unsigned long flags, void **virt_addr)
{
    int r;
    //L4::Cap<L4Re::Dataspace> ds;
    l4re_ds_t ds;

    /* Allocate a free capability index for our data space */
    ds = l4re_util_cap_alloc();
    if (l4_is_invalid_cap(ds))
        return -L4_ENOMEM;

    size_in_bytes = l4_round_page(size_in_bytes);

    /* Allocate memory via a dataspace */
    if ((r = l4re_ma_alloc(size_in_bytes, ds, flags)))
        return r;

    /* Make the dataspace visible in our address space */
    *virt_addr = 0;
    if ((r = l4re_rm_attach(virt_addr, size_in_bytes,
                            L4RE_RM_SEARCH_ADDR, ds, 0,
                            flags & L4RE_MA_SUPER_PAGES
                                 ? L4_SUPERPAGESHIFT : L4_PAGESHIFT)))
        return r;

    /* Done, virtual address is in virt_addr */
    return 0;
}

/**
 * \brief Free previously allocated memory.
 *
 * \param virt_addr    Virtual address return by allocate_mem
 *
 * \return 0 on success, error code otherwise
 */
int free_mem(void *virt_addr)
{
    int r;
    l4re_ds_t ds;

    /* Detach memory from our address space */
    if ((r = l4re_rm_detach_ds(virt_addr, &ds)))
        return r;

    /* Free memory at our memory allocator */
    if ((r = l4re_ma_free(ds)))
        return r;

    l4re_util_cap_free(ds);

    /* All went ok */
    return 0;
}

/**
 * based on pkg/karma/server/src/l4_mem.cc
 */
l4_addr_t map_mem(vz_vmm_t *vmm, l4_addr_t dest, l4_addr_t source,
                  unsigned int size, enum memmap_rw write, enum memmap_io io)
{
    l4_msgtag_t msg;
    unsigned int mapped_size = 0;
    int pageshift = 0;
    int rights;

    if(write && !io)
        l4_touch_rw((void*)source, size);
    else
        l4_touch_ro((void*)source, size);
    if(write || io)
        rights = L4_FPAGE_RW;
    else
        rights = L4_FPAGE_RO;

    while(mapped_size < size)
    {
        /* map superpages, if possible */
        if(((size - mapped_size) >= L4_SUPERPAGESIZE) &&
                !(source & (L4_SUPERPAGESIZE - 1)) &&
                !(dest & (L4_SUPERPAGESIZE - 1)))
            pageshift = L4_SUPERPAGESHIFT;
        else
            pageshift = L4_PAGESHIFT;

        msg = l4_task_map(vmm->vmtask, L4RE_THIS_TASK_CAP,
                          l4_fpage(source, pageshift, rights),
                          l4_map_control(dest, 0, L4_MAP_ITEM_MAP));
        if(l4_error(msg))
        {
            karma_log(ERROR, "Error mapping main memory. src=%08lx dest=%08lx\n",
                source, dest);
            exit(1);
        }

        source +=   (1UL << pageshift);
        dest +=     (1UL << pageshift);
        mapped_size += (1UL << pageshift);
    }

    /* leave one page gap */
    return dest+=(1UL << pageshift);
}

