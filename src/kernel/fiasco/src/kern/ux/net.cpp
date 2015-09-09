//
// Network handling for Fiasco-UX
//

INTERFACE:

class Net
{
public:
  static void init();
  static void enable();

private:
  static int tunfd;

  static void bootstrap();
};

// ------------------------------------------------------------------------
IMPLEMENTATION:

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include "boot_info.h"
#include "initcalls.h"
#include "irq_chip.h"
#include "kmem.h"
#include "pic.h"
#include "warn.h"

int Net::tunfd;

IMPLEMENT FIASCO_INIT
void
Net::bootstrap()
{
  char s_net_fd[3];

  snprintf (s_net_fd,  sizeof (s_net_fd),  "%u",  tunfd);

  execl (Boot_info::net_program(), Boot_info::net_program(),
         "-t", s_net_fd, NULL);
}

IMPLEMENT FIASCO_INIT
void
Net::init()
{
  // Check if frame buffer is available
  if (!Boot_info::net())
    return;

  if ((tunfd = open("/dev/net/tun", O_RDWR)) < 0)
    {
      perror("open of /dev/net/tun");
      exit(1);
    }

  // Setup virtual interrupt
  if (!Pic::setup_irq_prov (Pic::Irq_net,
                            Boot_info::net_program(), bootstrap))
    {
      puts("Problems setting up network interrupt!");
      exit(1);
    }

  Kip::k()->vhw()->set_desc(Vhw_entry::TYPE_NET,
                            0, 0,
                            Pic::Irq_net,
                            Pic::get_pid_for_irq_prov(Pic::Irq_net), tunfd);

  printf("Starting Network.\n\n");
}
