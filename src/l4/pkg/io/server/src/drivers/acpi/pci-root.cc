#include "io_acpi.h"
#include "__acpi.h"
#include "pci.h"
#include "debug.h"
#include <l4/cxx/list>


namespace {

using namespace Hw;
using Hw::Device;


struct Acpi_pci_root_drv : Acpi_device_driver
{
  Acpi_dev *probe(Device *device, ACPI_HANDLE acpi_hdl,
                  ACPI_DEVICE_INFO const *info)
  {
    d_printf(DBG_DEBUG, "Found PCI root bridge...\n");
    // do this first so we have an Acpi_dev feature installed for 'device'
    Acpi_dev *adev = Acpi_device_driver::probe(device, acpi_hdl, info);
    if (Hw::Pci::Root_bridge *rb = Hw::Pci::root_bridge(0))
      {
        if (rb->host())
          {
            // we found a second root bridge
            // create a new root pridge instance
            rb = new Hw::Pci::Port_root_bridge(-1, device);
          }
        else
          rb->set_host(device);

        device->add_feature(rb);
      }
    else
      d_printf(DBG_ERR, "ERROR: there is no PCI bus driver for this platform\n");


    return adev;
  }
};

static Acpi_pci_root_drv pci_root_drv;

struct Init
{
  Init()
  {
    Acpi_device_driver::register_driver(PCI_ROOT_HID_STRING, &pci_root_drv);
    Acpi_device_driver::register_driver(PCI_EXPRESS_ROOT_HID_STRING, &pci_root_drv);
  }
};

static Init init;

}

