#ifndef _ASM_GENERIC_BUG_H
#define _ASM_GENERIC_BUG_H

#include <linux/compiler.h>
#include <linux/kernel.h>

#define __WARN()
#define __WARN_printf(arg...)	do { printk(arg); __WARN(); } while (0)

#ifndef WARN
#define WARN(condition, format...) ({						\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on))					\
		__WARN_printf(format);					\
	unlikely(__ret_warn_on);					\
})
#endif

#endif /* _ASM_GENERIC_BUG_H */
