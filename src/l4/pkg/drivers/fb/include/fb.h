/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef __DRIVERS__FB__INCLUDE__FB_H__
#define __DRIVERS__FB__INCLUDE__FB_H__

#include <l4/sys/types.h>
#include <l4/re/c/video/view.h>

EXTERN_C_BEGIN

struct fb_driver_ops {
  int          (*probe)(const char *configstr);
  void *       (*get_fb)(void);
  unsigned int (*get_video_mem_size)(void);
  const char * (*get_info)(void);

  int          (*get_fbinfo)(l4re_video_view_info_t *vinfo);

  void         (*enable)(void);
  void         (*disable)(void);
};

struct fb_driver_ops *fb_probe(const char *configstr);

void fb_register_driver(struct fb_driver_ops *);

/* Callable once per file (should be enough?) */
#define fb_register(ops)                                        \
    static void __attribute__((constructor)) __register_ops(void)    \
    { fb_register_driver(ops); }

EXTERN_C_END

#endif /* ! __DRIVERS__FB__INCLUDE__FB_H__ */

