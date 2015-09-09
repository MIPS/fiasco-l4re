/**
 *
 * Copyright  (c) 2015 Elliptic Technologies
 *
 * \author  Jason Butler jbutler@elliptictech.com
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 */

#pragma once

#include <sk_both.h>
#include <sk_swld.h>

#define ADDRESS3 0x00014000  //0x01084000
#define ADDRESS2 0x01088000
#define ADDRESS1 0x0108C000
enum
{
  DS_SIZE = 4 << 12,
};


#if 0

#define SK_ADDRESS_TYPE l4_uint32_t
#define SK_TAPI_MEMORY_ID_LENGTH 16
#define SK_MAXIMUM_OPERATIONS 4

typedef struct _sk_op_meminfo_
{
   l4_uint32_t        memory_size;
   l4_uint32_t        flag;
   SK_ADDRESS_TYPE addr;
   l4_uint8_t         id[SK_TAPI_MEMORY_ID_LENGTH];
}sk_op_meminfo;

typedef struct _sk_op_param_
{
  sk_op_meminfo mem;
}sk_op_param;

typedef struct _sk_operation_
{
   l4_uint32_t    num_param;
   sk_op_param param[SK_MAXIMUM_OPERATIONS];

  // template< typename STREAM >
  // STREAM &dump(STREAM &s) const
}sk_operation;

typedef sk_operation sk_operation;
#endif



namespace Opcode {
enum Opcodes {
  init_iface, close_iface, receive_operation_iface
};
};

namespace Protocol {
enum Protocols {
 Tee_trustlet 
};
};
