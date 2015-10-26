/*
 * Copyright (C) 2015 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

#include "types.h"

#define KDB_KE_USER_TRAP  1
#define KDB_KE_USER_SEQ   2
#define KDB_KE_IPI        3
#define KDB_KE_BREAKPOINT 4

inline ALWAYS_INLINE void kdb_ke(const char *msg);

IMPLEMENTATION [mips32]:

#include "inst.h"

class Kdb_ke
{
  public:
  union mips_instruction kdb_ke_break_insn;
  union mips_instruction kdb_ke_breakpoint_insn;
};

static const Kdb_ke kdb;

PUBLIC
Kdb_ke::Kdb_ke()
{
  kdb_ke_break_insn.b_format.opcode = spec_op;
  kdb_ke_break_insn.b_format.code = 0;
  kdb_ke_break_insn.b_format.func = break_op;

  kdb_ke_breakpoint_insn.b_format.opcode = spec_op;
  kdb_ke_breakpoint_insn.b_format.code = KDB_KE_BREAKPOINT;
  kdb_ke_breakpoint_insn.b_format.func = break_op;
}

bool
kdb_ke_is_break_opcode(Mword opcode)
{
  return ((opcode & ~(0xfffff << 6)) == kdb.kdb_ke_break_insn.word);
}

Mword
kdb_ke_breakpoint_opcode()
{
  return kdb.kdb_ke_breakpoint_insn.word;
}

IMPLEMENTATION [mips32 && debug]:

inline ALWAYS_INLINE
void kdb_ke(const char *msg)
{
  register const char *a0 asm ("a0") = msg;
  asm volatile (
        "    nop                                      \n"
        "    break   %[trap_code_hi],%[trap_code_lo]  \n"
        : "+r" (a0)
        : [trap_code_hi] "n" (0),
          [trap_code_lo] "n" (KDB_KE_USER_TRAP)
        :
      );
}

inline ALWAYS_INLINE
void kdb_ke_sequence(const char *msg)
{
  register const char *a0 asm ("a0") = msg;
  asm volatile (
        "    nop                                      \n"
        "    break   %[trap_code_hi],%[trap_code_lo]  \n"
        : "+r" (a0)
        : [trap_code_hi] "n" (0),
          [trap_code_lo] "n" (KDB_KE_USER_SEQ)
        :
      );
}

IMPLEMENTATION [mips32 && debug && mp]:

inline ALWAYS_INLINE
void kdb_ke_ipi()
{
  asm volatile (
        "    nop                                      \n"
        "    break   %[trap_code_hi],%[trap_code_lo]  \n"
        :
        : [trap_code_hi] "n" (0),
          [trap_code_lo] "n" (KDB_KE_IPI)
        :
      );
}

IMPLEMENTATION [mips32 && !debug]:

inline ALWAYS_INLINE
void kdb_ke(const char *)
{}

inline ALWAYS_INLINE
void kdb_ke_sequence(const char *)
{}

inline ALWAYS_INLINE
void kdb_ke_ipi()
{}
