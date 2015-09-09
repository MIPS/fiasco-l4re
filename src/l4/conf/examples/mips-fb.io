# vim:set ft=ioconfig:
# configuration file for io

gui => new System_bus()
{
  ps2dev => wrap(hw-root.match("PNP0303"));
}

fbdrv => new System_bus()
{
  PCI0 => new PCI_bus_ident()
  {
    host_bridge_dummy => new PCI_dummy_device();
    pci_gfx[] => wrap(hw-root.match("PCI/CC_03"));
  }
  #bios # dev1 => wrap(hw-root.match("BIOS"));
  #vga  # dev2 => wrap(hw-root.match("PNP0900"));
  #timer# dev3 => wrap(hw-root.match("PNP0100"));

  # Add malta system controller ports to fb vbus
  # This replaces PNP0900 for access to VGA 0x3c0-3df
  sysport => wrap(hw-root.SYSPORT);
  # --Res.mmio(0x180003b0, 0x180003bf, 0xc000); -- MDA
  # --Res.mmio(0x180003c0, 0x180003df, 0xc000); -- EGA/VGA
}
