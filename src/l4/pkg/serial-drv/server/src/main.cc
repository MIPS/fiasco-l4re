/*
 * (c) 2010 Alexander Warg <warg@os.inf.tu-dresden.de>,
 *          Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include <l4/drivers/uart_pl011.h>
#include <l4/drivers/uart_omap35x.h>
#include <l4/drivers/uart_pxa.h>
#include <l4/io/io.h>
#include <l4/re/env>
#include <l4/re/error_helper>
#include <l4/re/namespace>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/object_registry>
#include <l4/re/util/icu_svr>
#include <l4/re/util/vcon_svr>
#include <l4/sys/irq>
#include <l4/util/util.h>

#include <cstdlib>
#include <cstdio>


static L4::Cap<void> srv_rcv_cap;

class Loop_hooks :
  public L4::Ipc_svr::Ignore_errors,
  public L4::Ipc_svr::Default_timeout,
  public L4::Ipc_svr::Compound_reply
{
public:
  void setup_wait(L4::Ipc::Istream &istr, bool)
  {
    istr.reset();
    istr << L4::Ipc::Small_buf(srv_rcv_cap.cap(), L4_RCV_ITEM_LOCAL_ID);
    l4_utcb_br_u(istr.utcb())->bdr = 0;
  }
};

using L4Re::Env;
using L4Re::Util::Registry_server;

static Registry_server<Loop_hooks> server(l4_utcb(),
                                          Env::env()->main_thread(),
                                          Env::env()->factory());

using L4Re::Util::Vcon_svr;
using L4Re::Util::Icu_cap_array_svr;

class Serial_drv :
  public Vcon_svr<Serial_drv>,
  public Icu_cap_array_svr<Serial_drv>,
  public L4::Server_object
{
public:
  Serial_drv();
  virtual ~Serial_drv() throw() {}

  bool running() const { return _running; }

  int vcon_write(const char *buffer, unsigned size);
  unsigned vcon_read(char *buffer, unsigned size);

  L4::Cap<void> rcv_cap() { return srv_rcv_cap; }

  int handle_irq();

  bool init();
  int dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios);

private:
  bool _running;
  L4::Uart *_uart;
  L4::Cap<L4::Irq> _uart_irq;
  Icu_cap_array_svr<Serial_drv>::Irq _irq;
};

Serial_drv::Serial_drv()
  : Icu_cap_array_svr<Serial_drv>(1, &_irq),
    _running(false), _uart(0), _uart_irq(L4_INVALID_CAP),
    _irq()
{
  if (init())
    _running = true;
}

int
Serial_drv::vcon_write(const char *buffer, unsigned size)
{
  _uart->write(buffer, size);
  return -L4_EOK;
}

unsigned
Serial_drv::vcon_read(char *buffer, unsigned size)
{
  unsigned i = 0;
  while (_uart->char_avail() && size)
    {
      int c = _uart->get_char(false);
      if (c >= 0)
	{
	  buffer[i++] = (char)c;
	  size--;
	}
      else
	break;
    }
  // if there still some data available send this info to the client
  if (_uart->char_avail())
    i++;
  else
    _uart_irq->unmask();
  return i;
}

int
Serial_drv::handle_irq()
{
  if (_irq.cap().is_valid())
    _irq.cap()->trigger();

  //_uart_irq->unmask();

  return L4_EOK;
}

#ifdef USE_MALTA_UART
// NOTE: If there is no active receiver for the irq trigger in handle_irq()
// the irq is silently dropped. The irq will be masked until cleared when
// get_char and unmask are called in vcon_read. If instead there is a subsequent
// call to irq receive, that call will never return.
//
// A client of the Serial_drv may call vcon_read followed by irq receive (or irq
// wait) but if the interrupt is raised immediately after vcon_read the trigger
// will again be lost as there is no way to atomically attach to and unmask the
// trigger irq.
//
// The above imposes the limitation that the uart must be polled using vcon_read
// for input.
static int request_malta_uart_resources(L4::Io_register_block_mmio **regs, int
    *uart_irq)
{
  const int malta_uart_port = 0x3F8;
  l4_addr_t virt_base;
  l4io_device_handle_t uartdev_hdl;
  l4io_resource_handle_t uartres_hdl;

  if (l4io_lookup_device("Malta UART", &uartdev_hdl, 0, &uartres_hdl))
    {
      printf("serial-drv: Could not find UART iomem\n");
      return -1;
    }

  virt_base = l4io_request_resource_iomem(uartdev_hdl, &uartres_hdl);
  if (virt_base == 0)
    {
      printf("serial-drv: Could not map UART iomem\n");
      return -1;
    }

  // set malta uart port base
  virt_base |= malta_uart_port;

  printf("serial-drv: virtual base at %lx\n", virt_base);

  l4io_resource_t res;
  if (l4io_lookup_resource(uartdev_hdl, L4IO_RESOURCE_IRQ, &uartres_hdl, &res))
    {
      printf("serial-drv: Could not find UART irq\n");
      return -1;
    }

  *uart_irq = res.start;

  printf("serial-drv: uart_irq is %x\n", *uart_irq);

  *regs = new L4::Io_register_block_mmio(virt_base);

  return 0;
}
#endif // USE_MALTA_UART

bool
Serial_drv::init()
{
#ifndef USE_MALTA_UART
  int irq_num = 37;
  l4_addr_t phys_base = 0x1000a000;
#if 0
  int irq_num = 74;
  l4_addr_t phys_base = 0x49020000;
#endif
  l4_addr_t virt_base = 0;

  if (l4io_request_iomem(phys_base, 0x1000, L4IO_MEM_NONCACHED, &virt_base))
    {
      printf("serial-drv: request io-memory from l4io failed.\n");
      return false;
    }
  printf("serial-drv: virtual base at:%lx\n", virt_base);

  L4::Io_register_block_mmio *regs = new L4::Io_register_block_mmio(virt_base);

  _uart = new (malloc(sizeof(L4::Uart_pl011))) L4::Uart_pl011(24019200);
  //_uart = new (malloc(sizeof(L4::Uart_omap35x))) L4::Uart_omap35x;

#else
  int irq_num = 0;
  L4::Io_register_block_mmio *regs;

  if (request_malta_uart_resources(&regs, &irq_num))
      return false;

  _uart = new (malloc(sizeof(L4::Uart_16550))) L4::Uart_16550(115200);
#endif // USE_MALTA_UART

  _uart->startup(regs);

  _uart_irq = L4Re::Util::cap_alloc.alloc<L4::Irq>();
  if (!_uart_irq.is_valid())
    {
      printf("serial-drv: Alloc capability for uart-irq failed.\n");
      return false;
    }

  if (l4io_request_irq(irq_num, _uart_irq.cap()))
    {
      printf("serial-drv: request uart-irq from l4io failed\n");
      return false;
    }

  /* setting IRQ type to L4_IRQ_F_POS_EDGE seems to be wrong place */
  if (l4_error(_uart_irq->attach((l4_umword_t)static_cast<L4::Server_object *>(this),
	  L4Re::Env::env()->main_thread())))
    {
      printf("serial-drv: attach to uart-irq failed.\n");
      return false;
    }

  if ((l4_ipc_error(_uart_irq->unmask(), l4_utcb())))
    {
      printf("serial-drv: unmask uart-irq failed.\n");
      return false;
    }
  _uart->enable_rx_irq(true);

  srv_rcv_cap = L4Re::Util::cap_alloc.alloc<void>();
  if (!srv_rcv_cap.is_valid())
    {
      printf("serial-drv: Alloc capability for rcv-cap failed.\\n");
      return false;
    }
  
  return true;
}

int
Serial_drv::dispatch(l4_umword_t obj, L4::Ipc::Iostream &ios)
{
  l4_msgtag_t tag;
  ios >> tag;
  switch (tag.label())
    {
    case L4_PROTO_IRQ:
      if (!L4Re::Util::Icu_svr<Serial_drv>::dispatch(obj, ios))
	return handle_irq();
    case L4_PROTO_LOG:
      return L4Re::Util::Vcon_svr<Serial_drv>::dispatch(obj, ios);
    default:
      return -L4_EBADPROTO;
    }
}

int main()
{
  Serial_drv serial_drv;

  if (!server.registry()->register_obj(&serial_drv, "vcon"))
    {
      printf("Failed to register serial driver; Aborting.\n");
      return 1;
    }

  if (!serial_drv.running())
    {
      printf("Failed to initialize serial driver; Aborting.\n");
      return 1;
    }

  printf("Starting server loop\n");
  server.loop();

  return 0;
}
