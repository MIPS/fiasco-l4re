IMPLEMENTATION [mips32]:

#include <cstdio>
#include <cstring>

#include "config.h"
#include "globals.h"
#include "space.h"
#include "mipsregs.h"
#include "mipsvzregs.h"
#include "cpu.h"

class Jdb_kern_info_cpu : public Jdb_kern_info_module
{
private:
  static Mword jdb_mfc0_insn(unsigned r_val) asm ("jdb_mfc0_insn");
};

static Jdb_kern_info_cpu k_c INIT_PRIORITY(JDB_MODULE_INIT_PRIO+1);

PUBLIC
Jdb_kern_info_cpu::Jdb_kern_info_cpu()
  : Jdb_kern_info_module('c', "CPU features")
{
  Jdb_kern_info::register_subcmd(this);
}

#define DUMP_DESCR_FMT "%-16s"
#define DUMP_HEX(_descr, _val) \
  do { printf(DUMP_DESCR_FMT ": %08lx\n", (_descr), (_val)); } while(0)
#define DUMP_INT(_descr, _val) \
  do { printf(DUMP_DESCR_FMT ": %ld\n", (_descr), (_val)); } while(0)
#define DUMP_CP0(_descr, _read_c0_reg, _val) \
  do { \
    _val = _read_c0_reg; \
    DUMP_HEX(_descr, _val); \
  } while (0)

PUBLIC
void
Jdb_kern_info_cpu::dump_cp0_regs()
{
  Mword val;

  DUMP_CP0("EBase", read_c0_ebase(), val);
  DUMP_INT("Ebase.CPUNum", (val & 0x3ff));
  DUMP_CP0("EntryHi", read_c0_entryhi(), val);
  DUMP_HEX("EntryHi.ASID", (val & 0xff));
  DUMP_CP0("EPC", read_c0_epc(), val);
  DUMP_CP0("Status", read_c0_status(), val);
  DUMP_CP0("Cause", read_c0_cause(), val);
  DUMP_CP0("PRId", read_c0_prid(), val);
  DUMP_CP0("HWREna", read_c0_hwrena(), val);
  DUMP_CP0("Config", read_c0_config(), val);
  if (val & MIPS_CONF_M) {
    DUMP_CP0("Config1", read_c0_config1(), val);
    if (val & MIPS_CONF_M) {
      DUMP_CP0("Config2", read_c0_config2(), val);
      if (val & MIPS_CONF_M) {
        DUMP_CP0("Config3", read_c0_config3(), val);
        if (val & MIPS_CONF3_ULRI)
          DUMP_CP0("UserLocal", read_c0_userlocal(), val);
      }
    }
  }

  if (cpu_has_vz)
    DUMP_CP0("GuestCtl0", read_c0_guestctl0(), val);
  if (cpu_has_guestctl0ext)
    DUMP_CP0("GuestCtl0Ext", read_c0_guestctl0ext(), val);
  if (cpu_has_vz)
    DUMP_CP0("GTOffset", read_c0_gtoffset(), val);
  if (cpu_has_guestctl1) {
    DUMP_CP0("GuestCtl1", read_c0_guestctl1(), val);
    DUMP_HEX("GuestCtl1.ID", (val & GUESTCTL1_ID));
  }
  if (cpu_has_guestctl2) {
    DUMP_CP0("GuestCtl2", read_c0_guestctl2(), val);
    DUMP_HEX("GuestCtl2.VIP", (val & GUESTCTL2_VIP));
  }
}

PUBLIC
void
Jdb_kern_info_cpu::show()
{
  dump_cp0_regs();
}
