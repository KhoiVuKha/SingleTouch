#ifndef _MOUSE_H
#define _MOUSE_H

#include <stdint.h>
#include "keymap.h"
#include "usb.h"

extern uint8_t mouse_idle;

report_mouse_t *mouse_report(void);
void mouse_event(event_t *event, bool pressed);
void wheel_event(event_t *event, bool pressed);

#endif
