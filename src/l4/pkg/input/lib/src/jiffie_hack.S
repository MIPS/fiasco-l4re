
.global jiffies
#ifdef ARCH_amd64
jiffies = 0x6ffff000 + 0xa0
#elif defined(ARCH_mips)
#define __ASSEMBLY__
#include <l4/sys/mem_layout.h>
/* set jiffies to address of clock field in KIP */
jiffies = L4RE_KIP_ADDR + 0xa0
#else
jiffies = 0xaffff000 + 0xa0
#endif
