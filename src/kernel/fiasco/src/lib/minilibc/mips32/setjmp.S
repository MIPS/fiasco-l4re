#include "regdef.h"
#include "asm.h"
#include <setjmp.h>

.text
.weak setjmp
.type setjmp,function
setjmp:
LEAF(__setjmp)
    sw      ra,0(a0)
    sw      sp,4(a0)

    sw      $16,8(a0)
    sw      $17,12(a0)
    sw      $18,16(a0)
    sw      $19,20(a0)
    sw      $20,24(a0)
    sw      $21,28(a0)
    sw      $22,32(a0)
    sw      $23,36(a0)

    sw      $30,40(a0)
    sw      gp,44(a0)

    move    $2,$0
    j      $31

END(__setjmp)


.text
LEAF(longjmp)
    lw      ra,0(a0)
    lw      sp,4(a0)

    lw      $16,8(a0)
    lw      $17,12(a0)
    lw      $18,16(a0)
    lw      $19,20(a0)
    lw      $20,24(a0)
    lw      $21,28(a0)
    lw      $22,32(a0)
    lw      $23,36(a0)

    lw      $30,40(a0)
    lw      gp,44(a0)

    bne $5,$0,1f
    li  $5,1
1:
    move    $2,$5
    lw      $25,0(a0)
    jr      $25
END(longjmp)
