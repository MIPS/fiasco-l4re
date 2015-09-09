#pragma once

#include <l4/sys/l4int.h>
#include <l4/sys/vcpu.h>
#include <l4/sys/irq>
#include <l4/cxx/bitfield>

namespace Vmm {
namespace Arm {

class Hsr
{
public:
  Hsr() = default;
  explicit Hsr(l4_uint32_t ec) : _raw(ec) {}
  l4_uint32_t _raw;

  l4_uint32_t raw() const { return _raw; }

  CXX_BITFIELD_MEMBER(26, 31, ec, _raw);
  CXX_BITFIELD_MEMBER(25, 25, il, _raw);
  CXX_BITFIELD_MEMBER(24, 24, cv, _raw);
  CXX_BITFIELD_MEMBER(20, 23, cond, _raw);

  /** \pre ec == 0x01 */
  CXX_BITFIELD_MEMBER( 0,  0, wfe_trapped, _raw);

  CXX_BITFIELD_MEMBER(17, 19, mcr_opc2, _raw);
  CXX_BITFIELD_MEMBER(16, 19, mcrr_opc1, _raw);
  CXX_BITFIELD_MEMBER(14, 16, mcr_opc1, _raw);
  CXX_BITFIELD_MEMBER(10, 13, mcr_crn, _raw);
  CXX_BITFIELD_MEMBER(10, 13, mcrr_rt2, _raw);
  CXX_BITFIELD_MEMBER( 5,  8, mcr_rt, _raw);
  CXX_BITFIELD_MEMBER( 1,  4, mcr_crm, _raw);
  CXX_BITFIELD_MEMBER( 0,  0, mcr_read, _raw);

  CXX_BITFIELD_MEMBER(12, 19, ldc_imm, _raw);
  CXX_BITFIELD_MEMBER( 5,  8, ldc_rn, _raw);
  CXX_BITFIELD_MEMBER( 4,  4, ldc_offset_form, _raw);
  CXX_BITFIELD_MEMBER( 1,  3, ldc_addressing_mode, _raw);

  CXX_BITFIELD_MEMBER( 5,  5, cpt_simd, _raw);
  CXX_BITFIELD_MEMBER( 0,  3, cpt_cpnr, _raw);

  CXX_BITFIELD_MEMBER( 0,  3, bxj_rm, _raw);

  CXX_BITFIELD_MEMBER( 0, 15, svc_imm, _raw);

  CXX_BITFIELD_MEMBER(24, 24, pf_isv, _raw);
  CXX_BITFIELD_MEMBER(22, 23, pf_sas, _raw);
  CXX_BITFIELD_MEMBER(21, 21, pf_sse, _raw);
  CXX_BITFIELD_MEMBER(16, 19, pf_srt, _raw);
  CXX_BITFIELD_MEMBER( 9,  9, pf_ea, _raw);
  CXX_BITFIELD_MEMBER( 8,  8, pf_cache_maint, _raw);
  CXX_BITFIELD_MEMBER( 7,  7, pf_s1ptw, _raw);
  CXX_BITFIELD_MEMBER( 6,  6, pf_write, _raw);
  CXX_BITFIELD_MEMBER( 0,  5, pf_fsc, _raw);
};

namespace Gic_h {

  struct Hcr
  {
    l4_uint32_t raw;
    Hcr() = default;
    explicit Hcr(l4_uint32_t v) : raw(v) {}
    CXX_BITFIELD_MEMBER(  0,  0, en, raw);
    CXX_BITFIELD_MEMBER(  1,  1, uie, raw);
    CXX_BITFIELD_MEMBER(  2,  2, lr_en_pie, raw);
    CXX_BITFIELD_MEMBER(  3,  3, n_pie, raw);
    CXX_BITFIELD_MEMBER(  4,  4, vgrp0_eie, raw);
    CXX_BITFIELD_MEMBER(  5,  5, vgrp0_die, raw);
    CXX_BITFIELD_MEMBER(  6,  6, vgrp1_eie, raw);
    CXX_BITFIELD_MEMBER(  7,  7, vgrp1_die, raw);
    CXX_BITFIELD_MEMBER( 27, 31, eoi_cnt, raw);
  };

  struct Vtr
  {
    l4_uint32_t raw;
    Vtr() = default;
    explicit Vtr(l4_uint32_t v) : raw(v) {}
    CXX_BITFIELD_MEMBER(  0,  5, list_regs, raw);
    CXX_BITFIELD_MEMBER( 26, 28, pre_bits, raw);
    CXX_BITFIELD_MEMBER( 29, 31, pri_bits, raw);
  };

  struct Vmcr
  {
    l4_uint32_t raw;
    Vmcr() = default;
    explicit Vmcr(l4_uint32_t v) : raw(v) {}
    CXX_BITFIELD_MEMBER(  0,  0, grp0_en, raw);
    CXX_BITFIELD_MEMBER(  1,  1, grp1_en, raw);
    CXX_BITFIELD_MEMBER(  2,  2, ack_ctl, raw);
    CXX_BITFIELD_MEMBER(  3,  3, fiq_en, raw);
    CXX_BITFIELD_MEMBER(  4,  4, cbpr, raw);
    CXX_BITFIELD_MEMBER(  9,  9, vem, raw);
    CXX_BITFIELD_MEMBER( 18, 20, abp, raw);
    CXX_BITFIELD_MEMBER( 21, 23, bp, raw);
    CXX_BITFIELD_MEMBER( 27, 31, pri_mask, raw);
  };

  struct Misr
  {
    l4_uint32_t raw;
    Misr() = default;
    explicit Misr(l4_uint32_t v) : raw(v) {}
    CXX_BITFIELD_MEMBER(  0,  0, eoi, raw);
    CXX_BITFIELD_MEMBER(  1,  1, u, raw);
    CXX_BITFIELD_MEMBER(  2,  2, lrenp, raw);
    CXX_BITFIELD_MEMBER(  3,  3, np, raw);
    CXX_BITFIELD_MEMBER(  4,  4, grp0_e, raw);
    CXX_BITFIELD_MEMBER(  5,  5, grp0_d, raw);
    CXX_BITFIELD_MEMBER(  6,  6, grp1_e, raw);
    CXX_BITFIELD_MEMBER(  7,  7, grp1_d, raw);
  };

  struct Lr
  {
    enum State
    {
      Empty              = 0,
      Pending            = 1,
      Active             = 2,
      Active_and_pending = 3
    };

    l4_uint32_t raw;
    Lr() = default;
    explicit Lr(l4_uint32_t v) : raw(v) {}
    CXX_BITFIELD_MEMBER(  0,  9, vid, raw);
    CXX_BITFIELD_MEMBER( 10, 19, pid, raw);
    CXX_BITFIELD_MEMBER( 10, 12, cpuid, raw);
    CXX_BITFIELD_MEMBER( 19, 19, eoi, raw);
    CXX_BITFIELD_MEMBER( 23, 27, prio, raw);
    CXX_BITFIELD_MEMBER( 28, 29, state, raw);
    CXX_BITFIELD_MEMBER( 28, 28, pending, raw);
    CXX_BITFIELD_MEMBER( 29, 29, active, raw);
    CXX_BITFIELD_MEMBER( 30, 30, grp1, raw);
    CXX_BITFIELD_MEMBER( 31, 31, hw, raw);
  };
}

struct State
{
  struct Per_mode_regs
  {
    l4_umword_t sp;
    l4_umword_t lr;
    l4_umword_t spsr;
  };

  struct Regs
  {
    l4_uint32_t hcr;

    l4_uint64_t ttbr0;
    l4_uint64_t ttbr1;
    l4_uint32_t ttbcr;
    l4_uint32_t sctlr;
    l4_uint32_t dacr;
    l4_uint32_t fcseidr;
    l4_uint32_t contextidr;

    Per_mode_regs svc;
  };

  template< unsigned LREGS >
  struct Gic_t
  {
    Gic_h::Hcr hcr;
    Gic_h::Vtr vtr;
    Gic_h::Vmcr vmcr;
    Gic_h::Misr misr;
    l4_uint32_t eisr[2];
    l4_uint32_t elsr[2];
    l4_uint32_t apr;
    Gic_h::Lr lr[LREGS];
  };

  typedef Gic_t<4> Gic;

  Regs vm_regs;
  Regs host_regs;
  Gic  gic;

  l4_uint64_t cntvoff;

  l4_uint64_t cntv_cval;
  l4_uint32_t cntkctl;
  l4_uint32_t cntv_ctl;
};

inline State *
vm_state(l4_vcpu_state_t *vcpu)
{
  return reinterpret_cast<State *>(reinterpret_cast<char *>(vcpu) + 0x400);
}
}
}
