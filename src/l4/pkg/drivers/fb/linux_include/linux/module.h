#ifndef _LINUX_MODULE_H
#define _LINUX_MODULE_H

#include <linux/moduleparam.h>

struct module;

#define MODULE_LICENSE(x)
#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_DEVICE_TABLE(x,y)

/* XXX Consider to export this or provide setup funcs */
#define MODULE_PARM(x, y)
#define MODULE_PARM_DESC(x,y)

#define EXPORT_SYMBOL(sym)
#define EXPORT_SYMBOL_GPL(sym)

#define THIS_MODULE ((struct module *)0)

#define try_module_get(x)
#define module_put(x)

#endif

