/*
 * oms_accel.h
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// x86 accelerations
#define MM_ACCEL_X86_MMX	0x80000000
#define MM_ACCEL_X86_3DNOW	0x40000000
#define MM_ACCEL_X86_MMXEXT	0x20000000

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ARCH_x86) || defined(ARCH_amd64)
uint32_t mm_accel (void);
#else
inline uint32_t mm_accel (void);
inline uint32_t mm_accel (void)
{
  return 0;
}
#endif

#ifdef __cplusplus
}
#endif
