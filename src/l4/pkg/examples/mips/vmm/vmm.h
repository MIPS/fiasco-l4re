/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#ifndef _VMM_H
#define _VMM_H

#include <l4/sys/types.h>
#include <l4/sys/vm.h>
#include <l4/sys/vcpu.h>

enum vz_exit_type_e {
  wait_exits,
  cache_exits,
  signal_exits,
  int_exits,
  cop_unusable_exits,
  tlbmod_exits,
  tlbmiss_ld_exits,
  tlbmiss_st_exits,
  addrerr_st_exits,
  addrerr_ld_exits,
  syscall_exits,
  resvd_inst_exits,
  break_inst_exits,
  flush_dcache_exits,
  hypervisor_gpsi_exits,
  hypervisor_gpsi_cp0_exits,
  hypervisor_gpsi_cache_exits,
  hypervisor_gsfc_exits,
  hypervisor_gsfc_cp0_status_exits,
  hypervisor_gsfc_cp0_cause_exits,
  hypervisor_gsfc_cp0_intctl_exits,
  hypervisor_hc_exits,
  hypervisor_grr_exits,
  hypervisor_gva_exits,
  hypervisor_ghfc_exits,
  hypervisor_gpa_exits,
  hypervisor_resv_exits,
  MAX_MIPS_EXIT_TYPES
};

struct vz_exit_stats {
  l4_umword_t stats[MAX_MIPS_EXIT_TYPES];
};

/*
 * Coprocessor 0 register names
 */
#define	MIPS_CP0_TLB_INDEX	    0
#define	MIPS_CP0_TLB_RANDOM	    1
#define	MIPS_CP0_TLB_LOW	    2
#define	MIPS_CP0_TLB_LO0	    2
#define	MIPS_CP0_TLB_LO1	    3
#define	MIPS_CP0_TLB_CONTEXT	    4
#define	MIPS_CP0_TLB_PG_MASK	    5
#define	MIPS_CP0_TLB_WIRED	    6
#define	MIPS_CP0_HWRENA 	    7
#define	MIPS_CP0_BAD_VADDR	    8
#define	MIPS_CP0_COUNT	            9
#define	MIPS_CP0_TLB_HI	            10
#define	MIPS_CP0_COMPARE	    11
#define	MIPS_CP0_STATUS	            12
#define	MIPS_CP0_CAUSE	            13
#define	MIPS_CP0_EXC_PC	            14
#define	MIPS_CP0_PRID		    15
#define	MIPS_CP0_CONFIG	            16
#define	MIPS_CP0_LLADDR	            17
#define	MIPS_CP0_WATCH_LO	    18
#define	MIPS_CP0_WATCH_HI	    19
#define	MIPS_CP0_TLB_XCONTEXT       20
#define	MIPS_CP0_ECC		    26
#define	MIPS_CP0_CACHE_ERR	    27
#define	MIPS_CP0_TAG_LO	            28
#define	MIPS_CP0_TAG_HI	            29
#define	MIPS_CP0_ERROR_PC	    30
#define	MIPS_CP0_DEBUG	            23
#define	MIPS_CP0_DEPC		    24
#define	MIPS_CP0_PERFCNT	    25
#define	MIPS_CP0_ERRCTL             26
#define	MIPS_CP0_DATA_LO	    28
#define	MIPS_CP0_DATA_HI	    29
#define	MIPS_CP0_DESAVE	            31

#define MIPS_CP0_GTOFFSET           12
#define MIPS_CP0_GTOFFSET_SEL       7

#define MIPS_CP0_GUESTCTL0          12
#define MIPS_CP0_GUESTCTL0_SEL      6

#define MIPS_CP0_GUESTCTL0EXT       11
#define MIPS_CP0_GUESTCTL0EXT_SEL   4

#define MIPS_CP0_GUESTCTL1          10
#define MIPS_CP0_GUESTCTL1_SEL      4

#define MIPS_CP0_GUESTCTL2          10
#define MIPS_CP0_GUESTCTL2_SEL      5

#define MIPS_CP0_PRID_SEL           0

#define MIPS_CP0_USERLOCAL          4
#define MIPS_CP0_USERLOCAL_SEL      2

#define MIPS_CP0_INTCTL             12
#define MIPS_CP0_INTCTL_SEL         1

typedef struct vz_vmm_t {
  l4_cap_idx_t     vmtask;
  l4_vcpu_state_t  *vcpu;
  l4_vm_vz_state_t *vmstate;

  l4_umword_t vm_guest_mem_size;
  l4_addr_t vm_guest_entrypoint;  // address is in vm guest address space
  l4_addr_t vm_guest_mapbase;     // address is in vm guest address space
  l4_addr_t host_addr_mapbase;    // address is in host address space

} vz_vmm_t;

#define RESUME_GUEST            0
#define RESUME_HOST             1
#define RESUME_EXIT             2

/*
 * vz_trap_handler
 *
 * returns:
 *  RESUME_GUEST
 *  RESUME_HOST 
 *  RESUME_EXIT 
 */
int vz_trap_handler(vz_vmm_t *vmm);
int vz_init(vz_vmm_t *vmm);

inline l4_vcpu_regs_t& vmm_regs(vz_vmm_t *vmm) {
    return vmm->vcpu->r;
}

#endif /* _VMM_H */
