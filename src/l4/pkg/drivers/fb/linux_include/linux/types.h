#ifndef _LINUX_TYPES
#define _LINUX_TYPES

#include <asm/types.h>

/* from uapi/linux/types.h */
#define __bitwise__
#define __bitwise

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

/* from linux/types.h */
typedef _Bool			bool;

typedef unsigned __bitwise__ gfp_t;
typedef unsigned __bitwise__ fmode_t;
typedef unsigned __bitwise__ oom_flags_t;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
typedef u64 phys_addr_t;
#else
typedef u32 phys_addr_t;
#endif

typedef phys_addr_t resource_size_t;

typedef struct {
	int counter;
} atomic_t;

#ifdef USE_OSKIT
typedef signed long ssize_t;
#endif

#endif
