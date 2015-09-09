-- vi:ft=lua

local Res = Io.Res
local Hw = Io.Hw

Io.hw_add_devices(function()

  GPIO = Hw.Gpio_bcm2835_chip(function()
    hid = "gpio-bcm2835-GPIO";
    pins = 54;
    regs = Res.mmio(0x20200000, 0x202000b4);
    int0 = Res.irq(49);
    int2 = Res.irq(51);
  end);

  MBOX = Hw.Device(function()
    hid = "BCM2835_mbox";
    regs = Res.mmio(0x2000b880, 0x2000bfff);
  end);

  FB = Hw.Device(function()
    hid = "BCM2835_fb";
    mem = Res.mmio(0x5c006000, 0x60005fff);
  end);

  BSC2 = Hw.Device(function()
    hid = "BCM2835_bsc2";
    regs = Res.mmio(0x20805000, 0x20805fff);
    irq = Res.irq(53, Io.Resource.Irq_type_raising_edge);
  end);
end)
