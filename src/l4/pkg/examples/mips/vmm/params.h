/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

/*
 * params.h - TODO enter description
 * 
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
#pragma once

#define MAX_FILES 4

struct __params
{
  unsigned int debug_level;
  unsigned int mem_size;
  unsigned int premap;
  const char *kernel_image;
  const char *kernel_opts;
  const char *images[MAX_FILES];
};

extern struct __params params;
