/*
 * mouse
 *
 * Receive mouse events and process them into usb communication with the
 * host.
 */

#include "mouse.h"

static report_mouse_t state;
uint8_t mouse_idle = 0;

report_mouse_t *
mouse_report()
{
    return &state;
}

void
mouse_event(event_t *event, bool pressed)
{
    if (pressed) {
        state.buttons = event->mouse.button;
        state.x = event->mouse.x;
        state.y = event->mouse.y;
        state.h = state.v = 0;
        usb_update_mouse(&state);
    }
}

void
wheel_event(event_t *event, bool pressed)
{
    if (pressed) {
        state.buttons = event->wheel.button;
        state.h = event->wheel.h;
        state.v = event->wheel.v;
        state.x = state.y = 0;
        usb_update_mouse(&state);
    }
}
