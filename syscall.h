#ifndef SYSCALL_H
#define SYSCALL_H

#include "idt.h"

/* 
 * The main C dispatcher for system calls triggered by int 0x80.
 * It takes the register state pushed by isr128 to read arguments.
 */
void syscall_dispatcher(registers_t *regs);

#endif
