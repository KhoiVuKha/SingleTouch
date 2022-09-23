#ifndef _LOG_H
#define _LOG_H

#include <stdint.h>
#include "serial.h"
#include <stdio.h>

void elog_start(const char *name, uint16_t line);

#define elog elog_start(__FILE__, __LINE__), printf

#endif
