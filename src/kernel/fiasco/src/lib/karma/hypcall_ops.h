/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef ASM_HYPCALL_OPS_H_
#define ASM_HYPCALL_OPS_H_

/*
 * Definition of HYPCALL codes for interface between VZ VMM and VZ Guest
 */

/* max value 0x3ff */
#define HYPCALL_KARMA_DEV_OP    (0x000)
#define HYPCALL_MAGIC_EXIT      (0x111)

#ifdef __ASSEMBLER__

#define ASM_HYPCALL(code)       .word( 0x42000028 | (code<<11) )

#else /* __ASSEMBLER__ */

#ifndef __STR
#define __STR(x) #x
#endif
#ifndef STR
#define STR(x) __STR(x)
#endif

#define ASM_HYPCALL_STR(code)   STR(.word( 0x42000028 | (code<<11) ))

#endif /* __ASSEMBLER__ */

#endif /* ASM_HYPCALL_OPS_H_ */
