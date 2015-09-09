/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef _HYPCALL_OPS_H
#define _HYPCALL_OPS_H

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

#endif /* _HYPCALL_OPS_H */
