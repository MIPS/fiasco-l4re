/*
 * Some generic functions for framebuffer drivers.
 *
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <l4/drivers/fb.h>

enum { nr_drivers = 4 };
static struct fb_driver_ops *ops[nr_drivers];
static int ops_alloced;

struct fb_driver_ops *fb_probe(const char *configstr)
{
  int i;

  for (i = 0; i < ops_alloced; i++)
    if (ops[i]->probe && ops[i]->probe(configstr))
      return ops[i];

  return NULL;
}

void fb_register_driver(struct fb_driver_ops *_ops)
{
  if (ops_alloced < nr_drivers)
    ops[ops_alloced++] = _ops;
}
