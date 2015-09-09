/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE:

#include "bitmap.h"
#include "linux_bitops.h"

#define BITMAP_LAST_WORD_MASK(nbits) \
( \
  ((nbits) % Bpl) ? (1UL<<((nbits) % Bpl))-1 : ~0UL \
)

/*
 * Based on fiasco lib/libk class Bitmap with additional operators to allow
 * pointer access to the bitmap, as used in linux.
 */
template<unsigned BITS>
class Bitmap_linux : public Bitmap_base< (BITS > sizeof(unsigned long) * 8), BITS >
{
public:
  using Bitmap_base< (BITS > sizeof(unsigned long) * 8),BITS>::Bpl;
  using Bitmap_base< (BITS > sizeof(unsigned long) * 8),BITS>::Nr_elems;

  Bitmap_linux() {}
  Bitmap_linux(Bitmap_linux const &o) : Bitmap_base< (BITS > sizeof(unsigned long) * 8), BITS >()
  { this->_copy(o); }

  Bitmap_linux &operator = (Bitmap_linux const &o)
  {
    this->_copy(o);
    return *this;
  }

  template<unsigned SBITS>
  Bitmap_linux &operator = (Bitmap_linux<SBITS> const &o)
  {
    this->_copy(o);
    return *this;
  }

  Bitmap_linux &operator |= (Bitmap_linux const &o)
  {
    this->_or(o);
    return *this;
  }
};

/* Linux pointer access operators */
EXTENSION class Bitmap_linux
{
public:
  /* define operator [] to access one long at a time rather than bit-wise */
  unsigned long *operator [] (unsigned long index) const
  {
    if (index < Nr_elems)
      return &this->_bits[index];
  }

  operator unsigned long* ()
  {
    return this->_bits;
  }
};

/* Linux bitmap methods */
EXTENSION class Bitmap_linux
{
private:
  static bool small_const_nbits(unsigned int nbits)
  {
    return !(nbits > sizeof(unsigned long) * 8);
  }

  static int __bitmap_and(unsigned long *dst, const unsigned long *bitmap1,
                   const unsigned long *bitmap2, unsigned long bits)
  {
    unsigned int k;
    unsigned int lim = bits/Bpl;
    unsigned long result = 0;

    for (k = 0; k < lim; k++)
      result |= (dst[k] = bitmap1[k] & bitmap2[k]);
    if (bits % Bpl)
      result |= (dst[k] = bitmap1[k] & bitmap2[k] &
                 BITMAP_LAST_WORD_MASK(bits));
    return result != 0;
  }

public:
  static inline void bitmap_fill(unsigned long *dst, int nbits)
  {
          size_t nlongs = BITS_TO_LONGS(nbits);
          if (!small_const_nbits(nbits)) {
                  int len = (nlongs - 1) * sizeof(unsigned long);
                  __builtin_memset(dst, 0xff,  len);
          }
          dst[nlongs - 1] = BITMAP_LAST_WORD_MASK(nbits);
  }

  static int bitmap_and(unsigned long *dst, const unsigned long *src1,
                 const unsigned long *src2, unsigned int nbits)
  {
    if (small_const_nbits(nbits))
      return (*dst = *src1 & *src2 & BITMAP_LAST_WORD_MASK(nbits)) != 0;
    return __bitmap_and(dst, src1, src2, nbits);
  }
};
