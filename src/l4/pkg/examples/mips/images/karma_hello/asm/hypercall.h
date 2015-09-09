/*
 * hypercall.h
 *
 *  Created on: 11.05.2011
 *      Author: janis
 */

#ifndef HYPERCALL_H_
#define HYPERCALL_H_

struct karma_hypercall_s{
    void (*hc_args0)(const unsigned long);
    void (*hc_args1)(const unsigned long, unsigned long *);
    void (*hc_args2)(const unsigned long, unsigned long *, unsigned long *);
    void (*hc_args3)(const unsigned long, unsigned long *, unsigned long *, unsigned long *);
    void (*hc_args4)(const unsigned long, unsigned long *, unsigned long *, unsigned long *, unsigned long *);
};

extern struct karma_hypercall_s karma_hypercall_vz;
extern struct karma_hypercall_s * karma_hypercall;

void karma_hypercall_init(void);
void karma_hypercall_exit_op(void);

#endif /* HYPERCALL_H_ */
