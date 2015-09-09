#ifndef _LINUX_IOPORT_H
#define _LINUX_IOPORT_H

/*
 * Resources are tree-like, allowing
 * nesting etc..
 */
struct resource {
	const char *name;
	unsigned long start, end;
	unsigned long flags;
	struct resource *parent, *sibling, *child;
};

extern void * request_region(unsigned long start, unsigned long len, const char *name);
extern void release_region(unsigned long start, unsigned long len);

/*
 * IO resources have these defined flags.
 */
#define IORESOURCE_BITS		0x000000ff	/* Bus-specific bits */

#define IORESOURCE_TYPE_BITS	0x00001f00	/* Resource type */
#define IORESOURCE_IO		0x00000100	/* PCI/ISA I/O ports */
#define IORESOURCE_MEM		0x00000200
#define IORESOURCE_REG		0x00000300	/* Register offsets */
#define IORESOURCE_IRQ		0x00000400
#define IORESOURCE_DMA		0x00000800
#define IORESOURCE_BUS		0x00001000

#endif	/* _LINUX_IOPORT_H */
