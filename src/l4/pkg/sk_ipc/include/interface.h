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

#include <l4/sys/capability>
#include <l4/re/dataspace>
#include <l4/cxx/ipc_stream>

#include "sk_types.h"

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

#endif

/**
 * Interface for remote trustlet object
 */
class Sktapp_interface : public L4::Kobject_t<Sktapp_interface, L4::Kobject>
{

    L4_KOBJECT(Sktapp_interface)

public:
    int sktapp_init_iface(L4::Cap<L4Re::Dataspace> ds) throw();
    int sktapp_close_iface() throw();
    int sktapp_receive_operation_iface(l4_umword_t cmd, sk_operation* op) throw();
};
 

inline
int Sktapp_interface::sktapp_init_iface(L4::Cap<L4Re::Dataspace> ds) throw()
{
    L4::Ipc::Iostream s(l4_utcb());

    s << l4_umword_t(Opcode::init_iface);
    s << L4::Ipc::Small_buf(ds);

    return l4_error(s.call(cap(), Protocol::Tee_trustlet));
}

inline 
int Sktapp_interface::sktapp_close_iface() throw()
{
    return 0;

}

inline 
int Sktapp_interface::sktapp_receive_operation_iface(l4_umword_t cmd, sk_operation* op) throw()
{
    L4::Ipc::Iostream s(l4_utcb());

    s << l4_umword_t(Opcode::receive_operation_iface); 
    s << l4_umword_t(cmd);
    s.put(*op);

    return l4_error(s.call(cap(), Protocol::Tee_trustlet));
}

