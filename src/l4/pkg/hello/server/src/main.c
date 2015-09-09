/*
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 *
 * This file incorporates work covered by the following copyright notice:
 */

/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Lukas Grützmacher <lg2@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>


static __inline__ void *thread_self (void)
{
  return (l4_utcb_tcr()->user[0]); 
}


__thread char *tname;

void *
hello_thread(void *arg)
{
	int i = 0;
  int err;
  tname = (char *) arg;

  err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (err != 0)
    printf("pthread_setcancelstate failed %s\n", strerror(err));

  for (;;)
    {
      printf("[%s/%d] Hello World (%p)!\n", tname, i++, thread_self());
			fflush(stdout);
      /* Cancel point before we sleep */
      pthread_testcancel();
      sleep(1);
    }
  printf("%s: exiting\n", tname);
}

pthread_t thread1, thread2;
int
main(void)
{
  int err;
  void *res;
  pthread_create (&thread1, NULL, hello_thread, (void *)"FOO");
  pthread_create (&thread2, NULL, hello_thread, (void *)"BAR");

  printf("********** Main thread will sleep for 10 secs ****************\n");
  fflush (stdout);
  sleep (10);

  printf("********** Main thread is back baby, cancel thread1 ****************\n");
  fflush (stdout);

  pthread_cancel(thread1);

  printf("[%p] Main thread joining thread1 (%p)\n", thread_self(), thread1);
  fflush(stdout);

  err = pthread_join(thread1, &res);
  if (err != 0)
    printf("pthread_join failed %s\n", strerror(err));

  if (res == PTHREAD_CANCELED)
    printf("thread1 cancelled\n");
  else
    printf("thread1 could not be cancelled!\n");

  fflush(stdout);

  printf("[%p] Main thread joining thread2, should not return (%p)\n", thread_self(), thread2);
  err = pthread_join(thread2, &res);

  if (res == PTHREAD_CANCELED)
    printf("thread2 cancelled\n");
  else
    printf("thread2 exit result: %p\n", res);

  fflush(stdout);
}
