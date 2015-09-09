/*
 * Derived in part from linux arch/mips/kernel/irq-gic.c
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2008 Ralf Baechle (ralf@linux-mips.org)
 * Copyright (C) 2012 MIPS Technologies, Inc.  All rights reserved.
 */

INTERFACE [mips32 && pic_gic]:

#include "types.h"
#include "irq_chip_generic.h"
#include "linux_asm_gic.h"
#include "bitmap_linux.h"
#include "context_base.h"

#define DECLARE_BITMAP(name,bits) Bitmap_linux<bits> name;
#define DEFINE_BITMAP(name,bits) Bitmap_linux<bits> name;

class Gic : public Irq_chip_gen
{
public:
  enum Ipi
  {
    Global_request = 0,
    Request,
    Debug,
    Ipi_last,
  };

  int set_mode(Mword, Mode) { return 0; }
  bool is_edge_triggered(Mword) const { return true; }

private:
  Address GIC_BASE_ADDR;
  Address GIC_ADDRSPACE_SZ;
  const unsigned int MIPS_GIC_IRQ_BASE;
  int _numintrs;
  int nr_cpu_ids;

  int gic_request_int_base;
  int gic_globalrequest_int_base;
  int gic_debug_int_base;

  struct gic_intr_map *_intr_map;

  static Address _gic_base;
  static unsigned int gic_irq_base;

};

//-------------------------------------------------------------------
IMPLEMENTATION [mips32 && pic_gic]:

#include <cassert>
#include <cstring>
#include <cstdio>

#include "irq_chip_generic.h"
#include "processor.h"
#include "mem_layout.h"

#include "cpu.h"
#include "linux_bitops.h"
#include "mipsregs.h"
#include "warn.h"

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

Address Gic::_gic_base;
unsigned int Gic::gic_irq_base;

struct gic_pcpu_mask {
        DECLARE_BITMAP(pcpu_mask, Gic::GIC_NUM_INTRS);
};

struct gic_pending_regs {
        DECLARE_BITMAP(pending, Gic::GIC_NUM_INTRS);
};

struct gic_intrmask_regs {
        DECLARE_BITMAP(intrmask, Gic::GIC_NUM_INTRS);
};

static struct gic_pcpu_mask pcpu_masks[Gic::NR_CPUS];
static struct gic_pending_regs pending_regs[Gic::NR_CPUS];
static struct gic_intrmask_regs intrmask_regs[Gic::NR_CPUS];

/* linux to fiasco adaptor methods */

Unsigned32 smp_processor_id()
{
  Unsigned32 cpu_val = Cpu_number::val(current_cpu());
  assert(cpu_val < Gic::NR_CPUS);
  return cpu_val;
}

int bitmap_and(unsigned long *dst, const unsigned long *src1,
               const unsigned long *src2, unsigned long nbits)
{
  return Bitmap_linux<Gic::GIC_NUM_INTRS>::bitmap_and(dst, src1, src2, nbits);
}

void bitmap_fill(unsigned long *dst, int nbits)
{
  Bitmap_linux<Gic::GIC_NUM_INTRS>::bitmap_fill(dst, nbits);
}

void set_bit(int nr, Bitmap_linux<Gic::GIC_NUM_INTRS> &map)
{
  map.atomic_set_bit(nr);
}

void clear_bit(int nr, Bitmap_linux<Gic::GIC_NUM_INTRS> &map)
{
  map.atomic_clear_bit(nr);
}

PUBLIC inline
void Gic::gic_send_ipi(unsigned int intr)
{
#ifdef LINUX_GIC_FOR_FIASCO
  assert(intr >= gic_irq_base);
  GICWRITE(GIC_REG(SHARED, GIC_SH_WEDGE), 0x80000000 | (intr - gic_irq_base));
#else
  GICWRITE(GIC_REG(SHARED, GIC_SH_WEDGE), 0x80000000 | intr);
#endif
}

PUBLIC
void Gic::gic_get_int_mask(Bitmap_linux<Gic::GIC_NUM_INTRS> &dst, const Bitmap_linux<Gic::GIC_NUM_INTRS> &src)
{
  gic_get_int_mask(reinterpret_cast<unsigned long *>(&dst),
                   reinterpret_cast<const unsigned long *>(&src));
}

PRIVATE
void Gic::gic_get_int_mask(unsigned long *dst, const unsigned long *src)
{
	unsigned int i;
	unsigned long *pending, *intrmask, *pcpu_mask;
	unsigned long *pending_abs, *intrmask_abs;

	/* Get per-cpu bitmaps */
	pending = pending_regs[smp_processor_id()].pending;
	intrmask = intrmask_regs[smp_processor_id()].intrmask;
	pcpu_mask = pcpu_masks[smp_processor_id()].pcpu_mask;

	pending_abs = (unsigned long *) GIC_REG_ABS_ADDR(SHARED,
							 GIC_SH_PEND_31_0_OFS);
	intrmask_abs = (unsigned long *) GIC_REG_ABS_ADDR(SHARED,
							  GIC_SH_MASK_31_0_OFS);

	for (i = 0; i < BITS_TO_LONGS(GIC_NUM_INTRS); i++) {
		GICREAD(*pending_abs, pending[i]);
		GICREAD(*intrmask_abs, intrmask[i]);
		pending_abs++;
		intrmask_abs++;
	}

	bitmap_and(pending, pending, intrmask, GIC_NUM_INTRS);
	bitmap_and(pending, pending, pcpu_mask, GIC_NUM_INTRS);
	bitmap_and(dst, src, pending, GIC_NUM_INTRS);
}

PUBLIC
unsigned int Gic::gic_get_int(void)
{
	DECLARE_BITMAP(interrupts, GIC_NUM_INTRS);

	bitmap_fill(interrupts, GIC_NUM_INTRS);
	gic_get_int_mask(interrupts, interrupts);

	return find_first_bit(interrupts, GIC_NUM_INTRS);
}

PRIVATE
void
Gic::cpu_init(Cpu_number)
{
  /* enable ipi interrupts on this cpu */
  write_c0_status(read_c0_status() | Cp0_Status::ST_PLATFORM_IPI_INTS);
}

PUBLIC
void
Gic::init_ap(Cpu_number cpu)
{
  cpu_init(cpu);
}

PRIVATE
void Gic::gic_setup_intr(unsigned int intr, unsigned int cpu,
	unsigned int pin, unsigned int polarity, unsigned int trigtype,
	unsigned int flags)
{
	struct gic_shared_intr_map *map_ptr;

	/* Setup Intr to Pin mapping */
	if (pin & GIC_MAP_TO_NMI_MSK) {
		int i;

		GICWRITE(GIC_REG_ADDR(SHARED, GIC_SH_MAP_TO_PIN(intr)), pin);
		/* FIXME: hack to route NMI to all cpu's */
		for (i = 0; i < NR_CPUS; i += 32) {
			GICWRITE(GIC_REG_ADDR(SHARED,
					  GIC_SH_MAP_TO_VPE_REG_OFF(intr, i)),
				 0xffffffff);
		}
	} else {
		GICWRITE(GIC_REG_ADDR(SHARED, GIC_SH_MAP_TO_PIN(intr)),
			 GIC_MAP_TO_PIN_MSK | pin);
		/* Setup Intr to CPU mapping */
		GIC_SH_MAP_TO_VPE_SMASK(intr, cpu);
		if (cpu_has_veic) {
#ifdef LINUX_GIC_FOR_FIASCO
			assert(!cpu_has_veic); /* not supported */
			(void)map_ptr;
#else
			set_vi_handler(pin + GIC_PIN_TO_VEC_OFFSET,
				gic_eic_irq_dispatch);
			map_ptr = &gic_shared_intr_map[pin + GIC_PIN_TO_VEC_OFFSET];
			if (map_ptr->num_shared_intr >= GIC_MAX_SHARED_INTR)
				BUG();
			map_ptr->intr_list[map_ptr->num_shared_intr++] = intr;
#endif
		}
	}

	/* Setup Intr Polarity */
	GIC_SET_POLARITY(intr, polarity);

	/* Setup Intr Trigger Type */
	GIC_SET_TRIGGER(intr, trigtype);

	/* Init Intr Masks */
	GIC_CLR_INTR_MASK(intr);

	/* Initialise per-cpu Interrupt software masks */
	set_bit(intr, pcpu_masks[cpu].pcpu_mask);

	if ((flags & GIC_FLAG_TRANSPARENT) && (cpu_has_veic == 0))
		GIC_SET_INTR_MASK(intr);
	if (trigtype == GIC_TRIG_EDGE)
		gic_irq_flags[intr] |= GIC_TRIG_EDGE;
}

PRIVATE
void Gic::gic_basic_init(int numintrs, int numvpes,
                         struct gic_intr_map *intrmap, int mapsize)
{
	int i, cpu;
	unsigned int pin_offset = 0;

#ifndef LINUX_GIC_FOR_FIASCO
	board_bind_eic_interrupt = &gic_bind_eic_interrupt;
#endif

	/* Setup defaults */
	for (i = 0; i < numintrs; i++) {
		GIC_SET_POLARITY(i, GIC_POL_POS);
		GIC_SET_TRIGGER(i, GIC_TRIG_LEVEL);
		GIC_CLR_INTR_MASK(i);
		if (i < GIC_NUM_INTRS) {
			gic_irq_flags[i] = 0;
			gic_shared_intr_map[i].num_shared_intr = 0;
			gic_shared_intr_map[i].local_intr_mask = 0;
		}
	}

	/*
	 * In EIC mode, the HW_INT# is offset by (2-1). Need to subtract
	 * one because the GIC will add one (since 0=no intr).
	 */
	if (cpu_has_veic)
		pin_offset = (GIC_CPU_TO_VEC_OFFSET - GIC_PIN_TO_VEC_OFFSET);

	/* Setup specifics */
	for (i = 0; i < mapsize; i++) {
		cpu = intrmap[i].cpunum;
		if (cpu == GIC_UNUSED)
			continue;
		gic_setup_intr(i,
			intrmap[i].cpunum,
			intrmap[i].pin + pin_offset,
			intrmap[i].polarity,
			intrmap[i].trigtype,
			intrmap[i].flags);
	}

#ifdef LINUX_GIC_FOR_FIASCO
	// skip setting up GIC timer interrupt
	(void)numvpes;
#else
	vpe_local_setup(numvpes);
#endif
}

PRIVATE
Address
Gic::ioremap_nocache(unsigned long addr, unsigned long size)
{
  return Mem_layout::ioremap_nocache((Address)addr, (Address)size);
}

PRIVATE
void Gic::gic_init(unsigned long gic_base_addr,
		     unsigned long gic_addrspace_size,
		     struct gic_intr_map *intr_map, unsigned int intr_map_size,
		     unsigned int irqbase)
{
	unsigned int gicconfig;
	int numvpes, numintrs;

	_gic_base = (unsigned long) ioremap_nocache(gic_base_addr,
						    gic_addrspace_size);
	gic_irq_base = irqbase;

	GICREAD(GIC_REG(SHARED, GIC_SH_CONFIG), gicconfig);
	numintrs = (gicconfig & GIC_SH_CONFIG_NUMINTRS_MSK) >>
		   GIC_SH_CONFIG_NUMINTRS_SHF;
	numintrs = ((numintrs + 1) * 8);

	numvpes = (gicconfig & GIC_SH_CONFIG_NUMVPES_MSK) >>
		  GIC_SH_CONFIG_NUMVPES_SHF;
	numvpes = numvpes + 1;

	gic_basic_init(numintrs, numvpes, intr_map, intr_map_size);

#ifdef LINUX_GIC_FOR_FIASCO
	_numintrs = numintrs;
        if (GIC_NUM_INTRS > _numintrs) {
          panic("Number of IRQs configured %d exceeds available GIC IRQs %d."
                " Try configuring fewer cores (CONFIG_MP_MAX_CPUS).\n",
              GIC_NUM_INTRS, _numintrs);
        } else
          WARNX(Info, "Number of IRQs available at this GIC: %d\n", _numintrs);

        struct irq_chip gic_irq_controller;
#endif
	gic_platform_init(numintrs, &gic_irq_controller);
}

PUBLIC
void Gic::init()
{
#ifndef LINUX_GIC_FOR_FIASCO
  gic_call_int_base = GIC_NUM_INTRS -
          (NR_CPUS - nr_cpu_ids) * 2 - nr_cpu_ids;
  gic_resched_int_base = gic_call_int_base - nr_cpu_ids;
#endif
  fill_ipi_map();

  gic_init(GIC_BASE_ADDR, GIC_ADDRSPACE_SZ, gic_intr_map,
                  ARRAY_SIZE(gic_intr_map), MIPS_GIC_IRQ_BASE);

  cpu_init(Cpu_number::boot_cpu());
}

PUBLIC explicit
Gic::Gic(Address gic_base_addr, Address gic_addrspace_sz, unsigned int gic_irq_base)
  : GIC_BASE_ADDR(gic_base_addr),
  GIC_ADDRSPACE_SZ(gic_addrspace_sz),
  MIPS_GIC_IRQ_BASE(gic_irq_base),
  _numintrs(0),
  nr_cpu_ids(Config::Max_num_cpus)
{
  init();

  Irq_chip_gen::init(_numintrs);
}

PUBLIC
void
Gic::mask(Mword pin)
{
  assert (cpu_lock.test());
  disable_locked(pin);
}

PUBLIC
void
Gic::mask_and_ack(Mword pin)
{
  assert (cpu_lock.test());
  disable_locked(pin);
  acknowledge_locked(pin);
}

PUBLIC
void
Gic::ack(Mword pin)
{
  acknowledge_locked(pin);
}

PUBLIC
void
Gic::eoi(Mword pin)
{
  acknowledge_locked(pin);
  enable_locked(pin);
}

PUBLIC
void
Gic::unmask(Mword pin)
{
  assert (cpu_lock.test());
  enable_locked(pin);
}

//---------------------------------------------------------------------------
INTERFACE [mips32 && pic_gic && mp]:

#include "spin_lock.h"

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32 && pic_gic && mp]:

EXTENSION class Gic
{
private:
  static Spin_lock<> _gic_lock;
};

Spin_lock<> Gic::_gic_lock(Spin_lock<>::Unlocked);

PUBLIC
void
Gic::set_cpu(Mword pin, Cpu_number cpu)
{
  auto guard = lock_guard(_gic_lock);

  int irq = (pin - gic_irq_base);
  Unsigned32 cpu_val = Cpu_number::val(cpu);

  /* Re-route this IRQ */
  GIC_SH_MAP_TO_VPE_SMASK(irq, cpu_val);

  /* Update the pcpu_masks */
  for (int i = 0; i < NR_CPUS; i++)
    clear_bit(irq, pcpu_masks[i].pcpu_mask);

  set_bit(irq, pcpu_masks[cpu_val].pcpu_mask);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32 && pic_gic && !mp]:

PUBLIC inline
void
Gic::set_cpu(Mword, Cpu_number)
{}

//---------------------------------------------------------------------------
IMPLEMENTATION [mips32 && pic_gic && debug]:

PUBLIC
char const *
Gic::chip_type() const
{ return "GIC"; }
