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

#ifndef __TRUSTLET_SRV1_H
#define __TRUSTLET_SRV1_H

#include <l4/cxx/ipc_server>

#include "interface.h"

class Trustlet_server_obj : public L4::Server_object
{
private:

  L4::Cap<L4Re::Dataspace> _shm;
  int SKTAPP_init_iface();

public:

  explicit Trustlet_server_obj(L4::Cap<L4Re::Dataspace> shm)
  : _shm(shm)
  {}  

  int SKTAPP_receive_operation_iface(l4_umword_t cmd, sk_operation *op);
  int SKTAPP_receive_operation_iface2(l4_umword_t cmd, sk_operation *op);
  int SKTAPP_receive_operation_iface3(l4_umword_t cmd, sk_operation *op);
  int dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios);
};

#endif
