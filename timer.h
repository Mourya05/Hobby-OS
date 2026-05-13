#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint32_t tick_count;
void init_timer(uint32_t frequency);

#endif
