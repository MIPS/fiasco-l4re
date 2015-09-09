IMPLEMENTATION [arm && imx21]:

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum {
    WCR  = Kmem::Watchdog_phys_base + 0,
    WCR_SRS = 1 << 4, // Software Reset Signal

    PLL_PCCR1        = Kmem::Pll_phys_base + 0x24,
    PLL_PCCR1_WDT_EN = 1 << 24,
  };

  // WDT CLock Enable
  Io::set<Unsigned32>(PLL_PCCR1_WDT_EN, Kmem::mmio_remap(PLL_PCCR1));

  // Assert Software reset signal by making the bit zero
  Io::mask<Unsigned16>(~WCR_SRS, Kmem::mmio_remap(WCR));

  for (;;)
    ;
}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && (imx35 || imx51 || imx53)]:

void platform_imx_cpus_off()
{}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && imx6]:

void platform_imx_cpus_off()
{
  // switch off core1-3
  Io::clear<Mword>(7 << 22, Kmem::mmio_remap(Mem_layout::Src_phys_base) + 0);
}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && (imx35 || imx51 || imx53 || imx6)]:

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum {
    WCR  = Mem_layout::Watchdog_phys_base + 0,
    WCR_SRS = 1 << 4, // Software Reset Signal
  };

  platform_imx_cpus_off();

  // Assert Software reset signal by making the bit zero
  Io::mask<Unsigned16>(~WCR_SRS, Kmem::mmio_remap(WCR));

  for (;;)
    ;
}
