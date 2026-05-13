#ifndef TASK_H
#define TASK_H

#include <stdint.h>

typedef struct task {
    int id;                // Process ID
    uint32_t esp, ebp;     // Stack and base pointers
    uint32_t eip;          // Instruction pointer
    uint32_t page_directory; // Page directory for this task
    struct task *next;     // The next task in a linked list
} task_t;

extern void switch_to_user_mode(void);

#endif
