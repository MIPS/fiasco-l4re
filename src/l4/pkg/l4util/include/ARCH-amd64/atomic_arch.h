/*****************************************************************************/
/**
 * \file
 * \brief   i386 atomic operations
 * \ingroup atomic
 *
 * \date    10/20/2000
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>,
 *          Jork Loeser  <jork@os.inf.tu-dresden.de> */
/*
 * (c) 2000-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */

/*****************************************************************************/
#ifndef __L4UTIL__INCLUDE__ARCH_AMD64__ATOMIC_ARCH_H__
#define __L4UTIL__INCLUDE__ARCH_AMD64__ATOMIC_ARCH_H__

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

#define LOCK "lock;"

EXTERN_C_BEGIN

L4_INLINE l4_uint64_t
l4util_xchg64(volatile l4_uint64_t * dest, l4_uint64_t val);

L4_INLINE l4_uint64_t
l4util_cmpxchg64_res(volatile l4_uint64_t *dest,
                     l4_uint64_t cmp_val, l4_uint64_t new_val);
L4_INLINE void
l4util_add64(volatile l4_uint64_t *dest, l4_uint64_t val);
L4_INLINE void
l4util_sub64(volatile l4_uint64_t *dest, l4_uint64_t val);
L4_INLINE void
l4util_and64(volatile l4_uint64_t *dest, l4_uint64_t val);
L4_INLINE void
l4util_or64(volatile l4_uint64_t *dest, l4_uint64_t val);
L4_INLINE l4_uint64_t
l4util_add64_res(volatile l4_uint64_t *dest, l4_uint64_t val);
L4_INLINE l4_uint64_t
l4util_sub64_res(volatile l4_uint64_t *dest, l4_uint64_t val);
L4_INLINE l4_uint64_t
l4util_and64_res(volatile l4_uint64_t *dest, l4_uint64_t val);
L4_INLINE l4_uint64_t
l4util_or64_res(volatile l4_uint64_t *dest, l4_uint64_t val);
L4_INLINE void
l4util_inc64(volatile l4_uint64_t *dest);
L4_INLINE void
l4util_dec64(volatile l4_uint64_t *dest);
L4_INLINE l4_uint64_t
l4util_inc64_res(volatile l4_uint64_t *dest);
L4_INLINE l4_uint64_t
l4util_dec64_res(volatile l4_uint64_t *dest);

EXTERN_C_END

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

/* atomic compare and exchange 64 bit value */
#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG64
L4_INLINE int
l4util_cmpxchg64(volatile l4_uint64_t * dest,
		 l4_uint64_t cmp_val, l4_uint64_t new_val)
{
  l4_uint64_t tmp;

  __asm__ __volatile__
    (
     LOCK "cmpxchgq	%1, %3 \n\t"
     :
     "=a" (tmp)      /* 0 RAX, return val */
     :
     "r"  (new_val), /* 1 reg, new value */
     "0"  (cmp_val), /* 2 RAX, compare value */
     "m"  (*dest)    /* 3 mem, destination operand */
     : 
     "memory", "cc"
     );

  return tmp == cmp_val;
}

/* atomic compare and exchange 32 bit value */
#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG32
L4_INLINE int
l4util_cmpxchg32(volatile l4_uint32_t * dest,
		 l4_uint32_t cmp_val, l4_uint32_t new_val)
{
  l4_uint32_t tmp;

  __asm__ __volatile__
    (
     LOCK "cmpxchgl %1, %3 \n\t"
     :
     "=a" (tmp)      /* 0 EAX, return val */
     :
     "r"  (new_val), /* 1 reg, new value */
     "0"  (cmp_val), /* 2 EAX, compare value */
     "m"  (*dest)    /* 3 mem, destination operand */
     :
     "memory", "cc"
     );

  return tmp == cmp_val;
}

/* atomic compare and exchange 16 bit value */
#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG16
L4_INLINE int
l4util_cmpxchg16(volatile l4_uint16_t * dest,
		 l4_uint16_t cmp_val, l4_uint16_t new_val)
{
  l4_uint16_t tmp;

  __asm__ __volatile__
    (
     LOCK "cmpxchgw %1, %3 \n\t"
     :
     "=a" (tmp)      /* 0  AX, return value */
     :
     "c"  (new_val), /* 1  CX, new value */
     "0"  (cmp_val), /* 2  AX, compare value */
     "m"  (*dest)    /* 3 mem, destination operand */
     :
     "memory", "cc"
     );

  return tmp == cmp_val;
}

/* atomic compare and exchange 8 bit value */
#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG8
L4_INLINE int
l4util_cmpxchg8(volatile l4_uint8_t * dest,
		l4_uint8_t cmp_val, l4_uint8_t new_val)
{
  l4_uint8_t tmp;

  __asm__ __volatile__
    (
     LOCK "cmpxchgb %1, %3 \n\t"
     :
     "=a" (tmp)      /* 0  AL, return value */
     :
     "c"  (new_val), /* 1  CL, new value */
     "0"  (cmp_val), /* 2  AL, compare value */
     "m"  (*dest)    /* 3 mem, destination operand */
     :
     "memory", "cc"
     );

  return tmp == cmp_val;
}

#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG
L4_INLINE int
l4util_cmpxchg(volatile l4_umword_t * dest,
               l4_umword_t cmp_val, l4_umword_t new_val)
{
  return l4util_cmpxchg64((volatile l4_uint64_t *)dest,
                          (l4_uint64_t)cmp_val, (l4_uint64_t)new_val);
}

/* atomic exchange 64 bit value */
#define __L4UTIL_ATOMIC_HAVE_ARCH_XCHG64
L4_INLINE l4_uint64_t
l4util_xchg64(volatile l4_uint64_t * dest, l4_uint64_t val)
{
  __asm__ __volatile__
    (
     LOCK "xchg %0, %1 \n\t"
     :
     "=r" (val)
     :
     "m" (*dest), "0" (val)
     :
     "memory"
    );

  return val;
}

/* atomic exchange 32 bit value */
#define __L4UTIL_ATOMIC_HAVE_ARCH_XCHG32
L4_INLINE l4_uint32_t
l4util_xchg32(volatile l4_uint32_t * dest, l4_uint32_t val)
{
  __asm__ __volatile__
    (
     LOCK "xchg %0, %1 \n\t"
     :
     "=r" (val)
     :
     "m" (*dest), "0" (val)
     :
     "memory"
    );

  return val;
}

/* atomic exchange 16 bit value */
#define __L4UTIL_ATOMIC_HAVE_ARCH_XCHG16
L4_INLINE l4_uint16_t
l4util_xchg16(volatile l4_uint16_t * dest, l4_uint16_t val)
{
  __asm__ __volatile__
    (
     LOCK "xchg %w0, %1 \n\t"
     :
     "=r" (val)
     :
     "m" (*dest), "0" (val)
     :
     "memory"
    );

  return val;
}

/* atomic exchange 8 bit value */
#define __L4UTIL_ATOMIC_HAVE_ARCH_XCHG8
L4_INLINE l4_uint8_t
l4util_xchg8(volatile l4_uint8_t * dest, l4_uint8_t val)
{
  __asm__ __volatile__
    (
     LOCK "xchg %b0, %1 \n\t"
     :
     "=r" (val)
     :
     "m" (*dest), "0" (val)
     :
     "memory"
    );

  return val;
}

/* atomic exchange machine width */
#define __L4UTIL_ATOMIC_HAVE_ARCH_XCHG
L4_INLINE l4_umword_t
l4util_xchg(volatile l4_umword_t * dest, l4_umword_t val)
{
  return l4util_xchg64((volatile l4_uint64_t *)dest, (l4_uint64_t)val);
}

#define l4util_gen_allop(args...)	\
l4util_genop( 8,"b", "", args)	\
l4util_genop(16,"w", "", args)	\
l4util_genop(32,"l", "", args)  \
l4util_genop(64,"q", "", args)

#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG8_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG16_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG32_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG64_RES
/* Generate l4_uint{8,16,32,64} l4util_cmpxchg{8,16,32,64}_res(...) */
#undef l4util_genop
#define l4util_genop(bit, mod, op1, opname, opchar)		\
L4_INLINE l4_uint##bit##_t					\
l4util_##opname##bit##_res(volatile l4_uint##bit##_t* dest,	\
			   l4_uint##bit##_t cmp_val,		\
			   l4_uint##bit##_t new_val)		\
{								\
  l4_uint##bit##_t old_val;					\
  __asm__ __volatile__						\
    (								\
     LOCK #opname mod " %" op1 "1,%3	\n\t"			\
     :								\
     "=a"(old_val)						\
     :								\
     "r"(new_val), "a"(cmp_val), "m" (*dest)			\
     :								\
     "memory"							\
    );								\
  return old_val;						\
}
l4util_gen_allop(cmpxchg,cmpxchg)

#define __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG_RES
L4_INLINE l4_umword_t
l4util_cmpxchg_res(volatile l4_umword_t *dest,
                   l4_umword_t cmp_val, l4_umword_t new_val)
{
  return l4util_cmpxchg64_res((volatile l4_uint64_t *)dest,
                              (l4_uint64_t)cmp_val, (l4_uint64_t)new_val);
}

#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD8
#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD16
#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD32
#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD64
#define __L4UTIL_ATOMIC_HAVE_ARCH_SUB8
#define __L4UTIL_ATOMIC_HAVE_ARCH_SUB16
#define __L4UTIL_ATOMIC_HAVE_ARCH_SUB32
#define __L4UTIL_ATOMIC_HAVE_ARCH_SUB64
#define __L4UTIL_ATOMIC_HAVE_ARCH_AND8
#define __L4UTIL_ATOMIC_HAVE_ARCH_AND16
#define __L4UTIL_ATOMIC_HAVE_ARCH_AND32
#define __L4UTIL_ATOMIC_HAVE_ARCH_AND64
#define __L4UTIL_ATOMIC_HAVE_ARCH_OR8
#define __L4UTIL_ATOMIC_HAVE_ARCH_OR16
#define __L4UTIL_ATOMIC_HAVE_ARCH_OR32
#define __L4UTIL_ATOMIC_HAVE_ARCH_OR64
/* Generate void l4util_{add,sub,and,or}{8,16,32,64}(*dest) */
#undef l4util_genop
#define l4util_genop(bit, mod, op1, opname)			\
L4_INLINE void							\
l4util_##opname##bit(volatile l4_uint##bit##_t* dest, l4_uint##bit##_t val) \
{								\
  __asm__ __volatile__						\
    (								\
     LOCK #opname mod " %1,%0	\n\t"				\
     :								\
     :								\
     "m" (*dest), "ir" (val)					\
     :								\
     "memory"							\
    );								\
}
l4util_gen_allop(add)
l4util_gen_allop(sub)
l4util_gen_allop(and)
l4util_gen_allop(or)

#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD8_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD16_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD32_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD64_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_SUB8_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_SUB16_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_SUB32_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_SUB64_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_AND8_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_AND16_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_AND32_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_AND64_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_OR8_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_OR16_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_OR32_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_OR64_RES
/* Generate l4_uint{8,16,32,64} l4util_{add,sub,and,or}{8,16,32,64}_res(*dest) */
#undef l4util_genop
#define l4util_genop(bit, mod, op1, opname, opchar)		\
L4_INLINE l4_uint##bit##_t					\
l4util_##opname##bit##_res(volatile l4_uint##bit##_t* dest,	\
			   l4_uint##bit##_t val)		\
{								\
  l4_uint##bit##_t res, old;					\
								\
  do								\
    {								\
      old = *dest;						\
      res = old opchar val;					\
    }								\
  while (!l4util_cmpxchg##bit(dest, old, res));			\
								\
  return res;							\
}
l4util_gen_allop(add, +)
l4util_gen_allop(sub, -)
l4util_gen_allop(and, &)
l4util_gen_allop(or, &&)

#define __L4UTIL_ATOMIC_HAVE_ARCH_INC8
#define __L4UTIL_ATOMIC_HAVE_ARCH_INC16
#define __L4UTIL_ATOMIC_HAVE_ARCH_INC32
#define __L4UTIL_ATOMIC_HAVE_ARCH_INC64
#define __L4UTIL_ATOMIC_HAVE_ARCH_DEC8
#define __L4UTIL_ATOMIC_HAVE_ARCH_DEC16
#define __L4UTIL_ATOMIC_HAVE_ARCH_DEC32
#define __L4UTIL_ATOMIC_HAVE_ARCH_DEC64
/* Generate void l4util_{inc,dec}{8,16,32,64}(*dest) */
#undef l4util_genop
#define l4util_genop(bit, mod, op1, opname)			\
L4_INLINE void							\
l4util_##opname##bit(volatile l4_uint##bit##_t* dest)		\
{								\
  __asm__ __volatile__						\
    (								\
     LOCK #opname mod " %0	\n\t"					\
     :								\
     :								\
     "m" (*dest)						\
     :								\
     "memory"							\
    );								\
}
l4util_gen_allop(inc)
l4util_gen_allop(dec)

#define __L4UTIL_ATOMIC_HAVE_ARCH_INC8_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_INC16_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_INC32_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_INC64_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_DEC8_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_DEC16_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_DEC32_RES
#define __L4UTIL_ATOMIC_HAVE_ARCH_DEC64_RES
/* Generate l4_uint{8,16,32,64} l4util_{inc,dec}{8,16,32,64}_res(*dest) */
#undef l4util_genop
#define l4util_genop(bit, mod, op1, opname, opchar)		\
L4_INLINE l4_uint##bit##_t					\
l4util_##opname##bit##_res(volatile l4_uint##bit##_t* dest)	\
{								\
  l4_uint##bit##_t res, old;					\
								\
  do								\
    {								\
      res = *dest;						\
      old = res opchar;						\
    }								\
  while (!l4util_cmpxchg##bit(dest, old, res));			\
								\
  return res;							\
}
l4util_gen_allop(inc, ++)
l4util_gen_allop(dec, --)

#undef l4util_genop
#undef l4util_gen_allop


#define __L4UTIL_ATOMIC_HAVE_ARCH_ADD
L4_INLINE void
l4util_atomic_add(volatile long *dest, long val)
{
  __asm__ __volatile__(LOCK "addq %1, %0	\n"
                       : "=m" (*dest)
                       : "ri" (val), "m" (*dest));
}

#define __L4UTIL_ATOMIC_HAVE_ARCH_INC
L4_INLINE void
l4util_atomic_inc(volatile long *dest)
{
  __asm__ __volatile__(LOCK "incq %0"
                       : "=m" (*dest)
                       : "m" (*dest));
}

#endif /* ! __L4UTIL__INCLUDE__ARCH_AMD64__ATOMIC_ARCH_H__ */
