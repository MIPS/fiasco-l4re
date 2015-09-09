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

#ifndef __TEE_TRUSTLET_IFACE_H
#define __TEE_TRUSTLET_IFACE_H

#include <l4/re/dataspace>
#include <l4/re/env>

#include "interface.h"

class Trustlet
{
  
  private:

    l4_addr_t _addr;
    L4::Cap<Sktapp_interface> svr; 
    L4::Cap<L4Re::Dataspace> _ds; 
    static const l4_addr_t shared_addresses[3];
 
  public:
    Trustlet(char const *trustlet_name, int mail_box);
    int receive_operation_iface(l4_umword_t cmd, sk_operation* op) throw();
   
    /* IAsim has no cache but on real hardware writes will need to
 *      * to do a l4_cache_clean_data()
 *           */   
     l4_addr_t get_shared_mem() { return _addr; }    
    ~Trustlet();
};

#endif
