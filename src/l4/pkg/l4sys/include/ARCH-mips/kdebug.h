/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * (c) 2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
#ifndef __L4SYS__INCLUDE__ARCH_MIPS__KDEBUG_H__
#define __L4SYS__INCLUDE__ARCH_MIPS__KDEBUG_H__

#include <l4/sys/compiler.h>
#include <l4/sys/consts.h>

#define L4_SYSCALL_ENTER_KDEBUG      (1<<2)

#define L4_SYSCALL_ENTER_KDEBUG_TEXT 1
#define L4_SYSCALL_ENTER_KDEBUG_CALL 2


#ifdef __GNUC__

/*
 * gcc optimization was occasionally NOT using registers t0,s5,s6 therefore for
 * this macro we specify these kernel ABI registers directly in the inline
 * assembly.
 */
#define enter_kdebug(text...)       \
  ({                                \
    __asm__ __volatile__ (          \
      "  .set push              \n" \
      "  .set noreorder         \n" \
      "  li $s5, %[sc_num]      \n" \
      "  li $s6, %[op]          \n" \
      "  la $t0, 2f             \n" \
      "  syscall                \n" \
      "  b       1f             \n" \
      "  nop                    \n" \
      "2:                       \n" \
      "  .asciiz  \"" text "\"  \n" \
      "  .align 2               \n" \
      "1:                       \n" \
      "  .set pop               \n" \
      :                             \
      :                             \
      [sc_num] "i" (L4_SYSCALL_ENTER_KDEBUG), \
      [op] "i" (L4_SYSCALL_ENTER_KDEBUG_TEXT) \
      : "t0", "s5", "s6"            \
    );                              \
  })

L4_INLINE void
outnstring(const char* x, unsigned len);

L4_INLINE void
outstring(const char *text);

L4_INLINE void
outchar(char c);

L4_INLINE void
outdec(int number);

L4_INLINE void
outhex32(int number);

L4_INLINE void
outhex20(int number);

L4_INLINE void
outhex16(int number);

L4_INLINE void
outhex12(int number);

L4_INLINE void
outhex8(int number);

L4_INLINE void
kd_display(char *text);

L4_INLINE int
l4kd_inchar(void);

L4_INLINE void __touch_ro(const char *x, unsigned len);

L4_INLINE l4_umword_t
__kdebug_param(l4_umword_t nr, l4_umword_t p1, l4_umword_t p2);

L4_INLINE l4_umword_t
__kdebug_param_5(l4_umword_t nr, l4_umword_t p1, l4_umword_t p2,
                 l4_umword_t p3, l4_umword_t p4, l4_umword_t p5);

L4_INLINE
l4_umword_t
__kdebug_param(l4_umword_t nr, l4_umword_t p1, l4_umword_t p2)
{
  register l4_umword_t _nr __asm__("t0") = nr;
  register l4_umword_t _p1 __asm__("t1") = p1;
  register l4_umword_t _p2 __asm__("t2") = p2;
  register l4_umword_t _sc_num __asm__("s5") = L4_SYSCALL_ENTER_KDEBUG;
  register l4_umword_t _op __asm__("s6") = L4_SYSCALL_ENTER_KDEBUG_CALL;

  __asm__ __volatile__ (
    "  syscall \n"
    :
    "+r" (_nr),
    "+r" (_p1),
    "+r" (_p2),
    "+r" (_sc_num),
    "+r" (_op)
    :
    :
    "memory");
  return _p1;
}

L4_INLINE
l4_umword_t
__kdebug_param_5(l4_umword_t nr, l4_umword_t p1, l4_umword_t p2,
                 l4_umword_t p3, l4_umword_t p4, l4_umword_t p5)
{
  register l4_umword_t _nr __asm__("t0") = nr;
  register l4_umword_t _p1 __asm__("t1") = p1;
  register l4_umword_t _p2 __asm__("t2") = p2;
  register l4_umword_t _p3 __asm__("t3") = p3;
  register l4_umword_t _p4 __asm__("t4") = p4;
  register l4_umword_t _p5 __asm__("t5") = p5;
  register l4_umword_t _sc_num __asm__("s5") = L4_SYSCALL_ENTER_KDEBUG;
  register l4_umword_t _op __asm__("s6") = L4_SYSCALL_ENTER_KDEBUG_CALL;

  __asm__ __volatile__ (
    "  syscall \n"
    :
    "+r" (_nr),
    "+r" (_p1),
    "+r" (_p2),
    "+r" (_p3),
    "+r" (_p4),
    "+r" (_p5),
    "+r" (_sc_num),
    "+r" (_op)
    :
    :
    "memory");
  return _p1;
}

L4_INLINE void
__touch_ro(const char *x, unsigned len)
{
   volatile const char *sptr, *eptr;
   sptr = (const char*)((unsigned)x & L4_PAGEMASK);
   eptr = (const char*)(((unsigned)x + len -1) & L4_PAGEMASK);

   for(;sptr <= eptr; sptr += L4_PAGESIZE)
     (void)(*sptr);
}

L4_INLINE void
outnstring(const char* x, unsigned len)
{
  __touch_ro(x, len);
  __kdebug_param(3, (l4_umword_t)x, (l4_umword_t)len);
}

L4_INLINE void
outstring(const char *text)
{
  unsigned i = 0;
  while(text[i++]) ;
  outnstring(text, i);
}

L4_INLINE void
outchar(char c)
{
  __kdebug_param(1, (l4_umword_t)c, 0);
}

L4_INLINE void
outdec(int number)
{
  __kdebug_param(4, (l4_umword_t)number, 0);
}

L4_INLINE void
outhex32(int number)
{
  __kdebug_param(5, (l4_umword_t)number, 0);
}

L4_INLINE void
outhex20(int number)
{
  __kdebug_param(6, (l4_umword_t)number, 0);
}

L4_INLINE void
outhex16(int number)
{
  __kdebug_param(7, (l4_umword_t)number, 0);
}

L4_INLINE void
outhex12(int number)
{
  __kdebug_param(8, (l4_umword_t)number, 0);
}

L4_INLINE void
outhex8(int number)
{
  __kdebug_param(9, (l4_umword_t)number, 0);
}

L4_INLINE void
kd_display(char *text)
{
  outstring(text);
}

L4_INLINE int
l4kd_inchar(void)
{
  return __kdebug_param(0xd, 0, 0);
}

#endif //__GNUC__

#endif /* ! __L4SYS__INCLUDE__ARCH_MIPS__KDEBUG_H__ */
