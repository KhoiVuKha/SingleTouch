#ifndef _RING_H
#define _RING_H

#include <stdint.h>

typedef int32_t ring_size_t;

typedef struct ring {
	uint8_t *data;
	ring_size_t size;
	uint32_t begin;
	uint32_t end;
} ring_t;

#define RING_SIZE(RING)  ((RING)->size - 1)
#define RING_DATA(RING)  (RING)->data
#define RING_EMPTY(RING) ((RING)->begin == (RING)->end)

void ring_init(ring_t *ring, uint8_t *buf, ring_size_t size);
int32_t ring_write_ch(ring_t *ring, uint8_t ch);
int32_t ring_write(ring_t *ring, uint8_t *data, ring_size_t size);
int32_t ring_read_ch(ring_t *ring, uint8_t *ch);
int32_t ring_read(ring_t *ring, uint8_t *data, ring_size_t size);
int32_t ring_read_contineous(ring_t *ring, uint8_t **data);
uint32_t ring_mark(ring_t *ring);
uint32_t ring_marklen(ring_t *ring, uint32_t mark);
#endif
