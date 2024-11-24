#include <peripheral.h>
#include <utils.h>

void timer_free(device_t* dev) {
    free(dev->data);
}

uint32_t timer_io_bus_read(bus_link_t* link, uint32_t address) {
    device_t* dev       = bus_container_of(link->dst_bus, device_t);
    timer_data_t* locals = (timer_data_t*)dev->data;
    uint32_t data       = 0;

    switch (address) {
        case 0x00:
            data = locals->counter;
            break;

        case 0x01:
            data = locals->divider;
            break;
    }

    return data;
}

void timer_io_bus_write(bus_link_t* link, uint32_t address, uint32_t data) {
    device_t* dev       = bus_container_of(link->dst_bus, device_t);
    timer_data_t* locals = (timer_data_t*)dev->data;

    switch (address) {
        case 0x00:
            break;

        case 0x01:
            locals->divider = data;
            break;
    }
}

void timer_step(device_t* dev) {
    timer_data_t* locals = (timer_data_t*)dev->data;

    double now = get_time_in_nanoseconds();

    if ((now - locals->last_tick) >= 10000000) {
        locals->counter++;
        locals->last_tick = now;
    }

    if (locals->counter >= locals->divider && locals->divider > 0) {
        locals->counter = 0;
        bus_io_pin_write(&dev->bus, "irq", 1);
    }
}

bus_entry_t timer_bus_entries[] = {
    {
        .name   = "bus",
        .type   = BUS_TYPE_BUS,

        .address_width  = 16,
        .data_width     = 8,

        .io_bus_read    = timer_io_bus_read,
        .io_bus_write   = timer_io_bus_write,
    },
    {
        .name   = "irq",
        .type   = BUS_TYPE_PIN,

        .io_pin_read  = NULL,
        .io_pin_write = NULL,
    }
};

void timer_init(device_t* dev) {
    timer_data_t* locals = (timer_data_t*)malloc(sizeof(timer_data_t));
    memset(locals, 0, sizeof(timer_data_t));

    locals->divider = 32;

    dev->data       = (void*)locals;

    dev->step       = timer_step;
    dev->free       = timer_free;

    dev->bus.entries     = timer_bus_entries;
    dev->bus.entry_count = sizeof(timer_bus_entries) / sizeof(bus_entry_t);
    dev->bus.size        = 2;
}