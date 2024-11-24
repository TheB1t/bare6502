#include <ringbuf.h>

ringbuf_t* ringbuf_create(uint32_t size) {
    ringbuf_t* ringbuf = (ringbuf_t*)malloc(sizeof(ringbuf_t));

    ringbuf->start  = (uint8_t*)malloc(size);
    ringbuf->end    = ringbuf->start + size;
    ringbuf->write  = ringbuf->start;
    ringbuf->read   = ringbuf->start;

    return ringbuf;
}

void ringbuf_free(ringbuf_t* ringbuf) {
    free(ringbuf->start);
    free(ringbuf);
}

uint32_t ringbuf_size(ringbuf_t* ringbuf) {
    return ringbuf->end - ringbuf->start;
}

uint32_t ringbuf_count(ringbuf_t* ringbuf) {
    if (ringbuf->write >= ringbuf->read)
        return ringbuf->write - ringbuf->read;

    return (uint32_t)(ringbuf->end - ringbuf->read + ringbuf->write);
}

uint32_t ringbuf_write(ringbuf_t* ringbuf, uint8_t* data, uint32_t size) {
    uint32_t count = ringbuf_count(ringbuf);

    if (count + size > ringbuf_size(ringbuf))
        return 0;

    uint32_t write_size = size;
    if (ringbuf->write + size > ringbuf->end) {
        uint32_t first_size = ringbuf->end - ringbuf->write;
        memcpy(ringbuf->write, data, first_size);
        ringbuf->write = ringbuf->start;
        memcpy(ringbuf->write, data + first_size, size - first_size);
        ringbuf->write += size - first_size;
    } else {
        memcpy(ringbuf->write, data, size);
        ringbuf->write += size;
        if (ringbuf->write == ringbuf->end)
            ringbuf->write = ringbuf->start;
    }

    return write_size;
}

uint32_t ringbuf_read(ringbuf_t* ringbuf, uint8_t* data, uint32_t size) {
    uint32_t count = ringbuf_count(ringbuf);

    if (count < size)
        return 0;

    uint32_t read_size = size;
    if (ringbuf->read + size > ringbuf->end) {
        uint32_t first_size = ringbuf->end - ringbuf->read;
        memcpy(data, ringbuf->read, first_size);
        ringbuf->read = ringbuf->start;
        memcpy(data + first_size, ringbuf->read, size - first_size);
        ringbuf->read += size - first_size;
    } else {
        memcpy(data, ringbuf->read, size);
        ringbuf->read += size;
        if (ringbuf->read == ringbuf->end)
            ringbuf->read = ringbuf->start;
    }

    return read_size;
}