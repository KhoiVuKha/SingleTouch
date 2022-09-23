#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "config.h"
#include "ring.h"
#include "serial.h"
//#include "matrix.h"

static struct ring output_ring;
static struct ring input_ring;
// static uint8_t input_buffer[SERIAL_BUF_SIZEIN];
// static uint8_t output_buffer[SERIAL_BUF_SIZEOUT];
bool serial_active;
static uint32_t timer_out = 0;

//extern uint8_t show_matrix;

void serial_init()
{
//    ring_init(&output_ring, output_buffer, SERIAL_BUF_SIZEOUT);
//    ring_init(&input_ring, input_buffer, SERIAL_BUF_SIZEIN);
    serial_active = false;
}

extern bool nkro_active;

void serial_in(uint8_t *buf, uint16_t len)
{
    ring_write(&input_ring, buf, len);

    if (buf && len > 0) {
        if (buf[0] == 'n')
            nkro_active ^= 1;
//        if (buf[0] == 'm')
//            show_matrix ^= 1;
    }

    printf("nkro %d\r", nkro_active);
}

void serial_out()
{
    uint8_t *buf;
    int32_t len;
    uint32_t now = usb_now();

    if (timer_out == now)
        return;

    if (RING_EMPTY(&output_ring))
        return;

    len = ring_read_contineous(&output_ring, &buf);
    cdcacm_data_wx(buf, len);
    timer_out = now;
}
//
// static uint32_t
// itoa(int32_t value, uint32_t radix, uint32_t uppercase, uint32_t unsig,
//           char *buffer, uint32_t zero_pad)
// {
//     char *pbuffer = buffer;
//     int32_t negative = 0;
//     uint32_t i, len;
//
//     /* No support for unusual radixes. */
//     if (radix > 16)
//         return 0;
//
//     if (value < 0 && !unsig) {
//         negative = 1;
//         value = -value;
//     }
//
//     /* This builds the string back to front ... */
//     do {
//         int digit = value % radix;
//         *(pbuffer++) = (digit < 10 ? '0' + digit : (uppercase ? 'A' : 'a') + digit - 10);
//         value /= radix;
//     } while (value > 0);
//
//     for (i = (pbuffer - buffer); i < zero_pad; i++)
//         *(pbuffer++) = '0';
//
//     if (negative)
//         *(pbuffer++) = '-';
//
//     *(pbuffer) = '\0';
//
//     /* ... now we reverse it (could do it recursively but will
//      * conserve the stack space) */
//     len = (pbuffer - buffer);
//     for (i = 0; i < len / 2; i++) {
//         char j = buffer[i];
//         buffer[i] = buffer[len-i-1];
//         buffer[len-i-1] = j;
//     }
//
//     return len;
// }
//
// static int vrprintf(struct ring *ring, const char *fmt, va_list va)
// {
//     uint32_t mark = ring_mark(ring);
//     char bf[24];
//     char ch;
//
//     while ((ch = *(fmt++))) {
//         if (ch == '\n') {
//             ring_write_ch(ring, '\n');
//             ring_write_ch(ring, '\r');
//         } else if (ch != '%') {
//             ring_write_ch(ring, ch);
//         } else {
//             char zero_pad = 0;
//             char *ptr;
//             uint32_t len;
//
//             ch = *(fmt++);
//
//             /* Zero padding requested */
//             if (ch == '0') {
//                 ch = *(fmt++);
//                 if (ch == '\0')
//                     goto end;
//                 if (ch >= '0' && ch <= '9')
//                     zero_pad = ch - '0';
//                 ch = *(fmt++);
//             }
//
//             switch (ch) {
//             case 0:
//                 goto end;
//
//             case 'u':
//             case 'd':
//                 len = itoa(va_arg(va, uint32_t), 10, 0, (ch == 'u'), bf, zero_pad);
//                 ring_write(ring, (uint8_t *)bf, len);
//                 break;
//
//             case 'x':
//             case 'X':
//                 len = itoa(va_arg(va, uint32_t), 16, (ch == 'X'), 1, bf, zero_pad);
//                 ring_write(ring, (uint8_t *)bf, len);
//                 break;
//
//             case 'c' :
//                 ring_write_ch(ring, (char)(va_arg(va, int)));
//                 break;
//
//             case 's' :
//                 ptr = va_arg(va, char*);
//                 ring_write(ring, (uint8_t *)ptr, strlen(ptr));
//                 break;
//
//             default:
//                 ring_write_ch(ring, ch);
//                 break;
//             }
//         }
//     }
// end:
//     return ring_marklen(ring, mark);
// }
//
//
// int printf(const char *fmt, ...)
// {
//     int ret;
//     va_list va;
//     va_start(va, fmt);
//     ret = vrprintf(&output_ring, fmt, va);
//     va_end(va);
//
//     return ret;
//	 return 1;
// }
//
// int puts(const char *s)
// {
//     return ring_write(&output_ring, (uint8_t *)s, strlen(s));
//	 return 1;
// }
