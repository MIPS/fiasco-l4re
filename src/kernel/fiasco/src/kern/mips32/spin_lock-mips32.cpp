/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32 && mp]:

EXTENSION class Spin_lock
{
public:
  enum { Arch_lock = 2 };
};

//--------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mp]:

PRIVATE template<typename Lock_t> inline
void
Spin_lock<Lock_t>::lock_arch()
{
  Lock_t dummy, tmp;

  __asm__ __volatile__(
      "   .set    push            \n"
      "   .set    mips32          \n"
      "   .set    noreorder       \n"
      "1:                         \n"
      "   ll      %[d], %[lock]   \n"
      "   andi    %[tmp], %[d], 2 \n" /* Arch_lock == #2 */
      "   bnez    %[tmp], 2f      \n" /* branch if lock already taken */
      "   ori     %[d], %[d], 2   \n" /* Arch_lock == #2 */
      "   sc      %[d], %0        \n" /* acquire lock atomically */
      "   bnez    %[d], 3f        \n" /* branch if lock acquired successfully */
      "   sync                    \n"
      "2:                         \n"
      "   b       1b              \n" /* and retry the operation */
      "   nop                     \n"
      "3:                         \n"
      "   .set    reorder         \n"
      "   .set    pop             \n"
      : "=m" (_lock), [tmp] "=&r" (tmp), [d] "=&r" (dummy)
      : [lock] "m" (_lock)
      : "memory");
}

PRIVATE template<typename Lock_t> inline NEEDS["mem.h"]
void
Spin_lock<Lock_t>::unlock_arch()
{
  Mem::mp_mb();
  _lock &= ~Arch_lock;
}
