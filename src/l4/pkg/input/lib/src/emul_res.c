/**
 * \file   input/lib/src/emul_res.c
 * \brief  L4INPUT: Linux I/O resource management emulation
 *
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Currently we only support I/O port requests.
 */
/*
 * (c) 2007-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <l4/io/io.h>

#if defined(ARCH_mips)
void * request_region(unsigned long start, unsigned long len, const char *name)
{
	l4_addr_t vaddr;
	int err = l4io_request_iomem(start, len, 0, &vaddr);
	return err ? 0 : (void *)vaddr;
}

void release_region(unsigned long start_virt, unsigned long len)
{
	l4io_release_iomem(start_virt, len);
}

#else
void * request_region(unsigned long start, unsigned long len, const char *name)
{
	int err = l4io_request_ioport(start, len);
	return err ? 0 : (void *)1;
}

void release_region(unsigned long start, unsigned long len)
{
	l4io_release_ioport(start, len);
}

#endif
