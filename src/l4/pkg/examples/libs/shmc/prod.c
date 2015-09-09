/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

/*
 * This example is based on prodcons.c and uses shared memory between producer
 * and consumer threads in separate tasks (rather than two threads in one task).
 */

#include <l4/shmc/shmc.h>

#include <l4/util/util.h>

#include <stdio.h>
#include <string.h>
#include <pthread-l4.h>

#include <l4/sys/thread.h>
#include <l4/sys/debugger.h>

/*
 * NOTE: The l4shmc API does not provide any way to recover lost signals. If
 * l4shmc_wait_chunk() or l4shmc_wait_signal() are not called before the other
 * side sends a signal, that signal will be lost and the waitee will hang.
 *
 * This example program is sensitive to the scheduling order of the threads. The
 * initialization order of the original version has been re-arranged to ensure
 * objects are created before the other side requests them. When started under
 * conditions that cause the threads to run in a different order (such as with
 * different system task priorities, extra printfs, or running in a VM) it may
 * cause a signal to be lost and both threads to hang. The program has been
 * changed to account for this.
 *
 * The printf statements may be printed in a different order dependent on
 * preemption and scheduling.
 */

// a small helper
#define CHK(func) if (func) { printf("failure: %d\n", __LINE__); return (void *)-1; }

static char some_data[] = "0-Hi consumer!";

void set_some_data(void);
void set_some_data(void) {
  static int i = 0;
  some_data[0] = '0' + i++;
  i %= 10;
}

static void *thread_producer(void *d)
{
  (void)d;
  l4shmc_chunk_t p_one;
  l4shmc_signal_t s_one, s_done;
  l4shmc_area_t shmarea;

  l4_debugger_set_object_name(pthread_getl4cap(pthread_self()), "prod");

  // attach this thread to the shm object
  CHK(l4shmc_attach("testshm", &shmarea));

  // add a chunk
  CHK(l4shmc_add_chunk(&shmarea, "one", 1024, &p_one));

  // add a signal
  CHK(l4shmc_add_signal(&shmarea, "prod", &s_one));

  // connect chunk and signal
  CHK(l4shmc_connect_chunk_signal(&p_one, &s_one));

  CHK(l4shmc_attach_signal_to(&shmarea, "done",
                              pthread_getl4cap(pthread_self()), 10000, &s_done));

  printf("PRODUCER: ready\n");

  while (1)
    {
      while (l4shmc_chunk_try_to_take(&p_one))
        printf("Uh, should not happen!\n"); //l4_thread_yield();

      set_some_data();

      memcpy(l4shmc_chunk_ptr(&p_one), some_data, sizeof(some_data));

      CHK(l4shmc_chunk_ready_sig(&p_one, sizeof(some_data)));

      printf("PRODUCER: Sent data %s\n", some_data);

      CHK(l4shmc_wait_signal(&s_done));

      l4_sleep(5000);
    }

  l4_sleep_forever();
  return NULL;
}


int main(void)
{
  pthread_t one;

  // create new shared memory area, 8K in size
  if (l4shmc_create("testshm", 8192))
    return 1;

  pthread_create(&one, 0, thread_producer, 0);

  l4_sleep_forever();

  return 0;
}
