#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <stdint.h>
#include "keymap.h"
#include "usb.h"

extern bool keyboard_active;
extern uint8_t keyboard_idle;

// Phan them de in ra kiem tra
extern bool keyboard_dirty;
extern bool nkro_dirty;
report_keyboard_t keyboard_state;
report_nkro_t nkro_state;

void keyboard_set_protocol(uint8_t protocol);
uint8_t *keyboard_get_protocol(void);
report_keyboard_t *keyboard_report(void);
void keyboard_event(event_t *event, bool pressed);
void keyboard_add_key(uint8_t key);
void keyboard_del_key(uint8_t key);
void keyboard_set_leds(uint8_t leds);
void keyboard_add_modifier(uint8_t modifier);
void keyboard_del_modifier(uint8_t modifier);

extern uint8_t nkro_idle;
report_nkro_t *nkro_report(void);

#endif
