/*
 * hypercall_payload.hpp - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * hypercall_payload.hpp
 *
 *  Created on: 14.02.2011
 *      Author: janis
 */

#ifndef HYPERCALL_PAYLOAD_HPP_
#define HYPERCALL_PAYLOAD_HPP_

#include <stdio.h>

class HypercallPayload{
private:
    enum{
        dev_id_mask = 0x00ff0000,
        dev_id_shift = 16,
        addr_mask = 0x0000ffff,
        addr_shift = 0,
        reg_count_mask = 0x07000000,
        reg_count_shift = 24,
    };
    unsigned long _raw_command;
    unsigned long _regs[4];
public:
    inline unsigned long deviceID() const{
        return (_raw_command & dev_id_mask) >> dev_id_shift;
    }
    inline unsigned short address() const{
        return (_raw_command & addr_mask) >> addr_shift;
    }
    inline unsigned char regCount() const{
        return (_raw_command & reg_count_mask) >> reg_count_shift;
    }
    inline unsigned long & rawCommandReg(){
        return _raw_command;
    }
    inline const unsigned long & rawCommandReg() const{
        return _raw_command;
    }
    inline unsigned long & reg(const unsigned char index){
        return _regs[index];
    }
    inline const unsigned long & reg(const unsigned char index) const{
        return _regs[index];
    }
    void dump() const{
        printf("Hypercall payload dev: %lx addr: %hx regs: %hhx reg0: %lx reg1: %lx reg2: %lx reg3: %lx\n"
                ,deviceID(), address(), regCount(), reg(0), reg(1), reg(2), reg(3));
    }
};


#endif /* HYPERCALL_PAYLOAD_HPP_ */
