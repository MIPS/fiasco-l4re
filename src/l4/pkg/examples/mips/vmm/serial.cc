/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include "l4_ser.h"
#include "devices/device_init.hpp"
#include "devices/device_call.hpp"
#include "serial.h"

inline void toHypercallPayload(vz_vmm_t *vmm, HypercallPayload & hc_payload) {
    hc_payload.rawCommandReg() = vmm_regs(vmm).r[16]; /* s0 */
    hc_payload.reg(0) = vmm_regs(vmm).r[17]; /* s1 */
    hc_payload.reg(1) = vmm_regs(vmm).r[18]; /* s2 */
    hc_payload.reg(2) = vmm_regs(vmm).r[19]; /* s3 */
    hc_payload.reg(3) = vmm_regs(vmm).r[20]; /* s4 */
}

inline void fromHypercallPayload(vz_vmm_t *vmm, const HypercallPayload & hc_payload){
    vmm_regs(vmm).r[16] = hc_payload.rawCommandReg();
    vmm_regs(vmm).r[17] = hc_payload.reg(0);
    vmm_regs(vmm).r[18] = hc_payload.reg(1);
    vmm_regs(vmm).r[19] = hc_payload.reg(2);
    vmm_regs(vmm).r[20] = hc_payload.reg(3);
}

void serial_do_hypcall(vz_vmm_t *vmm, HypercallPayload &hp)
{
  toHypercallPayload(vmm, hp);
  DO_HYPERCALL(hp);
  fromHypercallPayload(vmm, hp);
}

int serial_init(vz_vmm_t *vmm)
{
  GLOBAL_DEVICES_INIT;
  GET_HYPERCALL_DEVICE(ser).init(12,
      vmm->host_addr_mapbase,
      vmm->vm_guest_mem_size,
      vmm->vm_guest_mapbase);
  return 0;
}
