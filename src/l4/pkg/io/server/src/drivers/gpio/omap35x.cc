#include <l4/cxx/unique_ptr>

#include "main.h"
#include "irqs.h"
#include "debug.h"
#include "gpio"
#include "hw_device.h"
#include "hw_mmio_register_block.h"
#include "hw_irqs.h"
#include "server.h"

namespace {

enum Regs : unsigned
{
  Irq_status1       = 0x018,
  Output_enable     = 0x034,
  Level_detect_low  = 0x040,
  Level_detect_high = 0x044,
  Rising_detect     = 0x048,
  Falling_detect    = 0x04c,
  Clr_irq_enable1   = 0x060,
  Set_irq_enable1   = 0x064,
};

enum : unsigned { Inval = 0xbfffffff, };

// the values in this table are taken from the OMAP35x TRM
// chapter 7 - System Control Module, table 7-4 (pad configuration
// register fields)
static const unsigned scm_offsets[6][32] =
{
  { 0x1e0, 0xa06, 0xa08, 0xa0c, 0xa0e, 0xa10, 0xa12, 0xa14,
    0xa16, 0xa18, 0xa1a, 0x424, 0x5d8, 0x5da, 0x5dc, 0x5de,
    0x5e0, 0x5e2, 0x5e4, 0x5e6, 0x5e8, 0x5ea, 0x5ec, 0x5ee,
    0x5f0, 0x5f2, 0x5f4, 0x5f6, 0x5f8, 0x5fa, 0xa08, 0xa26 },
  { 0x23a, Inval, 0x07a, 0x07c, 0x07e, 0x080, 0x082, 0x084,
    0x086, 0x088, 0x08a, 0x08c, 0x09e, 0x0a0, 0x0a2, 0x0a4,
    0x0a6, 0x0a8, 0x0aa, 0x0ac, 0x0b0, 0x0b2, 0x0b4, 0x0b6,
    0x0b8, 0x0ba, 0x0bc, 0x0be, 0x0c6, 0x0c8, 0x0ca, 0x0ce },
  { 0x0d0, 0x0d2, 0x0d4, 0x0d6, 0x0d8, 0x0da, 0x0dc, 0x0de,
    0x0e0, 0x0e2, 0x0e4, 0x0e6, 0x0e8, 0x0ea, 0x0ac, 0x0ee,
    0x0f0, 0x0f2, 0x0f4, 0x0f6, 0x0f8, 0x0fa, 0x0fc, 0x0fe,
    0x100, 0x102, 0x104, 0x106, 0x108, 0x10a, 0x10c, 0x10e },
  { 0x110, 0x112, 0x114, 0x116, 0x118, 0x11a, 0x11c, 0x11e,
    0x120, 0x122, 0x124, 0x126, 0x128, 0x12a, 0x12c, 0x12e,
    0x134, 0x136, 0x138, 0x13a, 0x13c, 0x13e, 0x140, 0x142,
    0x144, 0x146, 0x148, 0x14a, 0x14c, 0x14e, 0x150, 0x152 },
  { 0x154, 0x156, 0x158, 0x15a, 0x15c, 0x15e, 0x160, 0x162,
    0x164, 0x166, 0x168, 0x16a, 0x16c, 0x16e, 0x170, 0x172,
    0x174, 0x176, 0x178, 0x17a, 0x17c, 0x17e, 0x180, 0x182,
    0x184, 0x186, 0x188, 0x18a, 0x18c, 0x18e, 0x190, 0x192 },
  { 0x194, 0x196, 0x198, 0x19a, 0x19c, 0x19e, 0x1a0, 0x130,
    0x1be, 0x1b0, 0x1c6, 0x1c8, 0x1ca, 0x1cc, 0x1ce, 0x1d0,
    0x1d2, 0x1d4, 0x1d6, 0x1d8, 0x1da, 0x1dc, 0x1de, 0x1c0,
    0x1c2, 0x1c4, 0x1e2, 0x238, 0x1b0, 0x1b4, 0x1b6, 0x1b8 }
};

template<typename T> static
int
getval(const char *s, cxx::String const &prop, Hw::Device::Prop_val const &val,
       T *rval)
{
  if (prop == s)
    {
      if (val.type != Hw::Device::Prop_val::Int)
        return -Hw::Device::E_inval;

      *rval = val.val.integer;
      return Hw::Device::E_ok;
    }
  return -Hw::Device::E_no_prop;
}

class Scm_omap35x : public Hw::Device
{
  unsigned _offset;
  Hw::Register_block<16> _regs;

public:
  Scm_omap35x() : _offset(0)
  {
    add_cid("scm-omap");
    add_cid("scm-omap35x");
  }

  void init();

  void set_mode(unsigned offset, unsigned func)
  {
    if (offset == Inval)
      throw -L4_EINVAL;

    _regs[offset].modify(0x7, func & 0x7);
  }

  void set_pull(unsigned offset, unsigned value)
  {
    if (offset == Inval)
      throw -L4_EINVAL;

    // bits 3 (enable) and 4 (type) are for pull mode
    // also enable bidirectional mode in bit 8
    _regs[offset].modify(0x118, ((value & 0x3) << 3) | 0x100);
  }
};

void
Scm_omap35x::init()
{
  Hw::Device::init();

  Resource *regs = resources()->find("regs");
  if (!regs || regs->type() != Resource::Mmio_res)
    {
      d_printf(DBG_ERR, "error: %s: no base address set for device: Scm_omap35x\n"
                        "       missing or wrong 'regs' resource\n"
                        "       the SCM will not work at all!\n", name());
      return;
    }

  l4_addr_t phys_base = regs->start();
  l4_addr_t size = regs->size();

  if (size < 0x100 || size > (1 << 12))
    {
      d_printf(DBG_ERR, "error: %s: invalid mmio size (%lx) for device: Scm_omap35x\n"
                        "       the chip will not work at all!\n", name(), size);
      return;
    }

  l4_addr_t vbase = res_map_iomem(phys_base, size);
  if (!vbase)
    {
      d_printf(DBG_ERR, "error: %s: cannot map registers for Scm_omap35x\n"
                        "       phys=%lx-%lx", name(),
                        phys_base, phys_base + size - 1);
      return;
    }

  d_printf(DBG_DEBUG2, "%s: Scm_omap35x: mapped registers to %08lx\n",
           name(), vbase);

  _regs = new Hw::Mmio_register_block<16>(vbase);
}

static Hw::Device_factory_t<Scm_omap35x> __hw_scm_factory("Scm_omap35x");

class Gpio_irq_server;

class Gpio_irq_pin : public Io_irq_pin
{
  Gpio_irq_server *const _svr;
  unsigned const _pin;
  unsigned _mode;
  unsigned _enabled;

public:
  Gpio_irq_pin(unsigned pin, Gpio_irq_server *svr)
  : _svr(svr), _pin(pin), _mode(L4_IRQ_F_NONE), _enabled(0)
  { }

  int bind(L4::Cap<L4::Irq> irq, unsigned mode);
  int unbind();
  int set_mode(unsigned mode);
  int mask();
  int unmask();
  int clear();

  unsigned mode() const { return _mode; }
  unsigned pin() const { return _pin; }

  void trigger() { irq()->trigger(); }
};

class Gpio_irq_server : public L4::Server_object
{
  friend class Gpio_irq_pin;
  L4::Cap<L4::Irq> _irq;
  int _irq_num;

  Hw::Register_block<32> _regs;

  cxx::unique_ptr<cxx::unique_ptr<Gpio_irq_pin>[]> _pins;

  void write_reg_pin(unsigned reg, unsigned pin, int value)
  {
    if (value & 1)
      _regs[reg].set(1 << pin);
    else
      _regs[reg].clear(1 << pin);
  }


public:
  Gpio_irq_server(int irq, unsigned pins, l4_addr_t base);

  int dispatch(unsigned long, L4::Ipc::Iostream &);

  Gpio_irq_pin *register_pin(unsigned pin)
  {
    cxx::unique_ptr<Gpio_irq_pin> p = cxx::make_unique<Gpio_irq_pin>(pin, this);
    _pins[pin] = cxx::move(p);
    return _pins[pin].get();
  }

  void un_register_pin(Gpio_irq_pin *pin)
  { _pins[pin->pin()] = NULL; }

  void mask(Gpio_irq_pin *pin)
  {
    _regs[Clr_irq_enable1] = 1 << pin->pin();
  }

  void unmask(Gpio_irq_pin *pin)
  {
    _regs[Set_irq_enable1] = 1 << pin->pin();
  }

  void set_mode(Gpio_irq_pin *pin, unsigned mode)
  {
    int values[4] = {0, 0, 0, 0};
    switch(mode)
      {
    case L4_IRQ_F_NONE:
      _regs[Clr_irq_enable1] = 1 << pin->pin();
      break;
    case L4_IRQ_F_LEVEL_HIGH:
      values[3] = 1;
      break;
    case L4_IRQ_F_LEVEL_LOW:
      values[2] = 1;
      break;
    case L4_IRQ_F_POS_EDGE:
      values[0] = 1;
      break;
    case L4_IRQ_F_NEG_EDGE:
      values[1] = 1;
      break;
    case L4_IRQ_F_BOTH_EDGE:
      values[0] = 1;
      values[1] = 1;
      break;
      }

    _regs[Output_enable].set(1 << pin->pin());

    write_reg_pin(Rising_detect, pin->pin(), values[0]);
    write_reg_pin(Falling_detect, pin->pin(), values[1]);
    write_reg_pin(Level_detect_low, pin->pin(), values[2]);
    write_reg_pin(Level_detect_high, pin->pin(), values[3]);
  }
};

class Gpio_omap35x_chip : public Hw::Gpio_device
{
private:
  Hw::Register_block<32> _regs;
  unsigned _nr_pins;
  const unsigned *_scm_offset;
  Gpio_irq_server *_irq_svr;
  Scm_omap35x *_scm;


  enum Regs : unsigned
  {
    Revision          = 0x000,
    Sysconfig         = 0x010,
    Sysstatus         = 0x014,
    Irq_enable1       = 0x01c,
    Wakeup_enable     = 0x020,
    Irq_status2       = 0x028,
    Irq_enable2       = 0x02c,
    Ctrl              = 0x030,
    Data_in           = 0x038,
    Data_out          = 0x03c,
    Debounce_enable   = 0x050,
    Debouncing_time   = 0x054,
    Clr_irq_enable2   = 0x070,
    Set_irq_enable2   = 0x074,
    Clr_wku_enable    = 0x080,
    Set_wku_enable    = 0x084,
    Clr_data_out      = 0x090,
    Set_data_out      = 0x094,
  };

  enum Func : unsigned
  {
    Func_out = 0,
    Func_in  = 1,
  };

  enum Pad_mode : unsigned
  {
    Mode_gpio = 4,
    Mode_safe = 7,
  };

  static l4_uint32_t _pin_bit(unsigned pin)
  { return 1 << (pin & 31); }

  static unsigned _pin_shift(unsigned pin)
  { return pin % 32; }

  unsigned _reg_offset_check(unsigned pin_offset) const
  {
    switch (pin_offset)
      {
      case 0:
        return 0;

      default:
        throw -L4_EINVAL;
      }
  }

  void config(unsigned pin, unsigned func)
  {
    if (_scm)
      _scm->set_mode(_scm_offset[pin], func);
  }

  int set_property(cxx::String const &prop, Prop_val const &val);

public:
  Gpio_omap35x_chip() : _nr_pins(32), _scm_offset(0), _irq_svr(0), _scm(0)
  {
    add_cid("gpio");
    add_cid("gpio-omap35x");
  }

  unsigned nr_pins() const { return _nr_pins; }

  void request(unsigned) {}
  void free(unsigned) {}
  void setup(unsigned pin, unsigned mode, int value = 0);
  int get(unsigned pin);
  void set(unsigned pin, int value);
  void config_pad(unsigned pin, unsigned func, unsigned value);
  void config_get(unsigned pin, unsigned func, unsigned *value);

  void config_pull(unsigned pin, unsigned mode);

  Io_irq_pin *get_irq(unsigned pin);

  void multi_setup(Pin_slice const &mask, unsigned mode, unsigned outvalues = 0);
  void multi_config_pad(Pin_slice const &mask, unsigned func, unsigned value = 0);
  void multi_set(Pin_slice const &mask, unsigned data);
  unsigned multi_get(unsigned offset);

  void init();
};

Gpio_irq_server::Gpio_irq_server(int irq, unsigned pins, l4_addr_t base)
: _irq_num(irq)
{
  _regs = new Hw::Mmio_register_block<32>(base);

  _pins = cxx::make_unique<cxx::unique_ptr<Gpio_irq_pin>[]>(pins);

  if (l4_error(system_icu()->icu->set_mode(_irq_num, L4_IRQ_F_NONE)) < 0)
    {
      d_printf(DBG_ERR, "error: Gpio_irq_server: failed to set hw irq mode.\n");
      return;
    }

  _irq = registry->register_irq_obj(this);

  // FIXME: should test for unmask via ICU (result of bind ==1)
  if (l4_error(system_icu()->icu->bind(_irq_num, _irq)) < 0)
    {
      d_printf(DBG_ERR, "error: Gpio_irq_server: failed to bind hw irq\n");
      return;
    }

  _irq->unmask();
}

int
Gpio_irq_server::dispatch(unsigned long, L4::Ipc::Iostream &)
{
  // I think it is sufficient to read Irq_status1 as we only use the first
  // hw irq per chip
  unsigned status = _regs[Irq_status1];
  unsigned reset = status;

  if (!status) // usually never happens
    {
      _irq->unmask();
      return -L4_ENOREPLY;
    }

  l4_uint32_t mask_irqs = 0, pin_bit = 1;
  for (unsigned pin = 0; status; ++pin, status >>= 1, pin_bit <<= 1)
    if ((status & pin) && _pins[pin])
      {
        switch (_pins[pin]->mode())
          {
          case L4_IRQ_F_LEVEL_HIGH: mask_irqs |= pin_bit; break;
          case L4_IRQ_F_LEVEL_LOW: mask_irqs |= pin_bit; break;
          }
        _pins[pin]->trigger();
      }

  // do the mask for level triggered IRQs
  if (mask_irqs)
    _regs[Clr_irq_enable1] = mask_irqs;

  _regs[Irq_status1] = reset;
  _irq->unmask();

  return -L4_ENOREPLY;
}

int
Gpio_irq_pin::bind(L4::Cap<L4::Irq> irq, unsigned)
{
  this->irq(irq);
  set_shareable(false);

  if (_mode == L4_IRQ_F_NONE)
    {
      d_printf(DBG_ERR, "error: Gpio_irq_pin: invalid Irq mode.\n"
                        "       Set Irq mode before bind.\n");
      throw -L4_EINVAL;
    }

  return 1;
}

int
Gpio_irq_pin::unbind()
{
  _svr->un_register_pin(this);
  return 0;
}

int
Gpio_irq_pin::set_mode(unsigned mode)
{
  // nothing to do
  if (mode == L4_IRQ_F_NONE || _mode == mode)
    return 0;

  // this operation touches multiple mmio registers and is thus
  // not atomic, that's why we first mask the IRQ and if it was
  // enabled we unmask it after we have changed the mode
  if (_enabled)
    _svr->mask(this);
  _mode = mode;
  _svr->set_mode(this, mode);
  if (_enabled)
    _svr->unmask(this);

  return 0;
}

int
Gpio_irq_pin::mask()
{
  _enabled = 0;
  _svr->mask(this);
  return 0;
}

int
Gpio_irq_pin::unmask()
{
  // make sure client has selected an Irq mode
  // because GPIO irqs don't have a default mode
  if (_mode == L4_IRQ_F_NONE)
    d_printf(DBG_WARN, "warning: Gpio_irq_pin: No Irq mode set.\n"
                       "         You will not receive any Irqs.\n");

  _enabled = 1;
  _svr->unmask(this);
  return 0;
}

int
Gpio_irq_pin::clear()
{
  l4_uint32_t status = _svr->_regs[Irq_status1] & (1UL << _pin);
  if (status)
    _svr->_regs[Irq_status1] = status;

  return Io_irq_pin::clear() + (status >> _pin);
}

int
Gpio_omap35x_chip::get(unsigned pin)
{
  if (pin >= _nr_pins)
    throw -L4_EINVAL;

  return (_regs[Data_out] >> _pin_shift(pin)) & 1;
}

void
Gpio_omap35x_chip::set(unsigned pin, int value)
{
  if (pin >= _nr_pins)
    throw -L4_EINVAL;

  unsigned reg_set = value ? Set_data_out : Clr_data_out;
  _regs[reg_set] = _pin_bit(pin);
}

unsigned
Gpio_omap35x_chip::multi_get(unsigned offset)
{
  _reg_offset_check(offset);
  return _regs[Data_out];
}

void
Gpio_omap35x_chip::multi_set(Pin_slice const &mask, unsigned data)
{
  _reg_offset_check(mask.offset);
  if (mask.mask & data)
    _regs[Set_data_out] = (mask.mask & data);
  if (mask.mask & ~data)
    _regs[Clr_data_out] = (mask.mask & ~data);
}

void
Gpio_omap35x_chip::setup(unsigned pin, unsigned mode, int value)
{
  if (pin >= _nr_pins)
    throw -L4_EINVAL;

  switch (mode)
    {
    case Input:
      _regs[Output_enable].set(_pin_bit(pin));
      _scm->set_mode(_scm_offset[pin], Mode_gpio);
      return;
    case Output:
      _regs[Output_enable].clear(_pin_bit(pin));
      _scm->set_mode(_scm_offset[pin], Mode_gpio);
      set(pin, value);
      return;
    case Irq:
      d_printf(DBG_WARN, "warning: Gpio_omap35x_chip: trying to setup pin as Irq\n"
                         "         This mode is not supported. Setting mode to input\n"
                         "         Use to_irq() to configure Irq\n");
      _regs[Output_enable].set(_pin_bit(pin));
      return;
    default:
      // although setup is part of the generic Gpio API we allow
      // hardware specific modes as well
      mode &= 0x7;
      break;
    }

  config(pin, mode);
}

void
Gpio_omap35x_chip::config_pad(unsigned pin, unsigned reg, unsigned value)
{
  if (pin >= _nr_pins)
    throw -L4_EINVAL;

  switch (reg)
    {
    case Irq_status1:       // hmm, allow user to reset the irq status?
    case Irq_enable1:       // hmm, allow user to enable irq this way?
    case Wakeup_enable:
    case Irq_status2:       // currently the driver only uses IRQ1
    case Irq_enable2:       // currently the driver only uses IRQ1
    case Output_enable:
    case Data_out:
    case Level_detect_low:  // hmm, allow user to configure IRQ this way?
    case Level_detect_high: // hmm, allow user to configure IRQ this way?
    case Rising_detect:     // hmm, allow user to configure IRQ this way?
    case Falling_detect:    // hmm, allow user to configure IRQ this way?
    case Debounce_enable:
    case Clr_irq_enable1:   // hmm, allow user to disable IRQ this way?
    case Set_irq_enable1:   // hmm, allow user to enable IRQ this way?
    case Clr_irq_enable2:   // currently the driver only uses IRQ1
    case Set_irq_enable2:   // currently the driver only uses IRQ1
    case Clr_wku_enable:
    case Set_wku_enable:
    case Clr_data_out:
    case Set_data_out:
      _regs[reg].modify(_pin_bit(pin), value ? _pin_bit(pin) : 0);

    default:
      // cannot allow the following registers, they have security implications
      // Sysconfig, Ctrl, Debouncing_time
      // the following registers are read-only
      // Revision, Sysstatus (also security), Data_in
      throw -L4_EINVAL;
    }
}

void
Gpio_omap35x_chip::config_get(unsigned pin, unsigned reg, unsigned *value)
{
  if (pin >= _nr_pins)
    throw -L4_EINVAL;

  switch (reg)
    {
    case Revision:
      *value = _regs[Revision] & 0xff;
      break;
    case Sysconfig:
      *value = _regs[Sysconfig] & 0xf;
      break;
    case Sysstatus:
      *value = _regs[Sysstatus] & 0x1;
      break;
    case Ctrl:
      *value = _regs[Ctrl] & 0x7;
      break;
    case Debouncing_time:
      *value = _regs[Debouncing_time] & 0xff;
      break;
    case Irq_status1:
    case Irq_enable1:
    case Wakeup_enable:
    case Irq_status2:
    case Irq_enable2:
    case Output_enable:
    case Data_in:
    case Data_out:
    case Level_detect_low:
    case Level_detect_high:
    case Rising_detect:
    case Falling_detect:
    case Debounce_enable:
    case Clr_irq_enable1:
    case Set_irq_enable1:
    case Clr_irq_enable2:
    case Set_irq_enable2:
    case Clr_wku_enable:
    case Set_wku_enable:
    case Clr_data_out:
    case Set_data_out:
      *value = (_regs[reg] >> _pin_shift(pin)) & 1;
      break;

    default:
      throw -L4_EINVAL;
    }
}

void
Gpio_omap35x_chip::config_pull(unsigned pin, unsigned mode)
{
  if (pin >= _nr_pins)
    throw -L4_EINVAL;

  switch (mode)
    {
    case Pull_none:
      mode = 0;
      break;
    case Pull_up:
      mode = 3;
      break;
    case Pull_down:
      mode = 1;
      break;
    }

  if (_scm)
    _scm->set_pull(_scm_offset[pin], mode);
}

Io_irq_pin *
Gpio_omap35x_chip::get_irq(unsigned pin)
{
  if (pin >= _nr_pins)
    throw -L4_EINVAL;

  return _irq_svr->register_pin(pin);
}

void
Gpio_omap35x_chip::multi_config_pad(Pin_slice const &mask, unsigned func, unsigned value)
{
  unsigned m = mask.mask;
  for (unsigned pin = mask.offset; pin < _nr_pins; ++pin, m >>= 1)
    if (m & 1)
      config_pad(pin, func, value);
}

void
Gpio_omap35x_chip::multi_setup(Pin_slice const &mask, unsigned mode, unsigned outvalues)
{
  unsigned m = mask.mask;
  for (unsigned pin = mask.offset; pin < _nr_pins; ++pin, m >>= 1, outvalues >>= 1)
    if (m & 1)
      setup(pin, mode, outvalues & 1);
}

int
Gpio_omap35x_chip::set_property(cxx::String const &prop, Prop_val const &val)
{
  unsigned _table = ~0U;
  int r = getval("scm_table", prop, val, &_table);
  if (r != -E_no_prop)
    {
      // we should assert _table >=0 && _table <= 5
      _scm_offset = scm_offsets[_table];
      return r;
    }

  return Hw::Device::set_property(prop, val);
}

void
Gpio_omap35x_chip::init()
{
  Gpio_device::init();

  Resource *regs = resources()->find("regs");
  if (!regs || regs->type() != Resource::Mmio_res)
    {
      d_printf(DBG_ERR, "error: %s: no base address set for device: Gpio_omap35x_chip\n"
                        "       missing or wrong 'regs' resource\n"
                        "       the chip will not work at all!\n", name());
      return;
    }

  l4_addr_t phys_base = regs->start();
  l4_addr_t size = regs->size();

  if (size < 0x100 || size > (1 << 12))
    {
      d_printf(DBG_ERR, "error: %s: invalid mmio size (%lx) for device: Gpio_omap35x_chip\n"
                        "       the chip will not work at all!\n", name(), size);
      return;
    }

  l4_addr_t vbase = res_map_iomem(phys_base, size);
  if (!vbase)
    {
      d_printf(DBG_ERR, "error: %s: cannot map registers for Gpio_omap35x_chip\n"
                        "       phys=%lx-%lx", name(),
                        phys_base, phys_base + size - 1);
      return;
    }

  d_printf(DBG_DEBUG2, "%s: Gpio_omap35x_chip: mapped registers to %08lx\n",
           name(), vbase);

  _regs = new Hw::Mmio_register_block<32>(vbase);

  Resource *irq = resources()->find("irq");
  if (irq && irq->type() == Resource::Irq_res)
    _irq_svr = new Gpio_irq_server(irq->start(), _nr_pins, vbase);
  else
    d_printf(DBG_WARN, "warning: %s: Gpio_omap35x_chip no irq configured\n", name());

  // find Scm device
  for (auto i = Hw::Device::iterator(0, system_bus(), L4VBUS_MAX_DEPTH);
       i != Hw::Device::iterator(); ++i)
    if ((*i)->match_cid("scm-omap35x"))
      _scm = reinterpret_cast<Scm_omap35x *>(*i);

  if (!_scm)
    d_printf(DBG_WARN, "warning: %s: could not find Scm device for device: Gpio_omap35x_chip\n"
                       "         Setting Gpio pin modes and PUD will be disabled\n", name());
}

static Hw::Device_factory_t<Gpio_omap35x_chip> __hw_pf_factory("Gpio_omap35x_chip");
}
