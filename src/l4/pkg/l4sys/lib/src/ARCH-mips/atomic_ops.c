/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 *
 * Derived in part from linux 3.17 arch/mips/include/asm/cmpxchg.h
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2003, 06, 07 by Ralf Baechle (ralf@linux-mips.org)
 */

#include <l4/sys/kdebug.h>

extern long int l4_atomic_add(volatile long int* mem, long int offset);
extern long int l4_atomic_cmpxchg(volatile long int* mem, long int oldval, long int newval);
extern long int l4_atomic_xchg(volatile long int* mem, long int newval);

long int
l4_atomic_add(volatile long int* mem, long int offset)
{
  long int result, temp;

  do {
      asm volatile (
      " ll  %1, %2  # atomic_add_return \n"
      " addu  %0, %1, %3      \n"
      " sc  %0, %2        \n"
      : "=&r" (result), "=&r" (temp), "+m" (*mem)
      : "Ir" (offset));
  } while ((!result));

  result = temp + offset;

  return result;
}


// borrowed from linux; removed .set mips3, .set mips0 directives
#define __cmpxchg_asm(ld, st, m, old, new)				\
({									\
	__typeof(*(m)) __ret;						\
									\
		__asm__ __volatile__(					\
		"	.set	push				\n"	\
		"	.set	noat				\n"	\
		"1:	" ld "	%0, %2		# __cmpxchg_asm \n"	\
		"	bne	%0, %z3, 2f			\n"	\
		"	move	$1, %z4				\n"	\
		"	" st "	$1, %1				\n"	\
		"	beqz	$1, 1b				\n"	\
		"	.set	pop				\n"	\
		"2:						\n"	\
		: "=&r" (__ret), "=R" (*m)				\
		: "R" (*m), "Jr" (old), "Jr" (new)			\
		: "memory");						\
	__ret;								\
})

#define __cmpxchg(ptr, old, new, pre_barrier, post_barrier)   \
({                  \
  __typeof__(ptr) __ptr = (ptr);          \
  __typeof__(*(ptr)) __old = (old);       \
  __typeof__(*(ptr)) __new = (new);       \
  __typeof__(*(ptr)) __res = 0;         \
                  \
  pre_barrier;              \
                  \
  switch (sizeof(*(__ptr))) {         \
  case 4:               \
    __res = __cmpxchg_asm("ll", "sc", __ptr, __old, __new); \
    break;              \
  default:              \
    break;              \
  }               \
                  \
  post_barrier;             \
                  \
  __res;                \
})

/**
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */
#define cmpxchg(ptr, old, new)  __cmpxchg(ptr, old, new, , )


/**
 * Atomic compare and exchange.
 * return 0 if comparison failed, 1 otherwise
 */
long int
l4_atomic_cmpxchg(volatile long int* mem, long int oldval, long int newval)
{
  long int ret = cmpxchg(mem, oldval, newval);
  return (ret == oldval);
}

long int
l4_atomic_xchg(volatile long int* mem, long int newval)
{
  long int retval;
  unsigned long dummy;

  do {
      __asm__ __volatile__(
      " ll  %0, %3    # xchg_u32  \n"
      " move  %2, %z4       \n"
      " sc  %2, %1        \n"
      : "=&r" (retval), "=m" (*mem), "=&r" (dummy)
      : "R" (*mem), "Jr" (newval)
      : "memory");
  } while ((!dummy));

  return retval;
}
