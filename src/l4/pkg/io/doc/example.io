-- vim:ft=lua
-- Example configuration for io

-- Configure two platform devices to be known to io
Io.Dt.add_children(Io.system_bus(), function()

  FOODEVICE = Io.Hw.Device(function()
    hid  = "FOODEVICE";
    compatible = {"dev-foo,mmio", "dev-foo"};
    -- note: names for resources are truncated to 4 letters
    int  = Io.Res.irq(17);
    regs = Io.Res.mmio(0x6f000000, 0x6f007fff);
  end);

  BARDEVICE = Io.Hw.Device(function()
    hid  = "BARDEVICE";
    compatible = {"dev-bar,mmio", "dev-bar"};
    -- note: names for resources are truncated to 4 letters
    intA = Io.Res.irq(19);
    intB = Io.Res.irq(20);
    regs = Io.Res.mmio(0x6f100000, 0x6f100fff);
  end);
end);


Io.add_vbusses
{
-- Create a virtual bus for a client and give access to FOODEVICE
  client1 = Io.Vi.System_bus(function ()
    dev = wrap(Io.system_bus():match("FOODEVICE"));
  end);

-- Create a virtual bus for another client and give it access to BARDEVICE
  client2 = Io.Vi.System_bus(function ()
    dev = wrap(Io.system_bus():match("BARDEVICE"));
  end);
}
