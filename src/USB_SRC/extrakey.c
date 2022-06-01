/*
 * Extrakey
 *
 * Receive extrakey events and process them into usb communication with the
 * host.
 */

#include "elog.h"
#include "extrakey.h"

uint8_t extrakey_idle = 0;

static report_extrakey_t state;

report_extrakey_t *
extrakey_report()
{
    return &state;
}

void
extrakey_consumer_event(event_t *event, bool pressed)
{
    state.id = REPORTID_CONSUMER;

    elog("extrakey consumer %04x %d\n", event->extra.code, pressed);

    if (pressed) {
        state.codel = event->extra.code & 0xff;
        state.codeh = event->extra.code >> 8;
    } else {
        state.codel = state.codeh = 0;
    }

    usb_update_extrakey(&state);
}

void
extrakey_system_event(event_t *event, bool pressed)
{
    state.id = REPORTID_SYSTEM;

    elog("extrakey consumer %04x %d\n", event->extra.code, pressed);

    if (pressed) {
        state.codel = event->extra.code & 0xff;
        state.codeh = event->extra.code >> 8;
    } else {
        state.codel = state.codeh = 0;
    }

    usb_update_extrakey(&state);
}
