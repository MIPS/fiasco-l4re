/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

extern "C" void cpu_switchto (Context *old_ctx, Context *new_ctx, Mword **old_sp, Mword **new_sp);

EXTENSION class Context
{
private:
  //KYMAXXX check if we can use _kernel_sp for vm/vcpu without interfering
  Mword _saved_ksp;

  Mword _datalo1; 
  Mword _userlocal;
};

// ------------------------------------------------------------------------
IMPLEMENTATION [mips32]:

/** Note: TCB pointer is located in DDataLo and TLS is located in UserLocal */

#include <cassert>
#include <cstdio>

#include "l4_types.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "space.h"
#include "thread_state.h"
#include "warn.h"

#include "kmem.h"
#include "utcb_support.h"
#include "mem_op-mips32.h"
#include "mipsregs.h"

IMPLEMENT inline
void
Context::spill_user_state()
{}

IMPLEMENT inline
void
Context::fill_user_state()
{}

PROTECTED inline void Context::arch_setup_utcb_ptr()
{ 
  _datalo1 = reinterpret_cast<Mword>(utcb().usr().get());
}

PRIVATE inline NEEDS["mipsregs.h"]
void
Context::store_userlocal()
{
  _userlocal = read_c0_userlocal();
}

PRIVATE inline NEEDS["mipsregs.h"]
void
Context::load_userlocal() const
{
  write_c0_userlocal(_userlocal);
}

PRIVATE inline NEEDS["mipsregs.h"]
void
Context::store_datalo1()
{
  _datalo1 = read_c0_ddatalo();
}

PRIVATE inline NEEDS["mipsregs.h"]
void
Context::load_datalo1() const
{
  write_c0_ddatalo(_datalo1);
}



IMPLEMENT inline NEEDS[Context::store_userlocal, Context::load_userlocal, Context::store_datalo1, Context::load_datalo1]
void
Context::switch_cpu(Context *t)
{
  update_consumed_time();
  store_datalo1();
  store_userlocal();

  t->load_datalo1();
  t->load_userlocal();
  //printf("Switch %p -> %p (sp %p -> %p)\n", this, t, _kernel_sp, t->_kernel_sp);
  cpu_switchto(this, t, &_kernel_sp, &t->_kernel_sp); 
}

/** Thread context switchin.  Called on every re-activation of a
    thread (switch_exec()).  This method is public only because it is 
    called by assembly routine cpu_switchto via its assembly label
    switchin_context_label.  */


IMPLEMENT
void Context::switchin_context(Context *from)
{
  assert_kdb (this == current());
  assert_kdb (state() & Thread_ready_mask);

  from->handle_lock_holder_preemption();

  // Set kernel SP for saving trap state context when returning via exception to
  // kernel mode (see exception.S and kmem::kernel_sp).
  // regs() + 1 returns a pointer to the end of our kernel stack.
  Kmem::kernel_sp( reinterpret_cast<Mword*>(regs() + 1) );

  //printf("switchin_context errorepc: %p, %p <- %p\n", Kmem::kernel_sp(), this, from);

#ifndef CONFIG_PAGE_SIZE_16KB
#error Cache aliasing with pages < 16K
#endif

  // switch to our page directory if nessecary
  vcpu_aware_space()->switchin_context(from->vcpu_aware_space());
}

PUBLIC inline
bool
Context::vcpu_vzguest_enabled(void) const
{
  Vcpu_state *vcpu = current()->vcpu_state().access();
  return EXPECT_FALSE(state() & Thread_ext_vcpu_enabled)
    && vcpu && (vcpu->state & Vcpu_state::F_user_mode);
}
