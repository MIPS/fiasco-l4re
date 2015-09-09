/*****************************************************************************/
/**
 * \file
 * \brief  mips memory-mapped port I/O
 *
 */
/*****************************************************************************/

/*
 * This file is distributed under the terms of the GNU Lesser General Public
 * License 2.1.  Please see the COPYING-LGPL-2.1 file for details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef _L4UTIL_PORT_IO_H
#define _L4UTIL_PORT_IO_H

/**
 * \defgroup l4util_portio MIPS memory mapped Port I/O API
 * \ingroup l4util_api
 */

/* L4 includes */
#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>
#include <l4/sys/types.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

EXTERN_C_BEGIN

#define IO_SYNC()     asm volatile ("sync" : : : "memory")

/**
 * \addtogroup l4util_portio
 */
/*@{*/
/**
 * \brief Read byte from I/O port
 *
 * \param  port	   I/O port address
 * \return value
 */
L4_INLINE l4_uint8_t
l4util_in8(l4_addr_t port);

/**
 * \brief Read 16-bit-value from I/O port
 *
 * \param  port	   I/O port address
 * \return value
 */
L4_INLINE l4_uint16_t
l4util_in16(l4_addr_t port);

/**
 * \brief Read 32-bit-value from I/O port
 *
 * \param  port	   I/O port address
 * \return value
 */
L4_INLINE l4_uint32_t
l4util_in32(l4_addr_t port);

/**
 * \brief Write byte to I/O port
 *
 * \param  port	   I/O port address
 * \param  value   value to write
 */
L4_INLINE void
l4util_out8(l4_uint8_t value, l4_addr_t port);

/**
 * \brief Write 16-bit-value to I/O port
 * \ingroup port_io
 *
 * \param  port	   I/O port address
 * \param  value   value to write
 */
L4_INLINE void
l4util_out16(l4_uint16_t value, l4_addr_t port);

/**
 * \brief Write 32-bit-value to I/O port
 *
 * \param  port	   I/O port address
 * \param  value   value to write
 */
L4_INLINE void
l4util_out32(l4_uint32_t value, l4_addr_t port);

/*@}*/

EXTERN_C_END


/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE l4_uint8_t
l4util_in8(l4_addr_t port)
{
  l4_uint8_t value = *(volatile l4_uint8_t *)port;
  IO_SYNC();;
  return value;
}

L4_INLINE l4_uint16_t
l4util_in16(l4_addr_t port)
{
  l4_uint16_t value = *(volatile l4_uint16_t *)port;
  IO_SYNC();;
  return value;
}

L4_INLINE l4_uint32_t
l4util_in32(l4_addr_t port)
{
  l4_uint32_t value = *(volatile l4_uint32_t *)port;
  IO_SYNC();;
  return value;
}

L4_INLINE void
l4util_out8(l4_uint8_t value, l4_addr_t port)
{
  *(volatile l4_uint8_t *)port = value;
  IO_SYNC();
}

L4_INLINE void
l4util_out16(l4_uint16_t value, l4_addr_t port)
{
  *(volatile l4_uint16_t *)port = value;
  IO_SYNC();
}

L4_INLINE void
l4util_out32(l4_uint32_t value, l4_addr_t port)
{
  *(volatile l4_uint32_t *)port = value;
  IO_SYNC();
}

#endif
