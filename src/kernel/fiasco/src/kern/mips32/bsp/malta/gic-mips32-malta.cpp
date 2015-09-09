/*
 * Derived in part from linux arch/mips/mti-malta/malta-int.c
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
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000, 2001, 2004 MIPS Technologies, Inc.
 * Copyright (C) 2001 Ralf Baechle
 * Copyright (C) 2013 Imagination Technologies Ltd.
 *
 * Routines for generic manipulation of the interrupts found on the MIPS
 * Malta board. The interrupt controller is located in the South Bridge
 * a PIIX4 device with two internal 82C95 interrupt controllers.
 */

INTERFACE [mips32 && pic_gic && malta]:

#include "config.h"

EXTENSION class Gic
{
public:
  enum {
    NR_CPUS = Config::Max_num_cpus,
#ifdef LINUX_GIC_FOR_FIASCO
    /* Break with Linux malta driver convention to allow support for 8 cores
     * with 3 IPI pins each. Extend into the unused irq slots reserved for cp0
     * irqs routed to the GIC. Leave the first 16 peripheral irqs slots
     * unchanged.
     */
    GIC_NUM_INTRS = (16 + NR_CPUS * Gic::Ipi_last),
#else
    GIC_NUM_INTRS = (24 + NR_CPUS * 2),
#endif
  };
};

//-------------------------------------------------------------------
IMPLEMENTATION [mips32 && pic_gic && malta]:

#include "bitmap_linux.h"
#include "assert.h"
#include "find_next_bit.h"

EXTENSION class Gic
{
private:
  static struct gic_intr_map gic_intr_map[GIC_NUM_INTRS];

  unsigned int gic_irq_flags[GIC_NUM_INTRS];

  /* The index into this array is the vector # of the interrupt. */
  struct gic_shared_intr_map gic_shared_intr_map[GIC_NUM_INTRS];

  static unsigned int ipi_map[NR_CPUS];
  static DECLARE_BITMAP(ipi_ints, GIC_NUM_INTRS);
};

unsigned int Gic::ipi_map[Gic::NR_CPUS];
DEFINE_BITMAP(Gic::ipi_ints, Gic::GIC_NUM_INTRS);

PUBLIC
bool Gic::ipi_mapped(unsigned ipi_irq)
{
  /* check if ipi is mapped to this cpu */
  return ((1 << ipi_irq) & ipi_map[smp_processor_id()]);
}

PUBLIC
const Bitmap_linux<Gic::GIC_NUM_INTRS> &
Gic::get_ipi_ints(void)
{
  return ipi_ints;
}

#define GIC_GLOBALREQUEST_INT(cpu) (gic_globalrequest_int_base+(cpu))
#define GIC_REQUEST_INT(cpu) (gic_request_int_base+(cpu))
#define GIC_DEBUG_INT(cpu) (gic_debug_int_base+(cpu))

PUBLIC
unsigned int Gic::plat_ipi_request_int_xlate(unsigned int cpu)
{
	return GIC_REQUEST_INT(cpu);
}

PUBLIC
unsigned int Gic::plat_ipi_globalrequest_int_xlate(unsigned int cpu)
{
	return GIC_GLOBALREQUEST_INT(cpu);
}

PUBLIC
unsigned int Gic::plat_ipi_debug_int_xlate(unsigned int cpu)
{
	return GIC_DEBUG_INT(cpu);
}

/*
 * This GIC specific tabular array defines the association between External
 * Interrupts and CPUs/Core Interrupts. The nature of the External
 * Interrupts is also defined here - polarity/trigger.
 */

#define GIC_CPU_NMI GIC_MAP_TO_NMI_MSK
#define X GIC_UNUSED

struct gic_intr_map Gic::gic_intr_map[GIC_NUM_INTRS] = {
	{ X, X,		   X,		X,		0 },
	{ X, X,		   X,		X,		0 },
	{ X, X,		   X,		X,		0 },
	{ 0, GIC_CPU_INT0, GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ 0, GIC_CPU_INT1, GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ 0, GIC_CPU_INT2, GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ 0, GIC_CPU_INT3, GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ 0, GIC_CPU_INT4, GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ 0, GIC_CPU_INT3, GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ 0, GIC_CPU_INT3, GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ X, X,		   X,		X,		0 },
	{ X, X,		   X,		X,		0 },
	{ 0, GIC_CPU_INT3, GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ 0, GIC_CPU_NMI,  GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ 0, GIC_CPU_NMI,  GIC_POL_POS, GIC_TRIG_LEVEL, GIC_FLAG_TRANSPARENT },
	{ X, X,		   X,		X,		0 },
	/* The remainder of this table is initialised by fill_ipi_map */
};
#undef X

static
void bitmap_set(Bitmap_linux<Gic::GIC_NUM_INTRS> &map, unsigned int start, int len)
{
  (void)len;
  assert(len == 1);
  map.set_bit(start);
}

PRIVATE
void Gic::fill_ipi_map1(int baseintr, int cpu, int cpupin)
{
	int intr = baseintr + cpu;
	gic_intr_map[intr].cpunum = cpu;
	gic_intr_map[intr].pin = cpupin;
	gic_intr_map[intr].polarity = GIC_POL_POS;
	gic_intr_map[intr].trigtype = GIC_TRIG_EDGE;
	gic_intr_map[intr].flags = 0;
	ipi_map[cpu] |= (1 << (cpupin + 2));
	bitmap_set(ipi_ints, intr, 1);
}

PRIVATE
void Gic::fill_ipi_map(void)
{
	int cpu;

#ifdef LINUX_GIC_FOR_FIASCO
	unsigned int int_base_end_limit = GIC_NUM_INTRS -
		(NR_CPUS - nr_cpu_ids) * Gic::Ipi_last;
	gic_debug_int_base = int_base_end_limit - nr_cpu_ids;
	gic_request_int_base = gic_debug_int_base - nr_cpu_ids;
	gic_globalrequest_int_base = gic_request_int_base - nr_cpu_ids;
#endif

	for (cpu = 0; cpu < nr_cpu_ids; cpu++) {
#ifdef LINUX_GIC_FOR_FIASCO
		fill_ipi_map1(gic_globalrequest_int_base, cpu, GIC_CPU_INT1);
		fill_ipi_map1(gic_request_int_base, cpu, GIC_CPU_INT2);
		fill_ipi_map1(gic_debug_int_base, cpu, GIC_CPU_INT3);
#else
		fill_ipi_map1(gic_resched_int_base, cpu, GIC_CPU_INT1);
		fill_ipi_map1(gic_call_int_base, cpu, GIC_CPU_INT2);
#endif
	}
}

PUBLIC inline
void Gic::disable_locked(unsigned irq)
{
	GIC_CLR_INTR_MASK(irq - gic_irq_base);
}

PUBLIC inline
void Gic::enable_locked(unsigned irq)
{
	GIC_SET_INTR_MASK(irq - gic_irq_base);
}

PUBLIC inline
void Gic::acknowledge_locked(unsigned irq)
{
	int gic_irq = (irq - gic_irq_base);

	GIC_CLR_INTR_MASK(gic_irq);

	if (gic_irq_flags[gic_irq] & GIC_TRIG_EDGE)
		GICWRITE(GIC_REG(SHARED, GIC_SH_WEDGE), gic_irq);
}

void Gic::gic_platform_init(int irqs, struct irq_chip *irq_controller)
{
#ifdef LINUX_GIC_FOR_FIASCO
	(void)irqs; (void)irq_controller;
	// skip setting GIC chip as handler for gic irqs, we are using
        // Irq_malta_chip instead.
#else
	int i;

	for (i = gic_irq_base; i < (gic_irq_base + irqs); i++)
		irq_set_chip(i, irq_controller);
#endif
}
