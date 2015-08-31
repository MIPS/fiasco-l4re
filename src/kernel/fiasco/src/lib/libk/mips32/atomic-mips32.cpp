/*
 * Derived in part from linux 3.17 arch/mips/include/asm/cmpxchg.h
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
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

IMPLEMENTATION [mips32]:

#include "panic.h"

// ``unsafe'' stands for no safety according to the size of the given type.
// There are type safe versions of the cas operations in the architecture
// independent part of atomic that use the unsafe versions and make a type
// check.

inline
bool
cas_unsafe( Mword *ptr, Mword oldval, Mword newval )
{
  Mword ret;

  __asm__ __volatile__(
  "     .set    push                            \n"
  "     .set    noat                            \n"
  "     .set    " MIPS_ISA_LEVEL"               \n"
  "1:   ll      %0, %2          # __cmpxchg_asm \n"
  "     bne     %0, %z3, 2f                     \n"
  "     .set    mips0                           \n"
  "     move    $1, %z4                         \n"
  "     .set    " MIPS_ISA_LEVEL"               \n"
  "     sc      $1, %1                          \n"
  "     beqz    $1, 1b                          \n"
  "     .set    pop                             \n"
  "2:                                           \n"
  : "=&r" (ret), "=" GCC_OFF_SMALL_ASM() (*ptr)
  : GCC_OFF_SMALL_ASM() (*ptr), "Jr" (oldval), "Jr" (newval)
  : "memory");

  // true is ok
  // false is failed
  return ret == oldval;
}

/* dummy implement */
inline NEEDS["panic.h"]
bool
cas2_unsafe( Mword *, Mword *, Mword *)
{
  panic("%s not implemented", __func__);
  return false;
}

inline
void
atomic_and (Mword *l, Mword mask)
{
  int old;

  do {
    __asm__ __volatile__(
    "     .set    " MIPS_ISA_LEVEL"               \n"
    "     ll      %0, %1                          \n"
    "     and     %0, %2                          \n"
    "     sc      %0, %1                          \n"
    "     .set    mips0                           \n"
    : "=&r" (old), "+" GCC_OFF_SMALL_ASM() (*l)
    : "Ir" (mask));
  } while (!old);
}

inline
void
atomic_or (Mword *l, Mword bits)
{
  int old;

  do {
    __asm__ __volatile__(
    "     .set    " MIPS_ISA_LEVEL"               \n"
    "     ll      %0, %1                          \n"
    "     or      %0, %2                          \n"
    "     sc      %0, %1                          \n"
    "     .set    mips0                           \n"
    : "=&r" (old), "+" GCC_OFF_SMALL_ASM() (*l)
    : "Ir" (bits));
  } while (!old);
}

/*
 * Derived in part from linux 3.17 arch/mips/include/asm/atomic.h
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 *
 * But use these as seldom as possible since they are much more slower
 * than regular operations.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 97, 99, 2000, 03, 04, 06 by Ralf Baechle
 */
inline
void
atomic_add (Mword *l, Mword value)
{
  int old;

  do {
    __asm__ __volatile__(
    "     .set    " MIPS_ISA_LEVEL"               \n"
    "     ll      %0, %1          # atomic_add    \n"
    "     addu    %0, %2                          \n"
    "     sc      %0, %1                          \n"
    "     .set    mips0                           \n"
    : "=&r" (old), "+" GCC_OFF_SMALL_ASM() (*l)
    : "Ir" (value));
  } while (!old);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mp]:

#include "mem.h"

inline NEEDS["mem.h"]
void
atomic_mp_and(Mword *l, Mword value)
{
  Mem::mp_mb();
  atomic_and(l, value);
  Mem::mp_mb();
}

inline NEEDS["mem.h"]
void
atomic_mp_or(Mword *l, Mword value)
{
  Mem::mp_mb();
  atomic_or(l, value);
  Mem::mp_mb();
}


inline NEEDS["mem.h"]
void
atomic_mp_add(Mword *l, Mword value)
{
  Mem::mp_mb();
  atomic_add(l, value);
  Mem::mp_mb();
}

inline NEEDS["mem.h"]
bool
mp_cas_arch(Mword *m, Mword o, Mword n)
{
  Mword ret;

  Mem::mp_mb();
  ret = cas_unsafe(m, o, n);
  Mem::mp_mb();

  return ret;
}

inline NEEDS["panic.h"]
bool mp_cas2_arch(char *, Mword, Mword, Mword, Mword)
{
  panic("%s not implemented", __func__);
  return false;
}
