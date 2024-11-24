#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <bus.h>

#define DEVICE_MAX_LINKS 6

typedef struct device           device_t;

struct device {
    void*       data;

    bus_t       bus;

    void        (*step)(device_t* dev);
    void        (*free)(device_t* dev);
};

extern device_t*    device_alloc();
extern void         device_step(device_t* dev);
extern void         device_free(device_t* dev);