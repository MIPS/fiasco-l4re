/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * stubs and definitions for using linux headers
 */

#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <linux/stddef.h>
#include <linux/types.h>
#include <string.h>
#include <asm/byteorder.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifdef DEBUG
#define dev_dbg(dev, format, arg...) printf("Dbg: " format, ##arg)
#else
#define dev_dbg(dev, format, arg...)
#endif

#define dev_info(dev, format, arg...) printf("Info: " format, ##arg)
#define dev_warn(dev, format, arg...) printf("Warn: " format, ##arg)
#define dev_err(dev, format, arg...) printf("Error: " format, ##arg)

#define printk printf

#define simple_strtol strtol

#define KERN_EMERG    "" /* system is unusable */
#define KERN_ALERT    "" /* action must be taken immediately */
#define KERN_CRIT     "" /* critical conditions */
#define KERN_ERR      "" /* error conditions */
#define KERN_WARNING  "" /* warning conditions */
#define KERN_NOTICE   "" /* normal but significant condition */
#define KERN_INFO     "" /* informational */
#define KERN_DEBUG    "" /* debug-level messages */

extern int printf(const char *format, ...);
extern int sprintf(char * buf, const char * fmt, ...);

static inline int get_option(char **str, int *pint)
{
	/* no int in sring */
	return 0;
}
static inline char *get_options(const char *str, int nints, int *ints)
{
	/* no ints found - start pointer breaks search */
	*ints = 0;
	return (char *)str;
}

#define min(x, y) ({                            \
        typeof(x) _min1 = (x);                  \
        typeof(y) _min2 = (y);                  \
        (void) (&_min1 == &_min2);              \
        _min1 < _min2 ? _min1 : _min2; })

#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define cpu_relax() l4_barrier()

#endif

