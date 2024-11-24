#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct ringbuf  ringbuf_t;

struct ringbuf {
    uint8_t*    start;
    uint8_t*    end;
    uint8_t*    write;
    uint8_t*    read;
};

extern ringbuf_t* ringbuf_create(uint32_t size);
extern void ringbuf_free(ringbuf_t* ringbuf);
extern uint32_t ringbuf_size(ringbuf_t* ringbuf);
extern uint32_t ringbuf_count(ringbuf_t* ringbuf);
extern uint32_t ringbuf_write(ringbuf_t* ringbuf, uint8_t* data, uint32_t size);
extern uint32_t ringbuf_read(ringbuf_t* ringbuf, uint8_t* data, uint32_t size);