/*
 * A keymap consists of <layers> * <rows> * <colums> of scanmap_t.
 *
 *
 * |87654321|87654321|87654321|87654321|
 * |--------+--------+--------+--------|
 * |type    |data1   |data2   |data3   |
 * |    0000|        |keymod  |scancode|
 * |    0001|        |extra key        |
 * |    0010|buttons |x       |y       |
 * |    0011|buttons |h       |v       |
 * |    0100|layer select              |
 * |--------+--------+--------+--------|
 */

#ifndef _KEYMAP_H
#define _KEYMAP_H
#include "config.h"

typedef struct {
    uint8_t type;
    union {
        uint8_t data[3];
        struct {
            uint8_t empty1;
            uint8_t mod;
            uint8_t code;
        } __attribute__ ((packed)) key;
        struct {
            uint8_t empty2;
            uint16_t code;
        } __attribute__ ((packed)) extra;
        struct {
            uint8_t button;
            int8_t x;
            int8_t y;
        } __attribute__ ((packed)) mouse;
        struct {
            uint8_t button;
            int8_t h;
            int8_t v;
        } __attribute__ ((packed)) wheel;
        struct {
            uint8_t empty3;
            uint8_t empty4;
            uint8_t number;
        } __attribute__ ((packed)) layer;
    };
} __attribute__ ((packed)) event_t;

extern event_t keymap[][ROWS_NUM][COLS_NUM];

enum {
    KMT_KEY,
    KMT_LAYER,
    KMT_CONSUMER,
    KMT_SYSTEM,
    KMT_MOUSE,
    KMT_WHEEL
};

#define _K(Key)        {.type = KMT_KEY, .key = { .empty1 = 0, .mod = 0, .code = KEY_##Key }}
#define _KC(Code)      {.type = KMT_KEY, .key = { .empty1 = 0, .mod = 0, .code = 0x##Code }}
#define _S(Mod)        {.type = KMT_KEY, .key = { .empty1 = 0, .code = 0, .mod = Mod }}
#define _M(X,Y)        {.type = KMT_MOUSE, .mouse = {.button = 0, .x = X, .y = Y }}
#define _B(Button)     {.type = KMT_MOUSE, .mouse = {.button = Button, .x = 0, .y = 0}}
#define _W(H,V)        {.type = KMT_WHEEL, .wheel = {.button = 0, .h = H, .v = V }}
#define _C(Key)        {.type = KMT_CONSUMER, .extra = { .empty2 = 0, .code = CONSUMER_##Key }}
#define _Y(Key)        {.type = KMT_SYSTEM, .extra = { .empty2 = 0, .code = SYSTEM_##Key }}
#define _L(Layer)      {.type = KMT_LAYER, .layer = { .number = Layer }}

void keymap_event(uint16_t row, uint16_t col, bool pressed);

#endif
