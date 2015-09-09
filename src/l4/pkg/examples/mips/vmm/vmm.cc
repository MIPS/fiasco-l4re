/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <l4/sys/types.h>
#include <l4/sys/cp0_op.h>
#include <l4/re/c/rm.h>
#include <stdio.h>

#include "inst.h"
#include "mipsregs.h"
#include "mipsvzregs.h"

#include "vmm.h"
#include "hypcall_ops.h"
#include "mem_man.h"
#include "serial.h"
#include "debug.h"

/*
 * Trace logging and statistic
 */
static struct vz_exit_stats vz_exit_stats;
static int exit_flag = 0;

/*
 * Emulation
 */
#define VZ_INVALID_INST         0xdeadbeef

enum emulation_result {
        EMULATE_DONE,           /* no further processing */
        EMULATE_FAIL,           /* can't emulate this instruction */
        EMULATE_WAIT,           /* WAIT instruction */
};

#define COP0MT  0xffe007f8
#define MTC0    0x40800000
#define COP0MF  0xffe007f8
#define MFC0    0x40000000
#define RT      0x001f0000
#define RD      0x0000f800
#define SEL     0x00000007

#define VZ_TRACE_EXIT(vmm, e) vz_trace_exit((vmm), (e), #e)

/*
 * Prototypes
 */
void vz_trace_exit(vz_vmm_t *vmm, enum vz_exit_type_e e, const char* e_str);
l4_umword_t get_badinstr(vz_vmm_t *vmm);
enum emulation_result vz_emulate_CP0(l4_umword_t inst, l4_umword_t *opc, l4_umword_t cause, vz_vmm_t *vmm);
enum emulation_result vz_trap_handle_gpsi(l4_umword_t cause, l4_umword_t *opc, vz_vmm_t *vmm);
enum emulation_result vz_trap_handle_gsfc(l4_umword_t cause, l4_umword_t *opc, vz_vmm_t *vmm);
enum emulation_result vz_trap_handle_hypcall(l4_umword_t cause, l4_umword_t *opc, vz_vmm_t *vmm);
enum emulation_result vz_trap_default_handler(l4_umword_t cause, l4_umword_t *opc, vz_vmm_t *vmm);
int vz_trap_handle_ge(vz_vmm_t *vmm);
void vz_update_pc(vz_vmm_t *vmm);
int vz_tlb_handler(vz_vmm_t *vmm);
int vz_init(vz_vmm_t *vmm);

void vz_trace_exit(vz_vmm_t *vmm, enum vz_exit_type_e e, const char* e_str)
{
  karma_log(TRACE, "[%#08lx] VZ EXIT %s count:%ld\n",
          vmm_regs(vmm).ip, e_str, ++vz_exit_stats.stats[e]);
}

/*
 *  Fetch the bad instruction register
 *  This code assumes that badinstr and badinstrp are supported.
 */
l4_umword_t get_badinstr(vz_vmm_t *vmm)
{
  static int init_done = 0;
  static unsigned set_bad_instr[Exc_code_LAST];
  l4_umword_t cause = vmm_regs(vmm).cause;
  l4_umword_t exccode = ((cause & CAUSEF_EXCCODE) >> CAUSEB_EXCCODE);

  if (!init_done) {
    set_bad_instr[Exc_code_Int]       = 0,      // Interrupt
    set_bad_instr[Exc_code_Mod]       = 1,      // TLB modification exception
    set_bad_instr[Exc_code_TLBL]      = 1,      // TLB exception (load or fetch)
    set_bad_instr[Exc_code_TLBS]      = 1,      // TLB exception (store)
    set_bad_instr[Exc_code_AdEL]      = 1,      // Address error exception (load or fetch)
    set_bad_instr[Exc_code_AdES]      = 1,      // Address error exception (store)
    set_bad_instr[Exc_code_IBE]       = 0,      // Bus error exception (load or fetch)
    set_bad_instr[Exc_code_DBE]       = 0,      // Bus error exception (store)
    set_bad_instr[Exc_code_Sys]       = 1,      // Syscall exception
    set_bad_instr[Exc_code_Bp]        = 1,      // Breakpoint exception
    set_bad_instr[Exc_code_RI]        = 1,      // Reserved instruction exception
    set_bad_instr[Exc_code_CpU]       = 1,      // Coprocessor unusable exception
    set_bad_instr[Exc_code_Ov]        = 1,      // Arithmetic overflow exception
    set_bad_instr[Exc_code_Tr]        = 1,      // Trap
    set_bad_instr[Exc_code_Res1]      = 0,      // (reserved)
    set_bad_instr[Exc_code_FPE]       = 1,      // Floating point exception
    set_bad_instr[Exc_code_Impl1]     = 0,      // (implementation-dependent 1)
    set_bad_instr[Exc_code_Impl2]     = 0,      // (implementation-dependent 2)
    set_bad_instr[Exc_code_C2E]       = 1,      // (reserved for precise coprocessor 2)
    set_bad_instr[Exc_code_TLBRI]     = 1,      // TLB exception (read)
    set_bad_instr[Exc_code_TLBXI]     = 1,      // TLB exception (execute)
    set_bad_instr[Exc_code_Res2]      = 0,      // (reserved)
    set_bad_instr[Exc_code_MDMX]      = 0,      // (MIPS64 MDMX unusable)
    set_bad_instr[Exc_code_WATCH]     = 0,      // Reference to watchHi/watchLo address
    set_bad_instr[Exc_code_MCheck]    = 0,      // Machine check exception
    set_bad_instr[Exc_code_Thread]    = 0,      // Thread exception
    set_bad_instr[Exc_code_DSPDis]    = 0,      // DSP disabled exception
    set_bad_instr[Exc_code_GE]        = 1,      // Guest Exit exception
    set_bad_instr[Exc_code_Res4]      = 0,      // (reserved)
    set_bad_instr[Exc_code_Prot]      = 0,      // Protection exception
    set_bad_instr[Exc_code_CacheErr]  = 0,      // Cache error exception
    set_bad_instr[Exc_code_Res6]      = 0,      // (reserved)
    init_done = 1;
  };

  unsigned bad_instr_valid = set_bad_instr[exccode];

  if (exccode == Exc_code_GE) {
    switch (vmm->vmstate->exit_gexccode) {
      case GUESTCTL0_GEXC_GPSI:
      case GUESTCTL0_GEXC_GSFC:
      case GUESTCTL0_GEXC_HC:
      case GUESTCTL0_GEXC_GRR:
        bad_instr_valid = 1;
        break;
      case GUESTCTL0_GEXC_GVA:
      case GUESTCTL0_GEXC_GHFC:
      case GUESTCTL0_GEXC_GPA:
      default:
        bad_instr_valid = 0;
        break;
    }
  }

  if (bad_instr_valid)
    return vmm->vmstate->exit_badinstr;
  else
    return VZ_INVALID_INST;
}

/* modelled on kvm_mips_emulate_CP0 */
enum emulation_result
vz_emulate_CP0(l4_umword_t inst, l4_umword_t *opc, l4_umword_t cause,
		     vz_vmm_t *vmm)
{
  (void)opc; (void)cause;
  enum emulation_result er = EMULATE_DONE;
  int rt, rd, copz, sel, co_bit, op;
  l4_umword_t pc = vmm_regs(vmm).ip;

  copz = (inst >> 21) & 0x1f;
  rt = (inst >> 16) & 0x1f;
  rd = (inst >> 11) & 0x1f;
  sel = inst & 0x7;
  co_bit = (inst >> 25) & 1;

  if (co_bit) {
    op = (inst) & 0xff;

    switch (op) {
#define WAIT_OP 0x20 /*  100000  */
      case WAIT_OP:
        VZ_TRACE_EXIT(vmm, wait_exits);
        break;
      default:
        er = EMULATE_FAIL;
        break;
    }
  } else {
    switch (copz) {
      case mfc_op:
        /* Get reg */
        if ((rd == MIPS_CP0_COUNT) && (sel == 0)) {
          vmm_regs(vmm).r[rt] = vmm->vmstate->gcp0[MIPS_CP0_COUNT][0];
        }
        else if ((rd == MIPS_CP0_COMPARE) && (sel == 0)) {
          vmm_regs(vmm).r[rt] = vmm->vmstate->gcp0[MIPS_CP0_COMPARE][0];
        }
        else if ((rd == MIPS_CP0_PRID) && (sel == MIPS_CP0_PRID_SEL)) {
          vmm_regs(vmm).r[rt] = vmm->vmstate->gcp0[MIPS_CP0_PRID][MIPS_CP0_PRID_SEL];
        } else {
          // other mfc0/mtc0 emulation
        }
        karma_log(DEBUG, "[%#08lx] MFCz[%d][%d], vmm_regs(vmm).r[%d]: %#08lx\n",
            pc, rd, sel, rt, vmm_regs(vmm).r[rt]);

        break;

      case mtc_op:
        if ((rd == MIPS_CP0_COMPARE) && (sel == 0)) {
          karma_log(DEBUG, "[%#08lx] MTCz, cp0 reg[%d][%d] : COMPARE %#08lx <- %#08lx\n",
              pc, rd, sel, vmm->vmstate->gcp0[MIPS_CP0_COMPARE][0],
              vmm_regs(vmm).r[rt]);

          vmm->vmstate->gcp0[MIPS_CP0_COMPARE][0] = vmm_regs(vmm).r[rt];
        } else {
          // other mfc0/mtc0 emulation
          karma_log(DEBUG, "[%#08lx] MTCz, cp0 reg[%d][%d]: %#08lx\n",
              pc, rd, sel, vmm_regs(vmm).r[rt]);
        }
        break;

      default:
        karma_log(WARN, "[%#08lx] vz_emulate_CP0: unsupported COP0, copz: 0x%x\n",
            pc, copz);
        er = EMULATE_FAIL;
        break;
    }
  }
  return er;
}

enum emulation_result
vz_trap_handle_gpsi(l4_umword_t cause, l4_umword_t *opc, vz_vmm_t *vmm)
{
  enum emulation_result er = EMULATE_DONE;
  l4_umword_t inst = get_badinstr(vmm);
  union mips_instruction mips_inst;
  mips_inst.word = inst;

  if (vmm_regs(vmm).cause & CAUSEF_BD)
    opc += 1;

  switch (mips_inst.r_format.opcode) {
    case cop0_op:
      VZ_TRACE_EXIT(vmm, hypervisor_gpsi_cp0_exits);
      er = vz_emulate_CP0(inst, opc, cause, vmm);
      break;
    case cache_op:
      VZ_TRACE_EXIT(vmm, hypervisor_gpsi_cache_exits);
      break;

    default:
      karma_log(WARN, "[%#08lx] GPSI exception not supported: inst %#08lx\n", (l4_umword_t)opc, inst);
      er = EMULATE_FAIL;
      break;
  }

  return er;
}

enum emulation_result
vz_trap_handle_gsfc(l4_umword_t cause, l4_umword_t *opc, vz_vmm_t *vmm)
{
  enum emulation_result er = EMULATE_DONE;
  l4_umword_t inst = get_badinstr(vmm);
  (void)cause;

  if (vmm_regs(vmm).cause & CAUSEF_BD)
    opc += 1;

  /* complete MTC0 on behalf of guest */
  if ((inst & COP0MT) == MTC0) {
    int rt = (inst & RT) >> 16;
    int val = vmm_regs(vmm).r[rt];
    int rd = (inst & RD) >> 11;
    int sel = (inst & SEL);

    if ((rd == MIPS_CP0_STATUS) && (sel == 0)) {
      VZ_TRACE_EXIT(vmm, hypervisor_gsfc_cp0_status_exits);
      vmm->vmstate->gcp0[MIPS_CP0_STATUS][0] = val;
    } else if ((rd == MIPS_CP0_CAUSE) && (sel == 0)) {
      VZ_TRACE_EXIT(vmm, hypervisor_gsfc_cp0_cause_exits);
      vmm->vmstate->gcp0[MIPS_CP0_CAUSE][0] = val;
    } else if ((rd == MIPS_CP0_INTCTL) && (sel == MIPS_CP0_INTCTL_SEL)) {
      VZ_TRACE_EXIT(vmm, hypervisor_gsfc_cp0_intctl_exits);
      vmm->vmstate->gcp0[MIPS_CP0_INTCTL][MIPS_CP0_INTCTL_SEL] = val;
    } else {
      karma_log(WARN, "[%#08lx] Handle GSFC, unsupported field change: inst %#08lx\n", (l4_umword_t)opc, inst);
      er = EMULATE_FAIL;
    }
  } else {
    karma_log(WARN, "[%#08lx] Handle GSFC, unrecognized instruction: inst %#08lx\n", (l4_umword_t)opc, inst);
    er = EMULATE_FAIL;
  }

  return er;
}

enum emulation_result
vz_trap_handle_hypcall(l4_umword_t cause, l4_umword_t *opc, vz_vmm_t *vmm)
{
  enum emulation_result er = EMULATE_DONE;
  l4_umword_t inst = get_badinstr(vmm);
  l4_umword_t hypcall_code = (inst >> 11) & 0x3ff;
  (void)cause;

  if (vmm_regs(vmm).cause & CAUSEF_BD)
    opc += 1;

  karma_log(DEBUG, "[%#08lx] Handle HYPCALL code %lx\n", (l4_umword_t)opc, hypcall_code);

  if (hypcall_code == HYPCALL_MAGIC_EXIT) {
    karma_log(INFO, "VM EXIT CODE!!!!\n");
    exit_flag = 1;
  } else if (hypcall_code == HYPCALL_KARMA_DEV_OP) {
    HypercallPayload hp;
    serial_do_hypcall(vmm, hp);
  }

  return er;
}

enum emulation_result
vz_trap_default_handler(l4_umword_t cause,
                        l4_umword_t *opc, vz_vmm_t *vmm)
{
  (void)cause;
  l4_umword_t gexccode = vmm->vmstate->exit_gexccode;
  l4_umword_t inst;

  switch (vmm->vmstate->exit_gexccode) {
    case GUESTCTL0_GEXC_GPSI:
    case GUESTCTL0_GEXC_GSFC:
    case GUESTCTL0_GEXC_HC:
    case GUESTCTL0_GEXC_GRR:

      inst = get_badinstr(vmm);

      if (vmm_regs(vmm).cause & CAUSEF_BD)
        opc += 1;

      karma_log(WARN, 
        "[%#08lx] Guest Exception Code %lx:%s not yet handled: "
        "inst %#08lx Guest Status %#08lx\n",
            (l4_umword_t)opc, gexccode,
             (gexccode == GUESTCTL0_GEXC_GPSI ? "GPSI" :
              (gexccode == GUESTCTL0_GEXC_GSFC ? "GSFC" :
               (gexccode == GUESTCTL0_GEXC_HC ? "HC" :
                (gexccode == GUESTCTL0_GEXC_GRR ? "GRR" :
                  "RESV")))), inst, vmm->vmstate->gcp0[MIPS_CP0_STATUS][0]);
      break;

    case GUESTCTL0_GEXC_GVA:
    case GUESTCTL0_GEXC_GHFC:
    case GUESTCTL0_GEXC_GPA:
    default:
      karma_log(WARN, 
        "[%#08lx] Guest Exception Code %lx:%s not yet handled. "
        "Guest Status %#08lx\n",
            (l4_umword_t)opc, gexccode,
             (gexccode == GUESTCTL0_GEXC_GVA ? "GVA" :
              (gexccode == GUESTCTL0_GEXC_GHFC ? "GHFC" :
               (gexccode == GUESTCTL0_GEXC_GPA ? "GPA" :
                "RESV"))), vmm->vmstate->gcp0[MIPS_CP0_STATUS][0]);
      break;
  }

  return EMULATE_FAIL;
}

int vz_trap_handle_ge(vz_vmm_t *vmm)
{
  l4_umword_t *opc = (l4_umword_t *) vmm_regs(vmm).ip;
  l4_umword_t cause = vmm_regs(vmm).cause;
  enum emulation_result er = EMULATE_DONE;
  int ret = RESUME_GUEST;
  l4_umword_t gexccode = vmm->vmstate->exit_gexccode;

#if 0
  karma_log(DEBUG, "Hypervisor Guest Exit. GExcCode %lx:%s\n",
        gexccode,
         (gexccode == GUESTCTL0_GEXC_GPSI ? "GPSI" :
          (gexccode == GUESTCTL0_GEXC_GSFC ? "GSFC" :
           (gexccode == GUESTCTL0_GEXC_HC ? "HC" :
            (gexccode == GUESTCTL0_GEXC_GRR ? "GRR" :
             (gexccode == GUESTCTL0_GEXC_GVA ? "GVA" :
              (gexccode == GUESTCTL0_GEXC_GHFC ? "GHFC" :
               (gexccode == GUESTCTL0_GEXC_GPA ? "GPA" :
                "RESV"))))))));
#endif

  switch (gexccode) {
    case GUESTCTL0_GEXC_GPSI:
      VZ_TRACE_EXIT(vmm, hypervisor_gpsi_exits);
      er = vz_trap_handle_gpsi(cause, opc, vmm);
      break;
    case GUESTCTL0_GEXC_GSFC:
      VZ_TRACE_EXIT(vmm, hypervisor_gsfc_exits);
      er = vz_trap_handle_gsfc(cause, opc, vmm);
      break;
    case GUESTCTL0_GEXC_HC:
      VZ_TRACE_EXIT(vmm, hypervisor_hc_exits);
      er = vz_trap_handle_hypcall(cause, opc, vmm);
      break;
    case GUESTCTL0_GEXC_GRR:
      VZ_TRACE_EXIT(vmm, hypervisor_grr_exits);
      er = vz_trap_default_handler(cause, opc, vmm);
      break;
    case GUESTCTL0_GEXC_GVA:
      VZ_TRACE_EXIT(vmm, hypervisor_gva_exits);
      er = vz_trap_default_handler(cause, opc, vmm);
      break;
    case GUESTCTL0_GEXC_GHFC:
      VZ_TRACE_EXIT(vmm, hypervisor_ghfc_exits);
      er = vz_trap_default_handler(cause, opc, vmm);
      break;
    case GUESTCTL0_GEXC_GPA:
      VZ_TRACE_EXIT(vmm, hypervisor_gpa_exits);
      er = vz_trap_default_handler(cause, opc, vmm);
      break;
    default:
      VZ_TRACE_EXIT(vmm, hypervisor_resv_exits);
      er = vz_trap_default_handler(cause, opc, vmm);
      break;
  }

  if (er == EMULATE_DONE)
    ret = RESUME_GUEST;
  else {
    vmm->vmstate->exit_reason = L4_vm_exit_reason_undef;
    ret = RESUME_HOST;
  }
  return ret;
}

/* simplistic conversion of pfa guest PA to root VA */
#define STACK_GUESTPA_TO_ROOTVA(_pfa) \
            ((_pfa) - vmm->vm_guest_mapbase + vmm->host_addr_mapbase)

int vz_tlb_handler(vz_vmm_t *vmm)
{
  int ret = RESUME_GUEST;

  l4_addr_t pfa = vmm_regs(vmm).badvaddr;
  l4_addr_t pfa_rootva;
  l4_addr_t addr;
  unsigned long size = 1;
  l4_addr_t offset;
  unsigned flags;
  l4re_ds_t ds;
  int result;

  pfa_rootva = STACK_GUESTPA_TO_ROOTVA(pfa);
  addr = pfa_rootva;

  /* lookup root VA in region manager, addr and size are overwritten */
  result = l4re_rm_find(&addr, &size, &offset, &flags, &ds);
  if (result) {
    karma_log(WARN, "vz_tlb_handler: address %08lx is not mappable to VM. Err %s\n",
        pfa, l4sys_errtostr(result));
    ret = RESUME_HOST;
  } else if (flags & L4RE_RM_READ_ONLY) {
    karma_log(WARN, "vz_tlb_handler: address %08lx is readonly.\n", pfa);
    ret = RESUME_HOST;
  } else {
    /* just map the one page containing the pfa */
    map_mem(vmm, (pfa & L4_PAGEMASK), (pfa_rootva & L4_PAGEMASK), L4_PAGESIZE,
        memmap_write, memmap_not_io);
  }
  return ret;
}

void vz_update_pc(vz_vmm_t *vmm)
{
  vmm_regs(vmm).ip += 4;
}

int vz_trap_handler(vz_vmm_t *vmm)
{
  int ret = RESUME_GUEST;
  l4_umword_t *opc = (l4_umword_t *) vmm_regs(vmm).ip;
  l4_umword_t badvaddr = vmm_regs(vmm).badvaddr;
  l4_umword_t cause = vmm_regs(vmm).cause;
  l4_umword_t exccode = ((cause & CAUSEF_EXCCODE) >> CAUSEB_EXCCODE);

  switch (exccode) {
    case Exc_code_Int:
      VZ_TRACE_EXIT(vmm, int_exits);
      break;

    case Exc_code_CpU:
      VZ_TRACE_EXIT(vmm, cop_unusable_exits);
      vz_update_pc(vmm);
      break;

    case Exc_code_Mod:
      VZ_TRACE_EXIT(vmm, tlbmod_exits);
      vz_tlb_handler(vmm);
      break;

    case Exc_code_TLBS:
      VZ_TRACE_EXIT(vmm, tlbmiss_st_exits);
      vz_tlb_handler(vmm);
      break;

    case Exc_code_TLBL:
      VZ_TRACE_EXIT(vmm, tlbmiss_ld_exits);
      vz_tlb_handler(vmm);
      break;

    case Exc_code_GE:
      ret = vz_trap_handle_ge(vmm);
      vz_update_pc(vmm);
      break;

    default:
      karma_log(WARN, "Exception Code: %lx, not yet handled, @ PC: %p, inst: 0x%08lx  BadVaddr: %#08lx Status: %#08lx\n",
          exccode, opc, get_badinstr(vmm), badvaddr, vmm->vmstate->gcp0[MIPS_CP0_STATUS][0]);
      ret = RESUME_HOST;
      break;
  }

  if (exit_flag)
    ret = RESUME_EXIT;

  return ret;
}

int vz_init(vz_vmm_t *vmm)
{
  long result;
  l4_utcb_t *u = l4_utcb();
  l4_umword_t readval;
  l4_umword_t guestctl0;

  /* Validate vmstate before first use */
  if (vmm->vmstate->sizeof_vmstate != sizeof(*vmm->vmstate)) {
      karma_log(ERROR,
          "vz_init failed. VM state does not match kernel. Size mismatch: %ld, expected %d\n",
          vmm->vmstate->sizeof_vmstate, sizeof(*vmm->vmstate));
      return -1;
  }

  /* Selectively enable/disable guest virtualization features */
  guestctl0 = vmm->vmstate->host_cp0_guestctl0;
  guestctl0 |= GUESTCTL0_CP0 |
               GUESTCTL0_AT3 |
               GUESTCTL0_CG |
               GUESTCTL0_CF;

  /* Disable guest access to timer */
  guestctl0 &= ~GUESTCTL0_GT;

  /* Apply change to register */
  result = fiasco_mips_cp0_set(vmm->vmtask,
      MIPS_CP0_GUESTCTL0, MIPS_CP0_GUESTCTL0_SEL, guestctl0, u);
  if (result) {
      karma_log(ERROR, "Failed to set guestctl0 register: %s\n", l4sys_errtostr(result));
      return -1;
  }

  /* Check register (for debug only) */
  result = fiasco_mips_cp0_get(vmm->vmtask,
      MIPS_CP0_GUESTCTL0, MIPS_CP0_GUESTCTL0_SEL, &readval, u);
  if (result) {
      karma_log(ERROR, "Failed to get guestctl0 register: %s\n", l4sys_errtostr(result));
      return -1;
  }

  karma_log(DEBUG, "guestctl0 write 0x%08lx read 0x%08lx\n", guestctl0, readval);

  /* Emulate/Initialize guest PRId */
  result = fiasco_mips_cp0_get(vmm->vmtask,
      MIPS_CP0_PRID, MIPS_CP0_PRID_SEL, &readval, u);
  if (result) {
      karma_log(ERROR, "Failed to get prid register: %s\n", l4sys_errtostr(result));
      return -1;
  }
  vmm->vmstate->gcp0[MIPS_CP0_PRID][MIPS_CP0_PRID_SEL] = readval;

  /* Modify guest cp0 config registers. Changes are applied during VM resume. */

  /* Disable guest DSP */
  vmm->vmstate->gcp0[MIPS_CP0_CONFIG][3] &= ~(MIPS_CONF3_DSP | MIPS_CONF3_DSP2P);

  return 0;
}
