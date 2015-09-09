/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32]:

IMPLEMENT static inline
void
Mem::memset_mwords (void *dst, const unsigned long value, unsigned long nr_of_mwords)
{
  unsigned long *d = (unsigned long *)dst;
  for (; nr_of_mwords--; d++)
    *d = value;
}

IMPLEMENT static inline
void
Mem::memcpy_mwords (void *dst, void const *src, unsigned long nr_of_mwords)
{
  __builtin_memcpy(dst, src, nr_of_mwords * sizeof(unsigned long));
}

IMPLEMENT static inline
void
Mem::memcpy_bytes(void *dst, void const *src, unsigned long nr_of_bytes)
{
  __builtin_memcpy(dst, src, nr_of_bytes);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [mips32 && mp]:

IMPLEMENT inline static void Mem::mb() { __asm__ __volatile__("sync" : : : "memory"); }
IMPLEMENT inline static void Mem::rmb() { __asm__ __volatile__("sync" : : : "memory"); }
IMPLEMENT inline static void Mem::wmb() { __asm__ __volatile__("sync" : : : "memory"); }

IMPLEMENT inline static void Mem::mp_mb() { __asm__ __volatile__("sync" : : : "memory"); }
IMPLEMENT inline static void Mem::mp_rmb() { __asm__ __volatile__("sync" : : : "memory"); }
IMPLEMENT inline static void Mem::mp_wmb() { __asm__ __volatile__("sync" : : : "memory"); }
