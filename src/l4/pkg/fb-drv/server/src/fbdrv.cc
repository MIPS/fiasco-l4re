/*
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <l4/drivers/fb.h>
#include <l4/sys/cache.h>
#include <cstdio>

#include "fb.h"
#include "splash.h"

bool
Fb_drv::setup_drv(Prog_args *pa)
{
  struct fb_driver_ops *fb;

  if (!(fb = fb_probe(pa->config_str)))
    {
      printf("[fb-drv] Could not find fb.\n");
      return false;
    }

  printf("[fb-drv] Using fb driver: %s\n", fb->get_info());

  _vidmem_start = (l4_addr_t)fb->get_fb();

  if (!_vidmem_start)
    {
      printf("[fb-drv] Driver init failed\n");
      return false;
    }

  _vidmem_size  = fb->get_video_mem_size();
  _vidmem_end   = _vidmem_start + _vidmem_size;

  if (fb->get_fbinfo((l4re_video_view_info_t *)&_view_info))
    {
      printf("[fb-drv] Failed to get driver framebuffer info\n");
      return false;
    }

  _screen_info.width = _view_info.width;
  _screen_info.height = _view_info.height;
  _screen_info.flags = L4Re::Video::Goos::F_auto_refresh;
  _screen_info.pixel_info = _view_info.pixel_info;
  _view_info.buffer_offset = 0;

  init_infos();

  fb->enable();

  splash_display(&_view_info, _vidmem_start);

  return true;
}
