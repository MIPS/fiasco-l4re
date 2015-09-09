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

#include <l4/sys/scheduler.h>
#include <l4/re/env.h>

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

static l4_umword_t cpu_map, cpu_nrs, cpus_online;

static int check_cpus(void);
static void thread_migrate(void);

static void *thread_consume(void *d)
{
  (void)d;
  l4shmc_area_t shmarea;
  l4shmc_chunk_t p_one;
  l4shmc_signal_t s_one, s_done;
  const unsigned int MICROSEC_DELAY = 1 * 1000000;
  l4_timeout_t timeout = l4_timeout(L4_IPC_TIMEOUT_NEVER,l4util_micros2l4to(MICROSEC_DELAY));

  l4_debugger_set_object_name(pthread_getl4cap(pthread_self()), "cons");

  // attach to shared memory area
  CHK(l4shmc_attach("testshm", &shmarea));

  // add a signal
  CHK(l4shmc_add_signal(&shmarea, "done", &s_done));

  // attach signal to this thread
  CHK(l4shmc_attach_signal_to(&shmarea, "prod",
                              pthread_getl4cap(pthread_self()), 10000, &s_one));

  // get chunk 'one'
  CHK(l4shmc_get_chunk(&shmarea, "one", &p_one));

  // connect chunk and signal
  CHK(l4shmc_connect_chunk_signal(&p_one, &s_one));

  while (1)
    {
      while(l4shmc_wait_chunk_to(&p_one, timeout)) {
        // maybe we missed the initial signal?
        if (l4shmc_is_chunk_ready(&p_one))
          break;
      }

      printf("CONSUMER: Received from chunk one: %s\n",
             (char *)l4shmc_chunk_ptr(&p_one));

      memset(l4shmc_chunk_ptr(&p_one), 0, l4shmc_chunk_size(&p_one));

      CHK(l4shmc_chunk_consumed(&p_one));

      CHK(l4shmc_trigger(&s_done));

      thread_migrate();
    }

  return NULL;
}

int main(void)
{
  pthread_t two;
  int wait_for_producer_ms = 1000;

  if (check_cpus())
    return 1;

  // wait for producer to have created "testshm" namespace
  l4_sleep(wait_for_producer_ms);

  pthread_create(&two, 0, thread_consume, 0);

  l4_sleep_forever();

  return 0;
}

/* Helper function to get the next CPU */
static unsigned get_next_cpu(unsigned c)
{
  unsigned x = c;
  for (;;)
    {
      x = (x + 1) % cpu_nrs;
      if (l4_scheduler_is_online(l4re_env()->scheduler, x))
        return x;
      if (x == c)
        return c;
    }
}

/* Check how many CPUs we have available.
 */
static int check_cpus(void)
{
  l4_sched_cpu_set_t cs = l4_sched_cpu_set(0, 0, 1);

  if (l4_error(l4_scheduler_info(l4re_env()->scheduler, &cpu_nrs, &cs)) < 0)
    return 1;

  cpu_map = cs.map;
  cpus_online = __builtin_popcount(cpu_map);

  if (cpu_nrs > 1)
    printf("Found %ld/%ld CPUs online.\n", cpus_online, cpu_nrs);

  if (cpu_nrs >= L4_MWORD_BITS)
    {
      printf("Will only handle %ld CPUs.\n", cpu_nrs);
      cpu_nrs = L4_MWORD_BITS;
    }

  return 0;
}

static void thread_migrate(void)
{
  static unsigned c = 0;

  if (cpus_online < 2)
    return;

  l4_cap_idx_t thread_cap = pthread_getl4cap(pthread_self());
  l4_sched_param_t sp = l4_sched_param(20, 0);
  c = get_next_cpu(c);
  sp.affinity = l4_sched_cpu_set(c, 0, 1);
  if (l4_error(l4_scheduler_run_thread(l4re_env()->scheduler, thread_cap, &sp)))
    printf("Error migrating thread to CPU%02d\n", c);

  printf("Migrated Thread -> CPU%02d\n", c);
}
