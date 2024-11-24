#include <device.h>

device_t* device_alloc() {
    device_t* dev = malloc(sizeof(device_t));
    memset(dev, 0, sizeof(device_t));
    return dev;
}

void device_step(device_t* dev) {
    if (dev->step)
        dev->step(dev);
}

void device_free(device_t* dev) {
    if (dev->free)
        dev->free(dev);

    free(dev);
}