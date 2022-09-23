#ifndef _USB_H
#define _USB_H
#include <libopencm3/usb/usbd.h>

/*
 * USB Configuration:
 *
 * - 4 interfaces that carry hid endpoints
 *   - 1 endpoint boot keyboard
 *   - 1 endpoint boot mouse
 *   - 1 endpoint extra keys (system control/application keys)
 *   - 1 endpoint nkro keyboard
 * - 3 interfaces for cdc acm definition
 *   - 1 endpoint for communication interrupts
 *   - 1 endpoint for bulk data send
 *   - 1 endpoint for bulk data receive
 *
 * The usb interfaces, endpoints and string indexes are used in the
 * configuration. Internal consistency is easy to distort + magic numbers in
 * code are a horror. Here they are apriori:
 */

#define IF_KEYBOARD                             0
#define IF_MOUSE                                1
#define IF_EXTRAKEY                             2
#define IF_NKRO                                 3
#define IF_SERIALCOMM                           4
#define IF_SERIALDATA                           5
#define IF_MAX                                  6

#define EP_KEYBOARD                             1
#define EP_MOUSE                                2
#define EP_EXTRAKEY                             3
#define EP_NKRO                                 4
#define EP_SERIALCOMM                           5
#define EP_SERIALDATAIN                         6
#define EP_SERIALDATAOUT                        7

#define EP_SIZE_KEYBOARD                        8
#define EP_SIZE_MOUSE                           5
#define EP_SIZE_EXTRAKEY                        3
#define EP_SIZE_NKRO                            29

#define EP_SIZE_SERIALCOMM                      16
#define EP_SIZE_SERIALDATAIN                    64
#define EP_SIZE_SERIALDATAOUT                   32

#define STRI_MANUFACTURER                       1
#define STRI_PRODUCT                            2
#define STRI_SERIAL                             3
#define STRI_KEYBOARD                           4
#define STRI_MOUSE                              5
#define STRI_EXTRAKEY                           6
#define STRI_NKRO                               7
#define STRI_COMMAND                            8
#define STRI_MAX                                8

#define REPORTID_SYSTEM                         1
#define REPORTID_CONSUMER                       2

#define SEND_RETRIES                           10

/*
 * STM32F1 requires data buffers to be at an 8 byte boundary. Ensure that
 * EP_SIZEs are aligned that way using this macro
 */
#define EP_SIZE_ALIGN(x) ((x + 0b111) & ~0b111)

typedef union {
    uint8_t raw[EP_SIZE_KEYBOARD];
    struct {
        uint8_t mods;
        uint8_t reserved;
        uint8_t keys[EP_SIZE_KEYBOARD - 2];
    };
} __attribute__ ((packed)) report_keyboard_t;

typedef union {
    uint8_t raw[EP_SIZE_MOUSE];
    struct {
        uint8_t buttons;
        int8_t x;
        int8_t y;
        int8_t v;
        int8_t h;
    };
} __attribute__ ((packed)) report_mouse_t;

typedef union {
    uint8_t raw[EP_SIZE_EXTRAKEY];
    struct {
        uint8_t id;
        uint8_t codel;
        uint8_t codeh;
    };
} __attribute__ ((packed)) report_extrakey_t;

typedef union {
    uint8_t raw[EP_SIZE_NKRO];
    struct {
        uint8_t mods;
        uint8_t bits[EP_SIZE_NKRO - 1];
    };
} __attribute__ ((packed)) report_nkro_t;


extern volatile uint32_t usb_ms;

void usb_init(void);
void usb_poll(void);
uint32_t usb_now(void);

void usb_update_keyboard(report_keyboard_t *);
void usb_update_mouse(report_mouse_t *);
void usb_update_extrakey(report_extrakey_t *);
void usb_update_nkro(report_nkro_t *);

void cdcacm_data_rx_cb(usbd_device *dev, uint8_t ep);
void cdcacm_data_wx(uint8_t *buf, uint16_t len);

#endif
