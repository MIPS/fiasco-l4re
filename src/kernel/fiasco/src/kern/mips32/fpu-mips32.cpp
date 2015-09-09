INTERFACE [mips32 && fpu]:

#include <cxx/bitfield>
#include "mipsregs.h"

EXTENSION class Fpu
{
public:
  struct Fpu_regs
  {
    Unsigned64 regs[32];
    Mword fcsr;
  };

  struct Fir
  {
    Mword v;

    Fir() = default;
    explicit Fir(Mword v) : v(v) {}

    CXX_BITFIELD_MEMBER(0, 7, Revision, v);
    CXX_BITFIELD_MEMBER(8, 15, ProcessorID, v);
    CXX_BITFIELD_MEMBER(16, 16, S, v);
    CXX_BITFIELD_MEMBER(17, 17, D, v);
    CXX_BITFIELD_MEMBER(18, 18, PS, v);
    //CXX_BITFIELD_MEMBER(19, 19, _3D, v);
    CXX_BITFIELD_MEMBER(20, 20, W, v);
    CXX_BITFIELD_MEMBER(21, 21, L, v);
    CXX_BITFIELD_MEMBER(22, 22, F64, v);
    //CXX_BITFIELD_MEMBER(23, 23, Has2008, v);
    //CXX_BITFIELD_MEMBER(24, 24, FC, v);
    CXX_BITFIELD_MEMBER(28, 28, UFRP, v);
    CXX_BITFIELD_MEMBER(29, 29, FREP, v);
  };

  Fir fir() const { return _fir; }

private:
  enum {
    NoDebug = 0,
    Debug1 = 1,
    Debug2 = 2,
    Debug3 = 3,
  };

  Fir _fir;
};


// ------------------------------------------------------------------------
IMPLEMENTATION [mips32 && fpu]:

#include <cassert>
#include <cstdio>
#include <cstring>

#include "fpu_state.h"
#include "kdb_ke.h"
#include "mem.h"
#include "processor.h"
#include "static_assert.h"
#include "trap_state.h"
#include "mipsregs.h"
#include "cp0_status.h"
#include "cpu.h"


EXTENSION class Fpu
{
private:
  enum
  {
    /*
     * We initialize fcr31 to rounding to nearest, no exceptions.
     */
    CSR_DEFAULT = FPU_CSR_DEFAULT,

    /*
     * Load the FPU with signalling NANS.  This bit pattern we're using has
     * the property that no matter whether considered as single or as double
     * precision represents signaling NANS.
     */
    SNAN = -1,
  };
};

PUBLIC static inline
Mword
Fpu::fir_read()
{
  return read_32bit_cp1_register(CP1_REVISION);
}

PUBLIC static inline
Mword
Fpu::fcr_read()
{
  return read_32bit_cp1_register(CP1_STATUS);
}

PUBLIC static inline NEEDS ["fpu_state.h", <cassert>]
Mword
Fpu::fcr(Fpu_state *s)
{
  Fpu_regs *fpu_regs = reinterpret_cast<Fpu_regs *>(s->state_buffer());

  assert(fpu_regs);
  return fpu_regs->fcsr;
}

PUBLIC static inline
bool
Fpu::mode_64bit()
{
  return Cp0_Status::read_status() & Cp0_Status::ST_FR;
}

PRIVATE static
void
Fpu::show(Cpu_number cpu)
{
  const Fir f = fpu.cpu(cpu).fir();

  printf("FPU%d: fir:%08x ID:%x Rev:%x fp-type%s%s%s%s%s F64:%x UFRP:%x FREP:%x\n",
         cxx::int_value<Cpu_number>(cpu),
         (int)f.v,
         (int)f.ProcessorID(),
         (int)f.Revision(),
         f.S() ? ":S" : "",
         f.D() ? ":D" : "",
         f.PS() ? ":PS" : "",
         f.W() ? ":W" : "",
         f.L() ? ":L" : "",
         (int)f.F64(), (int)f.UFRP(), (int)f.FREP());
}

IMPLEMENT
void
Fpu::init(Cpu_number cpu, bool resume)
{
  Fpu &f = fpu.cpu(cpu);

  f.enable();
  f._fir = Fir(fir_read());

  if (!resume)
    show(cpu);

  f.disable();
  f.set_owner(0);
}

PRIVATE static inline
void
Fpu::fpu_save_16even(Fpu_regs *r)
{
  Mword tmp;

  asm volatile(".set   push\n");
  asm volatile(".set   hardfloat\n");
  asm volatile("cfc1   %0,   $31\n" : "=r" (tmp));
  asm volatile("sw     %1,   %0 \n" : "=m" (r->fcsr) : "r" (tmp));
  asm volatile("sdc1   $f0,  %0 \n" : "=m" (r->regs[0]));
  asm volatile("sdc1   $f2,  %0 \n" : "=m" (r->regs[2]));
  asm volatile("sdc1   $f4,  %0 \n" : "=m" (r->regs[4]));
  asm volatile("sdc1   $f6,  %0 \n" : "=m" (r->regs[6]));
  asm volatile("sdc1   $f8,  %0 \n" : "=m" (r->regs[8]));
  asm volatile("sdc1   $f10, %0 \n" : "=m" (r->regs[10]));
  asm volatile("sdc1   $f12, %0 \n" : "=m" (r->regs[12]));
  asm volatile("sdc1   $f14, %0 \n" : "=m" (r->regs[14]));
  asm volatile("sdc1   $f16, %0 \n" : "=m" (r->regs[16]));
  asm volatile("sdc1   $f18, %0 \n" : "=m" (r->regs[18]));
  asm volatile("sdc1   $f20, %0 \n" : "=m" (r->regs[20]));
  asm volatile("sdc1   $f22, %0 \n" : "=m" (r->regs[22]));
  asm volatile("sdc1   $f24, %0 \n" : "=m" (r->regs[24]));
  asm volatile("sdc1   $f26, %0 \n" : "=m" (r->regs[26]));
  asm volatile("sdc1   $f28, %0 \n" : "=m" (r->regs[28]));
  asm volatile("sdc1   $f30, %0 \n" : "=m" (r->regs[30]));
  asm volatile(".set   pop\n");
}

PRIVATE static inline
void
Fpu::fpu_save_16odd(Fpu_regs *r)
{
  asm volatile(".set   push\n");
  asm volatile(".set   hardfloat\n");
  asm volatile(".set   mips32r2\n");
  asm volatile(".set   fp=64\n");
  asm volatile("sdc1   $f1,  %0 \n" : "=m" (r->regs[1]));
  asm volatile("sdc1   $f3,  %0 \n" : "=m" (r->regs[3]));
  asm volatile("sdc1   $f5,  %0 \n" : "=m" (r->regs[5]));
  asm volatile("sdc1   $f7,  %0 \n" : "=m" (r->regs[7]));
  asm volatile("sdc1   $f9,  %0 \n" : "=m" (r->regs[9]));
  asm volatile("sdc1   $f11, %0 \n" : "=m" (r->regs[11]));
  asm volatile("sdc1   $f13, %0 \n" : "=m" (r->regs[13]));
  asm volatile("sdc1   $f15, %0 \n" : "=m" (r->regs[15]));
  asm volatile("sdc1   $f17, %0 \n" : "=m" (r->regs[17]));
  asm volatile("sdc1   $f19, %0 \n" : "=m" (r->regs[19]));
  asm volatile("sdc1   $f21, %0 \n" : "=m" (r->regs[21]));
  asm volatile("sdc1   $f23, %0 \n" : "=m" (r->regs[23]));
  asm volatile("sdc1   $f25, %0 \n" : "=m" (r->regs[25]));
  asm volatile("sdc1   $f27, %0 \n" : "=m" (r->regs[27]));
  asm volatile("sdc1   $f29, %0 \n" : "=m" (r->regs[29]));
  asm volatile("sdc1   $f31, %0 \n" : "=m" (r->regs[31]));
  asm volatile(".set   pop\n");
}

PRIVATE static inline
void
Fpu::fpu_restore_16even(Fpu_regs *r)
{
  Mword tmp;

  asm volatile(".set   push\n");
  asm volatile(".set   hardfloat\n");
  asm volatile("ldc1   $f0,  %0 \n" :: "m" (r->regs[0]));
  asm volatile("ldc1   $f2,  %0 \n" :: "m" (r->regs[2]));
  asm volatile("ldc1   $f4,  %0 \n" :: "m" (r->regs[4]));
  asm volatile("ldc1   $f6,  %0 \n" :: "m" (r->regs[6]));
  asm volatile("ldc1   $f8,  %0 \n" :: "m" (r->regs[8]));
  asm volatile("ldc1   $f10, %0 \n" :: "m" (r->regs[10]));
  asm volatile("ldc1   $f12, %0 \n" :: "m" (r->regs[12]));
  asm volatile("ldc1   $f14, %0 \n" :: "m" (r->regs[14]));
  asm volatile("ldc1   $f16, %0 \n" :: "m" (r->regs[16]));
  asm volatile("ldc1   $f18, %0 \n" :: "m" (r->regs[18]));
  asm volatile("ldc1   $f20, %0 \n" :: "m" (r->regs[20]));
  asm volatile("ldc1   $f22, %0 \n" :: "m" (r->regs[22]));
  asm volatile("ldc1   $f24, %0 \n" :: "m" (r->regs[24]));
  asm volatile("ldc1   $f26, %0 \n" :: "m" (r->regs[26]));
  asm volatile("ldc1   $f28, %0 \n" :: "m" (r->regs[28]));
  asm volatile("ldc1   $f30, %0 \n" :: "m" (r->regs[30]));
  asm volatile("lw     %0,   %1 \n" : "=r" (tmp) : "m" (r->fcsr));
  asm volatile("ctc1   %0,   $31\n" :: "r" (tmp));
  asm volatile(".set   pop\n");
}

PRIVATE static inline
void
Fpu::fpu_restore_16odd(Fpu_regs *r)
{
  asm volatile(".set   push\n");
  asm volatile(".set   hardfloat\n");
  asm volatile(".set   mips32r2\n");
  asm volatile(".set   fp=64\n");
  asm volatile("ldc1   $f1,  %0 \n" :: "m" (r->regs[1]));
  asm volatile("ldc1   $f3,  %0 \n" :: "m" (r->regs[3]));
  asm volatile("ldc1   $f5,  %0 \n" :: "m" (r->regs[5]));
  asm volatile("ldc1   $f7,  %0 \n" :: "m" (r->regs[7]));
  asm volatile("ldc1   $f9,  %0 \n" :: "m" (r->regs[9]));
  asm volatile("ldc1   $f11, %0 \n" :: "m" (r->regs[11]));
  asm volatile("ldc1   $f13, %0 \n" :: "m" (r->regs[13]));
  asm volatile("ldc1   $f15, %0 \n" :: "m" (r->regs[15]));
  asm volatile("ldc1   $f17, %0 \n" :: "m" (r->regs[17]));
  asm volatile("ldc1   $f19, %0 \n" :: "m" (r->regs[19]));
  asm volatile("ldc1   $f21, %0 \n" :: "m" (r->regs[21]));
  asm volatile("ldc1   $f23, %0 \n" :: "m" (r->regs[23]));
  asm volatile("ldc1   $f25, %0 \n" :: "m" (r->regs[25]));
  asm volatile("ldc1   $f27, %0 \n" :: "m" (r->regs[27]));
  asm volatile("ldc1   $f29, %0 \n" :: "m" (r->regs[29]));
  asm volatile("ldc1   $f31, %0 \n" :: "m" (r->regs[31]));
  asm volatile(".set   pop\n");
}

IMPLEMENT
void
Fpu::save_state(Fpu_state *s)
{
  Fpu_regs *fpu_regs = reinterpret_cast<Fpu_regs *>(s->state_buffer());

  assert(fpu_regs);

  if (fpu_debug())
    printf("###SAV own %p %p %s\n", Fpu::fpu.current().owner(), fpu_regs, Cpu::vz_str());

  if (Fpu::mode_64bit())
    fpu_save_16odd(fpu_regs);

  fpu_save_16even(fpu_regs);
}

IMPLEMENT
void
Fpu::restore_state(Fpu_state *s)
{
  Fpu_regs *fpu_regs = reinterpret_cast<Fpu_regs *>(s->state_buffer());

  assert(fpu_regs);

  if (fpu_debug())
    printf("###RES fpu_regs %p %s\n", fpu_regs, Cpu::vz_str());

  if (Fpu::mode_64bit())
    fpu_restore_16odd(fpu_regs);

  fpu_restore_16even(fpu_regs);
}

IMPLEMENT inline
unsigned
Fpu::state_size()
{ return sizeof (Fpu_regs); }

IMPLEMENT inline
unsigned
Fpu::state_align()
{ return sizeof(Unsigned64); }

IMPLEMENT
void
Fpu::init_state(Fpu_state *s)
{
  Fpu_regs *fpu_regs = reinterpret_cast<Fpu_regs *>(s->state_buffer());
  static_assert(!(sizeof (*fpu_regs) % sizeof(Mword)),
                "Non-mword size of Fpu_regs");
  Mem::memset_mwords(fpu_regs, Fpu::SNAN, sizeof (*fpu_regs) / sizeof(Mword));

  fpu_regs->fcsr = Fpu::CSR_DEFAULT;

  if (fpu_debug())
    printf("###INI cur %p fpu_regs %p %s\n", current(), fpu_regs, Cpu::vz_str());
}

PUBLIC static inline NEEDS ["cp0_status.h"]
bool
Fpu::is_enabled()
{
  return Cp0_Status::read_status() & Cp0_Status::ST_CU1;
}

PUBLIC static inline NEEDS ["cpu.h", "cp0_status.h"]
void
Fpu::enable()
{
  Cp0_Status::set_status_bit(Cp0_Status::ST_CU1);

  if (fpu_debug() >= Fpu::Debug2)
    printf("###ENA cur %p own %p %s\n", current(), Fpu::fpu.current().owner(), Cpu::vz_str());
}

PUBLIC static inline NEEDS ["cpu.h", "cp0_status.h"]
void
Fpu::disable()
{
  Cp0_Status::clear_status_bit(Cp0_Status::ST_CU1);

  if (fpu_debug() >= Fpu::Debug2)
    printf("###DIS cur %p own %p %s\n", current(), Fpu::fpu.current().owner(), Cpu::vz_str());
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [mips32 && fpu && !debug]:

EXTENSION class Fpu
{
  public:
    static inline int fpu_debug() { return 0; };
    static inline void fpu_debug(int) {};
};


//-----------------------------------------------------------------------------
IMPLEMENTATION [mips32 && fpu && debug]:

EXTENSION class Fpu
{
  public:
    static inline int fpu_debug() { return _fpu_debug; };
    static inline void fpu_debug(int level) { _fpu_debug = level; };
  private:
    static int _fpu_debug;
};

int Fpu::_fpu_debug = 0;
