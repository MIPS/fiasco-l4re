/*
 * Graphics mode initialization, mips specific
 *
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include <l4/drivers/fb.h>

#include "gmode.h"
#include "vc.h"

void
init_gmode(void)
{
  struct fb_driver_ops *fb;

  if (!(fb = fb_probe(NULL)))
    {
      printf("[l4con] fb hardware driver not available, using env 'fb' cap.\n");
      return;
    }

  printf("[l4con] Using fb driver: %s\n", fb->get_info());

  if (fb->get_fbinfo(&fb_info))
    {
      printf("[l4con] Could not get framebuffer info\n");
      return;
    }

  YRES_CLIENT           = fb_info.height - status_area;
  fb_info.buffer_offset = 0;

  gr_vmem         = fb->get_fb();
  gr_vmem_size    = fb->get_video_mem_size();
  gr_vmem_maxmap  = gr_vmem + gr_vmem_size;
  vis_vmem        = gr_vmem;
  if (!gr_vmem)
    {
      printf("[l4con] Could not setup video memory\n");
      exit(1);
    }

  fb->enable();
}
