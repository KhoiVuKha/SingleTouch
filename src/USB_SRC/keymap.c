/*
 * Keymap
 *
 * Holds the keymap and code to take on press and release events from the
 * matrix. These can then be passed on to the keyboard, mouse or extrakey
 * modules.
 */
#include <stdint.h>
#include "keymap.h"
#include "usb_keycode.h"
#include "config.h"

#include "keyboard.h"
#include "mouse.h"
#include "extrakey.h"

event_t keymap[][ROWS_NUM][COLS_NUM] =
{
    {
        { _K(F13), _K(F14), _K(F15), _K(F16), _K(F17) },
        { _K(F18), _K(F19), _K(F20), _K(F21), _K(F22) },
        { _C(VOLUMEINC), _C(VOLUMEDEC), _C(PLAY), _C(FASTFORWARD), _C(MUTE) },
        { _K(PAD_1), _K(PAD_2), _K(PAD_3), _K(PAD_4), _K(PAD_5) },
        { _K(PAD_6), _K(PAD_7), _K(PAD_8), _K(PAD_9), _K(PAD_0) }
    },
};

static uint8_t layer = 0;

void
keymap_event(uint16_t row, uint16_t col, bool pressed)
{
    event_t *event = &keymap[layer][row][col];

    switch (event->type) {
    case KMT_KEY:
        keyboard_event(event, pressed);
        break;

    case KMT_MOUSE:
        mouse_event(event, pressed);
        break;

    case KMT_WHEEL:
        wheel_event(event, pressed);
        break;

    case KMT_CONSUMER:
        extrakey_consumer_event(event, pressed);
        break;

    case KMT_SYSTEM:
        extrakey_system_event(event, pressed);
        break;

    case KMT_LAYER:
        layer = event->layer.number % (sizeof(keymap));
        break;
    }
}
