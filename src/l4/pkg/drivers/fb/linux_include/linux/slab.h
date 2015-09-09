#ifndef _LINUX_SLAB_H
#define _LINUX_SLAB_H

/* Hmm: stdlib.h defines a structure with __init as member... */
#undef __init

#include <stdlib.h>
#include <stddef.h>
#include <linux/init.h>

#define __init

/* from linux/gpf.h */
/* Plain integer GFP bitmasks. Do not use this directly. */
#define ___GFP_DMA		0x01u
#define ___GFP_HIGHMEM		0x02u
#define ___GFP_DMA32		0x04u
#define ___GFP_MOVABLE		0x08u
#define ___GFP_WAIT		0x10u
#define ___GFP_HIGH		0x20u
#define ___GFP_IO		0x40u
#define ___GFP_FS		0x80u
#define ___GFP_COLD		0x100u
#define ___GFP_NOWARN		0x200u
#define ___GFP_REPEAT		0x400u
#define ___GFP_NOFAIL		0x800u
#define ___GFP_NORETRY		0x1000u
#define ___GFP_MEMALLOC		0x2000u
#define ___GFP_COMP		0x4000u
#define ___GFP_ZERO		0x8000u
#define ___GFP_NOMEMALLOC	0x10000u
#define ___GFP_HARDWALL		0x20000u
#define ___GFP_THISNODE		0x40000u
#define ___GFP_RECLAIMABLE	0x80000u
#define ___GFP_KMEMCG		0x100000u
#define ___GFP_NOTRACK		0x200000u
#define ___GFP_NO_KSWAPD	0x400000u
#define ___GFP_OTHER_NODE	0x800000u
#define ___GFP_WRITE		0x1000000u

#define __GFP_WAIT	((__force gfp_t)___GFP_WAIT)	/* Can wait and reschedule? */
#define __GFP_HIGH	((__force gfp_t)___GFP_HIGH)	/* Should access emergency pools? */
#define __GFP_IO	((__force gfp_t)___GFP_IO)	/* Can start physical IO? */
#define __GFP_FS	((__force gfp_t)___GFP_FS)	/* Can call down to low-level FS? */
#define __GFP_ZERO	((__force gfp_t)___GFP_ZERO)	/* Return zeroed page on success */

#define GFP_ATOMIC	(__GFP_HIGH)
#define GFP_KERNEL	(__GFP_WAIT | __GFP_IO | __GFP_FS)

static inline void *kmalloc(size_t size, int flags)
{
	if (flags & __GFP_ZERO)
		return calloc(1, size);
	else
		return malloc(size);
}

static inline void kfree(void *p)
{
	free(p);
}

/**
 * kzalloc - allocate memory. The memory is set to zero.
 * @size: how many bytes of memory are required.
 * @flags: the type of memory to allocate (see kmalloc).
 */
static inline void *kzalloc(size_t size, gfp_t flags)
{
	return kmalloc(size, flags | __GFP_ZERO);
}
#endif

