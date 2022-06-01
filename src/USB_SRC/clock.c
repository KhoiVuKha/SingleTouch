/*
 * clock
 *
 * Millisecond system clock that counts up incrementally.
 */
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>

#include "clock.h"

static volatile uint32_t system_ms = 0;

void
sys_tick_handler(void)
{
    system_ms++;
}

void
clock_init(void)
{
    systick_set_reload(rcc_ahb_frequency / 1000);
    STK_CVR = 0;
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();
}

uint32_t
clock_now(void)
{
    return system_ms;
}

uint32_t
timer_set(uint32_t delay)
{
    return system_ms + delay;
}

bool
timer_passed(uint32_t timer)
{
    return (timer < system_ms);
}
