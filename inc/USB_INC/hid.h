/*
 * USB magic numbers used in hid requests
 */
#ifndef _HID_H
#define _HID_H

/* HID Class specific bRequest */
#define USBHID_REQ_GET_REPORT    0x01
#define USBHID_REQ_GET_IDLE      0x02
#define USBHID_REQ_GET_PROTOCOL  0x03
#define USBHID_REQ_SET_REPORT    0x09
#define USBHID_REQ_SET_IDLE      0x0a
#define USBHID_REQ_SET_PROTOCOL  0x0b

#endif /* _HID_H */
