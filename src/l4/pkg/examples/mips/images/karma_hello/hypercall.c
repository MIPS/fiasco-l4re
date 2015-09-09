/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file is based on L4Linux linux/arch/mips/kernel/hypercall.c
 */

#include "asm/hypercall.h"
#include "asm/hypcall_ops.h"

void karma_hypercall0_vz(const unsigned long cmd){
    register unsigned long raw __asm__("s0") = cmd;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : : "r"(raw) : "memory"
    );
}
void karma_hypercall1_vz(const unsigned long cmd, unsigned long * reg1){
    register unsigned long raw __asm__("s0") = cmd;
    register unsigned long s1 __asm__("s1") = *reg1;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : "=r"(*reg1)
	    : "r"(raw), "0"(s1) : "memory"
    );
}
void karma_hypercall2_vz(const unsigned long cmd, unsigned long * reg1, unsigned long * reg2){
    register unsigned long raw __asm__("s0") = cmd;
    register unsigned long s1 __asm__("s1") = *reg1;
    register unsigned long s2 __asm__("s2") = *reg2;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : "=r"(*reg1), "=r"(*reg2)
            : "r"(raw), "0"(s1), "1"(s2) : "memory"
    );
}
void karma_hypercall3_vz(const unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3){
    register unsigned long raw __asm__("s0") = cmd;
    register unsigned long s1 __asm__("s1") = *reg1;
    register unsigned long s2 __asm__("s2") = *reg2;
    register unsigned long s3 __asm__("s3") = *reg3;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : "=r"(*reg1), "=r"(*reg2), "=r"(*reg3)
            : "r"(raw), "0"(s1), "1"(s2), "2"(s3) : "memory"
    );
}
void karma_hypercall4_vz(const unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3, unsigned long * reg4){
    register unsigned long raw __asm__("s0") = cmd;
    register unsigned long s1 __asm__("s1") = *reg1;
    register unsigned long s2 __asm__("s2") = *reg2;
    register unsigned long s3 __asm__("s3") = *reg3;
    register unsigned long s4 __asm__("s4") = *reg4;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : "=r"(*reg1), "=r"(*reg2), "=r"(*reg3), "=r"(*reg4)
            : "r"(raw), "0"(s1), "1"(s2), "2"(s3), "3"(s4) : "memory"
    );
}

struct karma_hypercall_s karma_hypercall_vz = {
    .hc_args0 = karma_hypercall0_vz,
    .hc_args1 = karma_hypercall1_vz,
    .hc_args2 = karma_hypercall2_vz,
    .hc_args3 = karma_hypercall3_vz,
    .hc_args4 = karma_hypercall4_vz,
};

struct karma_hypercall_s * karma_hypercall;

void karma_hypercall_init(void){
    karma_hypercall = &karma_hypercall_vz;
}

/* bypass karma devices and issue hypcall to exit vm */
void karma_hypercall_exit_op(void){
    asm volatile(
            ".set push; .set noreorder; \n"
            ASM_HYPCALL_STR(HYPCALL_MAGIC_EXIT) "\n"
            ".set pop; \n"
            : : : "memory"
    );
}


