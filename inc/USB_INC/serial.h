#ifndef _SERIAL_H
#define _SERIAL_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "ring.h"

extern bool serial_active;
// void serial_init(void);
void serial_in(uint8_t *buf, uint16_t len);
void serial_out(void);
// int printf(const char *fmt, ...);
// int puts(const char *s);

#endif
