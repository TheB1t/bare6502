#include <peripheral.h>

void acia_free(device_t* dev) {
    acia_data_t* locals = (acia_data_t*)dev->data;

    ringbuf_free(locals->rx_ringbuf);
    ringbuf_free(locals->tx_ringbuf);

    close(locals->pty_master);
    free(dev->data);
}

uint32_t acia_io_bus_read(bus_link_t* link, uint32_t address) {
    device_t* dev       = bus_container_of(link->dst_bus, device_t);
    acia_data_t* locals = (acia_data_t*)dev->data;
    uint32_t data       = 0;

    switch (address) {
        case 0x00:
            ringbuf_read(locals->rx_ringbuf, (uint8_t*)&data, 1);

            if (locals->status.irq)
                locals->status.irq = 0;
            break;

        case 0x01:
            data = locals->status.raw;
            break;

        case 0x02:
            data = locals->command.raw;
            break;

        case 0x03:
            break;
    }

    return data;
}

void acia_io_bus_write(bus_link_t* link, uint32_t address, uint32_t data) {
    device_t* dev       = bus_container_of(link->dst_bus, device_t);
    acia_data_t* locals = (acia_data_t*)dev->data;

    switch (address) {
        case 0x00:
            ringbuf_write(locals->tx_ringbuf, (uint8_t*)&data, 1);
            break;

        case 0x01:
            locals->status.raw = data;
            break;

        case 0x02:
            locals->command.raw = data;
            break;

        case 0x03:
            break;
    }
}

void acia_step(device_t* dev) {
    acia_data_t* locals = (acia_data_t*)dev->data;
    uint8_t data = 0;

    ssize_t n = read(locals->pty_master, (uint8_t*)&data, 1);
    if (n > 0)
        ringbuf_write(locals->rx_ringbuf, (uint8_t*)&data, 1);

    if (ringbuf_count(locals->tx_ringbuf) > 0) {
        ringbuf_read(locals->tx_ringbuf, (uint8_t*)&data, 1);
        write(locals->pty_master, (char*)&data, 1);
    }

    locals->status.rdrf = ringbuf_count(locals->rx_ringbuf) > 0;
    locals->status.tdre = ringbuf_count(locals->tx_ringbuf) == 0;

    if (!locals->command.ird && locals->status.rdrf)
        locals->status.irq = 1;

    if (locals->status.irq)
        bus_io_pin_write(&dev->bus, "irq", 1);
}

bus_entry_t acia_bus_entries[] = {
    {
        .name   = "bus",
        .type   = BUS_TYPE_BUS,

        .address_width  = 16,
        .data_width     = 8,

        .io_bus_read    = acia_io_bus_read,
        .io_bus_write   = acia_io_bus_write,
    },
    {
        .name   = "irq",
        .type   = BUS_TYPE_PIN,

        .io_pin_read  = NULL,
        .io_pin_write = NULL,
    }
};

void acia_init(device_t* dev) {
    acia_data_t* locals = (acia_data_t*)malloc(sizeof(acia_data_t));
    memset(locals, 0, sizeof(acia_data_t));

    dev->data = (void*)locals;

    dev->step       = acia_step;
    dev->free       = acia_free;

    dev->bus.entries     = acia_bus_entries;
    dev->bus.entry_count = sizeof(acia_bus_entries) / sizeof(bus_entry_t);
    dev->bus.size        = 4;

    locals->rx_ringbuf = ringbuf_create(64);
    locals->tx_ringbuf = ringbuf_create(64);

    if (openpty(&locals->pty_master, &locals->pty_slave, locals->pty_name, NULL, NULL) == -1)
        exit(1);

    fcntl(locals->pty_master, F_SETFL, O_NONBLOCK);
}