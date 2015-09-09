/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License 2.  See the file "COPYING-GPL-2" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2013 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * This file is based on L4Linux linux/arch/mips/kernel/hypercall.c
 */

/*
 * hypercall.c
 *
 */

#include "asm/hypercall.h"
#include "asm/hypcall_ops.h"

void karma_hypercall0_vz(const unsigned long cmd){
    register unsigned long _cmd __asm__("s0") = cmd;
    asm volatile(
            ".set push; .set noreorder; \n"
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            ".set pop; \n"
            : : "r"(_cmd) : "memory"
    );
}
void karma_hypercall1_vz(const unsigned long cmd, unsigned long * reg1){
    register unsigned long _cmd __asm__("s0") = cmd;
    register unsigned long _reg1 __asm__("s1") = *reg1;
    asm volatile(
            ".set push; .set noreorder; \n"
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            ".set pop; \n"
            : "+r"(_reg1)
            : "r"(_cmd) : "memory"
    );
}
void karma_hypercall2_vz(const unsigned long cmd, unsigned long * reg1, unsigned long * reg2){
    register unsigned long _cmd __asm__("s0") = cmd;
    register unsigned long _reg1 __asm__("s1") = *reg1;
    register unsigned long _reg2 __asm__("s2") = *reg2;
    asm volatile(
            ".set push; .set noreorder; \n"
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            ".set pop; \n"
            : "+r"(_reg1),
              "+r"(_reg2)
            : "r"(_cmd) : "memory"
    );
}
void karma_hypercall3_vz(const unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3){
    register unsigned long _cmd __asm__("s0") = cmd;
    register unsigned long _reg1 __asm__("s1") = *reg1;
    register unsigned long _reg2 __asm__("s2") = *reg2;
    register unsigned long _reg3 __asm__("s3") = *reg3;
    asm volatile(
            ".set push; .set noreorder; \n"
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            ".set pop; \n"
            : "+r"(_reg1),
              "+r"(_reg2),
              "+r"(_reg3)
            : "r"(_cmd) : "memory"
    );
}
void karma_hypercall4_vz(const unsigned long cmd, unsigned long * reg1, unsigned long * reg2, unsigned long * reg3, unsigned long * reg4){
    register unsigned long _cmd __asm__("s0") = cmd;
    register unsigned long _reg1 __asm__("s1") = *reg1;
    register unsigned long _reg2 __asm__("s2") = *reg2;
    register unsigned long _reg3 __asm__("s3") = *reg3;
    register unsigned long _reg4 __asm__("s4") = *reg4;
    asm volatile(
            ".set push; .set noreorder; \n"
            ASM_HYPCALL_STR(HYPCALL_KARMA_DEV_OP) "\n"
            ".set pop; \n"
            : "+r"(_reg1),
              "+r"(_reg2),
              "+r"(_reg3),
              "+r"(_reg4)
            : "r"(_cmd) : "memory"
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
