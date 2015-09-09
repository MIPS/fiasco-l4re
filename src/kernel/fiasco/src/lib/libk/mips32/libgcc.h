/*
 * Derived from linux arch/mips/lib/libgcc.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef __ASM_LIBGCC_H
#define __ASM_LIBGCC_H

#define LINUX_LIBGCC_FOR_FIASCO

#ifdef LINUX_LIBGCC_FOR_FIASCO
#ifdef CONFIG_BIG_ENDIAN
#define __BIG_ENDIAN
#else
#define __LITTLE_ENDIAN
#endif
#define EXPORT_SYMBOL(x)
#else
//#include <asm/byteorder.h>
#endif /* LINUX_LIBGCC_FOR_FIASCO */

typedef int word_type __attribute__ ((mode (__word__)));

#ifdef __BIG_ENDIAN
struct DWstruct {
	int high, low;
};
#elif defined(__LITTLE_ENDIAN)
struct DWstruct {
	int low, high;
};
#else
#error I feel sick.
#endif

typedef union {
	struct DWstruct s;
	long long ll;
} DWunion;

#endif /* __ASM_LIBGCC_H */
