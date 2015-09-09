/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE[mips32]:

#include "types.h"
#include "mipsdefs.h"

/* 
 * Entry_frame = Syscall_frame[32] + Return_frame[status, lo, hi, epc ...]
 */
EXTENSION class Syscall_frame
{
  public:
    Mword r[32]; //GPRs
    void dump() const;

    enum
    {
      REG_ZERO = (FRAME_REG_ZERO - FRAME_TRAP_REGS_NR),
      REG_AST = (FRAME_REG_AST - FRAME_TRAP_REGS_NR),
      REG_V0 = (FRAME_REG_V0 - FRAME_TRAP_REGS_NR),
      REG_V1 = (FRAME_REG_V1 - FRAME_TRAP_REGS_NR),
      REG_A0 = (FRAME_REG_A0 - FRAME_TRAP_REGS_NR),
      REG_A1 = (FRAME_REG_A1 - FRAME_TRAP_REGS_NR),
      REG_A2 = (FRAME_REG_A2 - FRAME_TRAP_REGS_NR),
      REG_A3 = (FRAME_REG_A3 - FRAME_TRAP_REGS_NR),
      REG_T0 = (FRAME_REG_T0 - FRAME_TRAP_REGS_NR),
      REG_T1 = (FRAME_REG_T1 - FRAME_TRAP_REGS_NR),
      REG_T2 = (FRAME_REG_T2 - FRAME_TRAP_REGS_NR),
      REG_T3 = (FRAME_REG_T3 - FRAME_TRAP_REGS_NR),
      REG_T4 = (FRAME_REG_T4 - FRAME_TRAP_REGS_NR),
      REG_T5 = (FRAME_REG_T5 - FRAME_TRAP_REGS_NR),
      REG_T6 = (FRAME_REG_T6 - FRAME_TRAP_REGS_NR),
      REG_T7 = (FRAME_REG_T7 - FRAME_TRAP_REGS_NR),
      REG_S0 = (FRAME_REG_S0 - FRAME_TRAP_REGS_NR),
      REG_S1 = (FRAME_REG_S1 - FRAME_TRAP_REGS_NR),
      REG_S2 = (FRAME_REG_S2 - FRAME_TRAP_REGS_NR),
      REG_S3 = (FRAME_REG_S3 - FRAME_TRAP_REGS_NR),
      REG_S4 = (FRAME_REG_S4 - FRAME_TRAP_REGS_NR),
      REG_S5 = (FRAME_REG_S5 - FRAME_TRAP_REGS_NR),
      REG_S6 = (FRAME_REG_S6 - FRAME_TRAP_REGS_NR),
      REG_S7 = (FRAME_REG_S7 - FRAME_TRAP_REGS_NR),
      REG_T8 = (FRAME_REG_T8 - FRAME_TRAP_REGS_NR),
      REG_T9 = (FRAME_REG_T9 - FRAME_TRAP_REGS_NR),
      REG_K0 = (FRAME_REG_K0 - FRAME_TRAP_REGS_NR),
      REG_K1 = (FRAME_REG_K1 - FRAME_TRAP_REGS_NR),
      REG_GP = (FRAME_REG_GP - FRAME_TRAP_REGS_NR),
      REG_SP = (FRAME_REG_SP - FRAME_TRAP_REGS_NR),
      REG_S8 = (FRAME_REG_S8 - FRAME_TRAP_REGS_NR),
      REG_RA = (FRAME_REG_RA - FRAME_TRAP_REGS_NR),
    };
};

EXTENSION class Return_frame
{
  public:
    Mword lo, hi;
    Mword status;
    Mword epc;
    Mword cause;
    Mword badvaddr;

    /* usp is required for continuations, put it @ the end */
    Mword usp;

    void dump();
    bool user_mode();
};

//------------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

#include "warn.h"
#include "cp0_status.h"
#include "mipsdefs.h"

IMPLEMENT
void
Syscall_frame::dump() const
{
  printf("r0[%2d]: %08lx at[%2d]: %08lx v0[%2d]: %08lx v1[%2d]: %08lx\n",
    0, r[0], 1, r[1], 2, r[2], 3, r[3]);
  printf("a0[%2d]: %08lx a1[%2d]: %08lx a2[%2d]: %08lx a3[%2d]: %08lx\n",
    4, r[4], 5, r[5], 6, r[6], 7, r[7]);
  printf("t0[%2d]: %08lx t1[%2d]: %08lx t2[%2d]: %08lx t3[%2d]: %08lx\n",
    8, r[8], 9, r[9], 10, r[10], 11, r[11]);
  printf("t4[%2d]: %08lx t5[%2d]: %08lx t6[%2d]: %08lx t7[%2d]: %08lx\n",
    12, r[12], 13, r[13], 14, r[14], 15, r[15]);
  printf("s0[%2d]: %08lx s1[%2d]: %08lx s2[%2d]: %08lx s3[%2d]: %08lx\n",
    16, r[16], 17, r[17], 18, r[18], 19, r[19]);
  printf("s4[%2d]: %08lx s5[%2d]: %08lx s6[%2d]: %08lx s7[%2d]: %08lx\n",
    20, r[20], 21, r[21], 22, r[22], 23, r[23]);
  printf("t8[%2d]: %08lx t9[%2d]: %08lx k0[%2d]: %08lx k1[%2d]: %08lx\n",
    24, r[24], 25, r[25], 26, r[26], 27, r[27]);
  printf("gp[%2d]: %08lx sp[%2d]: %08lx s8[%2d]: %08lx ra[%2d]: %08lx\n",
    28, r[28], 29, r[29], 30, r[30], 31, r[31]);
}

IMPLEMENT
void
Return_frame::dump()
{
  printf("Status %08lx EPC %08lx SP %08lx\n", status, epc, usp);
  printf("Cause  %08lx BadVaddr %08lx\n", cause, badvaddr);
  printf("HI: %08lx LO: %08lx\n", hi, lo);
}

IMPLEMENT inline
Mword
Return_frame::sp() const
{
  return Return_frame::usp;
}

IMPLEMENT inline
void
Return_frame::sp(Mword _sp)
{
  Return_frame::usp = _sp;
}

IMPLEMENT inline
Mword
Return_frame::ip() const
{
  return Return_frame::epc;
}

IMPLEMENT inline
void
Return_frame::ip(Mword _pc)
{
  Return_frame::epc = _pc;
}

IMPLEMENT inline NEEDS ["cp0_status.h"]
bool
Return_frame::user_mode()
{
  return Cp0_Status::ST_KSU_USER & status;
}

IMPLEMENT inline
Mword
Return_frame::ip_syscall_page_user() const
{ return Return_frame::epc; }

//---------------------------------------------------------------------------
IMPLEMENT inline
Mword Syscall_frame::next_period() const
{ return false; }

IMPLEMENT inline
void Syscall_frame::from(Mword id)
{ r[Syscall_frame::REG_S4] = id; } // r[20]

IMPLEMENT inline
Mword Syscall_frame::from_spec() const
{ return r[Syscall_frame::REG_S4]; } // r[20]

IMPLEMENT inline
L4_obj_ref Syscall_frame::ref() const
{ return L4_obj_ref::from_raw(r[Syscall_frame::REG_S1]); } // r[17]

IMPLEMENT inline
void Syscall_frame::ref(L4_obj_ref const &ref)
{ r[Syscall_frame::REG_S1] = ref.raw(); } // r[17]

IMPLEMENT inline
L4_timeout_pair Syscall_frame::timeout() const
{ return L4_timeout_pair(r[Syscall_frame::REG_S2]); } // r[18]

IMPLEMENT inline 
void Syscall_frame::timeout(L4_timeout_pair const &to)
{ r[Syscall_frame::REG_S2] = to.raw(); } // r[18]

IMPLEMENT inline Utcb *Syscall_frame::utcb() const
{ return reinterpret_cast<Utcb*>(r[Syscall_frame::REG_S0]); } // r[16]

IMPLEMENT inline L4_msg_tag Syscall_frame::tag() const
{ return L4_msg_tag(r[Syscall_frame::REG_S3]); } // r[19]

IMPLEMENT inline
void Syscall_frame::tag(L4_msg_tag const &tag)
{ r[Syscall_frame::REG_S3] = tag.raw(); } //r[19]
