/*****************************************************************************/
/**
 * \file
 * \brief   L4 kernel event tracing
 * \ingroup api_calls
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Björn Döbel <doebel@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
/*****************************************************************************/
#ifndef __L4_KTRACE_H__
#define __L4_KTRACE_H__

#include <l4/sys/types.h>
#include <l4/sys/ktrace_events.h>

/**
 * \brief Log event types
 * \ingroup api_calls_fiasco
 */
enum {
  LOG_EVENT_MAX_EVENTS = 16,
};

/**
 * Trace-buffer status window descriptor.
 * \ingroup api_calls_fiasco
 */
// keep in sync with fiasco/src/jabi/jdb_ktrace.cpp
typedef struct
{
  /// Address of trace-buffer
  l4_tracebuffer_entry_t *tracebuffer;
  /// Size of trace-buffer
  l4_umword_t size;
  /// Version number of trace-buffer  (incremented if trace-buffer overruns)
  volatile l4_uint64_t version;
} l4_tracebuffer_status_window_t;

/**
 * Trace-buffer status.
 * \ingroup api_calls_fiasco
 */
// keep in sync with fiasco/src/jabi/jdb_ktrace.cpp
typedef struct
{
  l4_tracebuffer_status_window_t window[2];
  /// Address of the most current event in trace-buffer.
  volatile l4_tracebuffer_entry_t * current_entry;
  /// Available LOG events
  l4_umword_t logevents[LOG_EVENT_MAX_EVENTS];

  /// Scaler used for translation of CPU cycles to nano seconds
  l4_umword_t scaler_tsc_to_ns;
  /// Scaler used for translation of CPU cycles to micro seconds
  l4_umword_t scaler_tsc_to_us;
  /// Scaler used for translation of nano seconds to CPU cycles
  l4_umword_t scaler_ns_to_tsc;

  /// Number of context switches (intra AS or inter AS)
  volatile l4_umword_t cnt_context_switch;
  /// Number of inter AS context switches
  volatile l4_umword_t cnt_addr_space_switch;
  /// How often was the IPC shortcut taken
  volatile l4_umword_t cnt_shortcut_failed;
  /// How often was the IPC shortcut not taken
  volatile l4_umword_t cnt_shortcut_success;
  /// Number of hardware interrupts (without kernel scheduling interrupt)
  volatile l4_umword_t cnt_irq;
  /// Number of long IPCs
  volatile l4_umword_t cnt_ipc_long;
  /// Number of page faults
  volatile l4_umword_t cnt_page_fault;
  /// Number of faults (application runs at IOPL 0 and tries to execute
  /// cli, sti, in, or out but does not have a sufficient in the I/O bitmap)
  volatile l4_umword_t cnt_io_fault;
  /// Number of tasks created
  volatile l4_umword_t cnt_task_create;
  /// Number of reschedules
  volatile l4_umword_t cnt_schedule;
  /// Number of flushes of the I/O bitmap. Increases on context switches
  /// between two small address spaces if at least one of the spaces has
  /// an I/O bitmap allocated.
  volatile l4_umword_t cnt_iobmap_tlb_flush;

} l4_tracebuffer_status_t;

/**
 * Return trace-buffer status.
 * \ingroup api_calls_fiasco
 *
 * \return Pointer to trace-buffer status struct.
 */
L4_INLINE l4_tracebuffer_status_t *
fiasco_tbuf_get_status(void);

/**
 * Return the physical address of the trace-buffer status struct.
 * \ingroup api_calls_fiasco
 *
 * \return physical address of status struct.
 */
L4_INLINE l4_addr_t
fiasco_tbuf_get_status_phys(void);

/**
 * Create new trace-buffer entry with describing \<text\>.
 * \ingroup api_calls_fiasco
 *
 * \param  text   Logging text
 * \return Pointer to trace-buffer entry
 */
L4_INLINE l4_umword_t
fiasco_tbuf_log(const char *text);

/**
 * Create new trace-buffer entry with describing \<text\> and three additional
 * values.
 * \ingroup api_calls_fiasco
 *
 * \param  text   Logging text
 * \param  v1     first value
 * \param  v2     second value
 * \param  v3     third value
 * \return Pointer to trace-buffer entry
 */
L4_INLINE l4_umword_t
fiasco_tbuf_log_3val(const char *text, l4_umword_t v1, l4_umword_t v2, l4_umword_t v3);

/**
 * Create new trace-buffer entry with binary data.
 * \ingroup api_calls_fiasco
 *
 * \param  data       binary data
 * \return Pointer to trace-buffer entry
 */
L4_INLINE l4_umword_t
fiasco_tbuf_log_binary(const unsigned char *data);

/**
 * Clear trace-buffer.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_tbuf_clear(void);

/**
 * Dump trace-buffer to kernel console.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_tbuf_dump(void);

/**
 * Disable the kernel scheduling timer.
 */
L4_INLINE void
fiasco_timer_disable(void);

/**
 * Enable the kernel scheduling timer (after it was disabled with
 * fiasco_timer_disable).
 */
L4_INLINE void
fiasco_timer_enable(void);

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE l4_tracebuffer_status_t *
fiasco_tbuf_get_status(void)
{
  l4_tracebuffer_status_t *tbuf;
  asm("int $3; cmpb $29, %%al" : "=a" (tbuf) : "0" (0));
  return tbuf;
}

L4_INLINE l4_addr_t
fiasco_tbuf_get_status_phys(void)
{
  l4_addr_t tbuf_phys;
  asm("int $3; cmpb $29, %%al" : "=a" (tbuf_phys) : "0" (5));
  return tbuf_phys;
}

L4_INLINE l4_umword_t
fiasco_tbuf_log(const char *text)
{
  l4_umword_t offset;
  asm volatile("int $3; cmpb $29, %%al"
	      : "=a" (offset)
	      : "a" (1), "d" (text));
  return offset;
}

L4_INLINE l4_umword_t
fiasco_tbuf_log_3val(const char *text, l4_umword_t v1, l4_umword_t v2, l4_umword_t v3)
{
  l4_umword_t offset;
  asm volatile("int $3; cmpb $29, %%al"
	      : "=a" (offset)
	      : "a" (4), "d" (text), "c" (v1), "S" (v2), "D" (v3));
  return offset;
}

L4_INLINE void
fiasco_tbuf_clear(void)
{
  asm volatile("int $3; cmpb $29, %%al" : : "a" (2));
}

L4_INLINE void
fiasco_tbuf_dump(void)
{
  asm volatile("int $3; cmpb $29, %%al" : : "a" (3));
}

L4_INLINE void
fiasco_timer_disable(void)
{
  asm volatile("int $3; cmpb $29, %%al" : : "a" (6));
}

L4_INLINE void
fiasco_timer_enable(void)
{
  asm volatile("int $3; cmpb $29, %%al" : : "a" (7));
}

L4_INLINE l4_umword_t
fiasco_tbuf_log_binary(const unsigned char *data)
{
  l4_umword_t offset;
  asm volatile("int $3; cmpb $29, %%al"
               : "=a" (offset)
               : "a" (8), "d" (data));
  return offset;
}

#endif

