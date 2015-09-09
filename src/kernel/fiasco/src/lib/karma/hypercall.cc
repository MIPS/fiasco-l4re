/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file is based on L4Linux linux/arch/x86/kernel/hypercall.c
 */

#include "hypercall.h"
#include "hypcall_ops.h"

static void karma_hypercall0_vz(unsigned long cmd) {
    register unsigned long raw __asm__("s0") = cmd;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : : "r"(raw) : "memory"
    );
}
static void karma_hypercall1_vz(unsigned long cmd, unsigned long * reg1) {
    register unsigned long raw __asm__("s0") = cmd;
    register unsigned long s1 __asm__("s1") = *reg1;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : "=r"(*reg1)
            : "r"(raw), "0"(s1) : "memory"
    );
}
static void karma_hypercall2_vz(unsigned long cmd, unsigned long * reg1, unsigned long * reg2) {
    register unsigned long raw __asm__("s0") = cmd;
    register unsigned long s1 __asm__("s1") = *reg1;
    register unsigned long s2 __asm__("s2") = *reg2;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : "=r"(*reg1), "=r"(*reg2)
            : "r"(raw), "0"(s1), "1"(s2) : "memory"
    );
}
static void karma_hypercall3_vz(unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3) {
    register unsigned long raw __asm__("s0") = cmd;
    register unsigned long s1 __asm__("s1") = *reg1;
    register unsigned long s2 __asm__("s2") = *reg2;
    register unsigned long s3 __asm__("s3") = *reg3;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : "=r"(*reg1),"=r"(*reg2),"=r"(*reg3)
            : "r"(raw), "0"(s1), "1"(s2), "2"(s3) : "memory"
    );
}
static void karma_hypercall4_vz(unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3, unsigned long * reg4) {
    register unsigned long raw __asm__("s0") = cmd;
    register unsigned long s1 __asm__("s1") = *reg1;
    register unsigned long s2 __asm__("s2") = *reg2;
    register unsigned long s3 __asm__("s3") = *reg3;
    register unsigned long s4 __asm__("s4") = *reg4;
    asm volatile(
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            : "=r"(*reg1), "=r"(*reg2), "=r"(*reg3), "=r"(*reg4)
            : "r"(raw), "0"(s1), "1"(s2), "2"(s3), "3"(s4)  : "memory"
    );
}

static void karma_set_reg_count(unsigned long * cmd, unsigned long reg_count) {
    reg_count &= 0x7;
    reg_count <<= 24;
    *cmd |= reg_count;
}

void karma_hypercall0(unsigned long cmd) {
    karma_hypercall0_vz(cmd);
}

void karma_hypercall1(unsigned long cmd, unsigned long * reg1) {
    karma_set_reg_count(&cmd, 1);
    karma_hypercall1_vz(cmd, reg1);
}

void karma_hypercall2(unsigned long cmd, unsigned long * reg1, unsigned long * reg2) {
    karma_set_reg_count(&cmd, 2);
    karma_hypercall2_vz(cmd, reg1, reg2);
}

void karma_hypercall3(unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3) {
    karma_set_reg_count(&cmd, 3);
    karma_hypercall3_vz(cmd, reg1, reg2, reg3);
}

void karma_hypercall4(unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3, unsigned long * reg4) {
    karma_set_reg_count(&cmd, 4);
    karma_hypercall4_vz(cmd, reg1, reg2, reg3, reg4);
}

/* bypass karma devices and issue hypcall to exit vm */
void karma_hypercall_exit_op(void) {
    asm volatile(
            ".set push; .set noreorder; \n"
            ASM_HYPCALL_STR(HYPCALL_MAGIC_EXIT) "\n"
            ".set pop; \n"
            : : : "memory"
    );
}
