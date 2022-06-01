#ifndef _EXTRAKEY_H
#define _EXTRAKEY_H

#include <stdint.h>
#include "keymap.h"
#include "usb.h"

extern uint8_t extrakey_idle;

report_extrakey_t *extrakey_report(void);
void extrakey_consumer_event(event_t *event, bool pressed);
void extrakey_system_event(event_t *event, bool pressed);

#endif
