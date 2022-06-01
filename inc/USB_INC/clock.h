#ifndef _CLOCK_H
#define _CLOCK_H

#include <stdbool.h>
#include <stdint.h>

void clock_init(void);
uint32_t clock_now(void);
uint32_t timer_set(uint32_t delay);
bool timer_passed(uint32_t timer);

#endif
