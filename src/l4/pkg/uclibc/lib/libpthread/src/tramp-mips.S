#include <sys/asm.h>
#include <sys/regdef.h>

.text
.align 2
.global __pthread_new_thread_entry
__pthread_new_thread_entry:
.set push
.set noreorder
  lw    a0, 8(sp)
  lw    t9, 0(sp)
  addiu sp, sp, 12
  subu  sp, 24
  jalr  t9
  nop
.set pop
