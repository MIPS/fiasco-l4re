/*
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License 2.1.  See the file "COPYING-LGPL-2.1" in the main directory of
 * this archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <l4/cxx/thread>


void L4_cxx_start(void);

void L4_cxx_start(void)
{
  /*
   * get _this pointer from the stack and call _this->execute()
   *
   * void Thread::start_cxx_thread(Thread *_this)
   * { _this->execute(); }
   *
   */
  asm volatile (".global L4_Thread_start_cxx_thread \n"
                "L4_Thread_start_cxx_thread:        \n"
                "lw  $a0, 4($sp)                    \n"
                "la  $t9,1f                         \n"
                "jr  $t9                            \n"
                "    nop                            \n"
                "1: .word L4_Thread_execute         \n");
}

void L4_cxx_kill(void)
{
  /*
   * get _this pointer from the stack and call _this->shutdown()
   *
   * void Thread::kill_cxx_thread(Thread *_this)
   * { _this->shutdown(); }
   *
   */
  asm volatile (".global L4_Thread_kill_cxx_thread \n"
                "L4_Thread_kill_cxx_thread:        \n"
                "lw  $a0, 4($sp)                    \n"
                "la  $t9,1f                         \n"
                "jr  $t9                            \n"
                "    nop                            \n"
                "1: .word L4_Thread_shutdown       \n");
}

