/*
 * Cirrus framebuffer driver
 *
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <stdio.h>
#include <l4/drivers/fb.h>
#include <l4/io/io.h>

#include <linux/fb.h>

#include "vbus_pci.h"
#include "l4_cirrusfb.h"

static const char *fb_get_name(void)
{ return "Cirrus FB driver"; }

static unsigned int fb_mem_size(void)
{
  struct fb_info *info = l4_cirrusfb_get_fbinfo();

  if (!info) {
    printf("[FB] ERROR: fb_mem_size called before fb_info was initialized\n");
    return 0;
  }

  return info->screen_size;
}

static int get_fbinfo(l4re_video_view_info_t *vinfo)
{
  struct fb_info *info = l4_cirrusfb_get_fbinfo();
  int bytes_per_pixel;

  if (!info) {
    printf("[FB] ERROR: get_fbinfo called before fb_info was initialized\n");
    return 1;
  }

  bytes_per_pixel = (info->var.bits_per_pixel + 7) / 8;

  vinfo->width               = info->var.xres;
  vinfo->height              = info->var.yres;
  vinfo->bytes_per_line      = bytes_per_pixel * info->var.xres;

  vinfo->pixel_info.bytes_per_pixel = bytes_per_pixel;
  vinfo->pixel_info.r.shift         = info->var.red.offset;
  vinfo->pixel_info.r.size          = info->var.red.length;
  vinfo->pixel_info.g.shift         = info->var.green.offset;
  vinfo->pixel_info.g.size          = info->var.green.length;
  vinfo->pixel_info.b.shift         = info->var.blue.offset;
  vinfo->pixel_info.b.size          = info->var.blue.length;
  vinfo->pixel_info.a.shift         = info->var.transp.offset;
  vinfo->pixel_info.a.size          = info->var.transp.length;
  return 0;
}

static void fb_enable(void)
{
  struct fb_info *info = l4_cirrusfb_get_fbinfo();

  if (!info) {
    printf("[FB] ERROR: fb_enable called before fb_info was initialized\n");
    return;
  }

  fb_set_cmap(&info->cmap, info);
  info->state = FBINFO_STATE_RUNNING;
}

static void fb_disable(void)
{
  printf("%s unimplemented.\n", __func__);
}

// return virtual address of framebuffer video memory
static void *fb(void)
{
  struct fb_info *info = l4_cirrusfb_get_fbinfo();

  if (!info || !info->screen_base) {
    printf("[FB] ERROR: NULL video buffer resource!\n");
    return 0;
  }

  return (void*)info->screen_base;
}

// i.e. mode:640x480-16@60,noaccel
static char *cirrusfb_configstr;

static void alloc_configstr(const char *configstr)
{

  if (!configstr || !*configstr)
    return;

  cirrusfb_configstr = strdup(configstr);
}

static void free_configstr(void)
{
  if (cirrusfb_configstr)
    free(cirrusfb_configstr);
}

static int cirrusfb_probe(const char *configstr)
{
  struct fb_info *info;

  // leave original configstr unchanged for other drivers' use
  alloc_configstr(configstr);

  l4_cirrusfb_config(cirrusfb_configstr);

  if (l4_cirrusfb_init() != 0) {
    printf("[FB] ERROR: l4_cirrusfb_init failed\n");
    goto err_release_configstr;
  }

#ifdef CIRRUSFB_DEBUG
  vbus_dump_pci_vbus();
#endif

  // scan vbus for vga device and call cirrusfb_pci_probe on match
  if (vbus_pci_probe_vga() != L4_EOK)
    goto err_release_configstr;

  info = l4_cirrusfb_get_fbinfo();
  if (!info) {
    printf("[FB] ERROR: fb_info was not initialized\n");
    goto err_release_configstr;
  }

  // found driver
  return 1;

err_release_configstr:
  free_configstr();
  return 0;
}

static struct fb_driver_ops fb_driver_ops_cirrus = {
  .probe              = cirrusfb_probe,
  .get_fb             = fb,
  .get_fbinfo         = get_fbinfo,
  .get_video_mem_size = fb_mem_size,
  .get_info           = fb_get_name,
  .enable             = fb_enable,
  .disable            = fb_disable,
};

fb_register(&fb_driver_ops_cirrus);
