/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

#include <l4/vcpu/vcpu.h>

#include <stdio.h>

void l4vcpu_print_state_arch(l4_vcpu_state_t *vcpu,
                             const char *prefix) L4_NOTHROW
{
  l4_vcpu_regs_t& r = vcpu->r;

  printf("%svcpu=%p pfa=%08lx err=%08lx label=%08lx\n",
         prefix, vcpu, r.pfa, r.err, vcpu->i.label);

  printf("%sstatus %08lx ip/epc %08lx sp %08lx\n",
      prefix, r.status, r.ip, r.sp);
  printf("%scause %08lx badvaddr %08lx\n",
      prefix, r.cause, r.badvaddr);
  printf("%shi %08lx lo %08lx\n",
      prefix, r.hi, r.lo);

  printf("%sr0[%2d]: %08lx at[%2d]: %08lx v0[%2d]: %08lx v1[%2d]: %08lx\n",
    prefix, 0, r.r[0], 1, r.r[1], 2, r.r[2], 3, r.r[3]);
  printf("%sa0[%2d]: %08lx a1[%2d]: %08lx a2[%2d]: %08lx a3[%2d]: %08lx\n",
    prefix, 4, r.r[4], 5, r.r[5], 6, r.r[6], 7, r.r[7]);
  printf("%st0[%2d]: %08lx t1[%2d]: %08lx t2[%2d]: %08lx t3[%2d]: %08lx\n",
    prefix, 8, r.r[8], 9, r.r[9], 10, r.r[10], 11, r.r[11]);
  printf("%st4[%2d]: %08lx t5[%2d]: %08lx t6[%2d]: %08lx t7[%2d]: %08lx\n",
    prefix, 12, r.r[12], 13, r.r[13], 14, r.r[14], 15, r.r[15]);
  printf("%ss0[%2d]: %08lx s1[%2d]: %08lx s2[%2d]: %08lx s3[%2d]: %08lx\n",
    prefix, 16, r.r[16], 17, r.r[17], 18, r.r[18], 19, r.r[19]);
  printf("%ss4[%2d]: %08lx s5[%2d]: %08lx s6[%2d]: %08lx s7[%2d]: %08lx\n",
    prefix, 20, r.r[20], 21, r.r[21], 22, r.r[22], 23, r.r[23]);
  printf("%st8[%2d]: %08lx t9[%2d]: %08lx k0[%2d]: %08lx k1[%2d]: %08lx\n",
    prefix, 24, r.r[24], 25, r.r[25], 26, r.r[26], 27, r.r[27]);
  printf("%sgp[%2d]: %08lx sp[%2d]: %08lx s8[%2d]: %08lx ra[%2d]: %08lx\n",
    prefix, 28, r.r[28], 29, r.r[29], 30, r.r[30], 31, r.r[31]);
}
