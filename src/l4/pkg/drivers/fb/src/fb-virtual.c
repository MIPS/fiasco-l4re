/*
 * Dummy framebuffer driver
 *
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <stdlib.h>

#include <l4/drivers/fb.h>

static const char *fb_get_info(void)
{ return "FB virtual driver"; }

static void void_dummy(void) {}

static int probe(const char *configstr)
{
  (void)configstr;
// KYMA consider changing registration scheme to fixed-order init
#undef FBDRV_VIRTUALDRV_ENABLE
#ifdef FBDRV_VIRTUALDRV_ENABLE
  return 1;
#else
  return 0;
#endif
}

static inline int width(void) { return 640; }
static inline int height(void) { return 480; }
static inline int bytes_per_pixel(void) { return 4; }

static unsigned int fb_mem_size(void)
{ return height() * width() * bytes_per_pixel(); }

static void *fb(void) { return malloc(fb_mem_size()); }

static int get_fbinfo(l4re_video_view_info_t *vinfo)
{
  vinfo->width               = width();
  vinfo->height              = height();
  vinfo->bytes_per_line      = bytes_per_pixel() * vinfo->width;

  vinfo->pixel_info.bytes_per_pixel = bytes_per_pixel();
  vinfo->pixel_info.r.shift         = 0;
  vinfo->pixel_info.r.size          = 8;
  vinfo->pixel_info.g.shift         = 8;
  vinfo->pixel_info.g.size          = 8;
  vinfo->pixel_info.b.shift         = 16;
  vinfo->pixel_info.b.size          = 8;
  vinfo->pixel_info.a.shift         = 0;
  vinfo->pixel_info.a.size          = 0;

  return 0;
}

static struct fb_driver_ops fb_driver_ops_virtual = {
  .probe              = probe,
  .get_fb             = fb,
  .get_fbinfo         = get_fbinfo,
  .get_video_mem_size = fb_mem_size,
  .get_info           = fb_get_info,
  .enable             = void_dummy,
  .disable            = void_dummy,
};

fb_register(&fb_driver_ops_virtual);
