#include <peripheral.h>

void mem_free(device_t* dev) {
    if (!dev)
        return;

    if (dev->data)
        free(dev->data);

    free(dev);
}

uint32_t mem_io_bus_read(bus_link_t* link, uint32_t address) {
    device_t* dev       = bus_container_of(link->dst_bus, device_t);
    uint8_t* mem        = (uint8_t*)dev->data;

    return mem[address];
}

void mem_io_bus_write(bus_link_t* link, uint32_t address, uint32_t data) {
    device_t* dev       = bus_container_of(link->dst_bus, device_t);
    uint8_t* mem        = (uint8_t*)dev->data;

    mem[address] = data;
}

bus_entry_t mem_ram_bus_entries[] = {
    {
        .name   = "bus",
        .type   = BUS_TYPE_BUS,

        .address_width  = 16,
        .data_width     = 8,

        .io_bus_read    = mem_io_bus_read,
        .io_bus_write   = mem_io_bus_write,
    }
};

bus_entry_t mem_rom_bus_entries[] = {
    {
        .name   = "bus",
        .type   = BUS_TYPE_BUS,

        .address_width  = 16,
        .data_width     = 8,

        .io_bus_read    = mem_io_bus_read,
        .io_bus_write   = NULL,
    }
};

void mem_init(device_t* dev, uint32_t size, bool is_rom) {
    dev->data = malloc(size);

    memset(dev->data, 0, size);

    dev->bus.entries        = is_rom ? mem_rom_bus_entries : mem_ram_bus_entries;
    dev->bus.entry_count    = is_rom ? sizeof(mem_rom_bus_entries) : sizeof(mem_ram_bus_entries);
    dev->bus.entry_count   /= sizeof(bus_entry_t);
    dev->bus.size           = size;

    dev->step       = NULL;
    dev->free       = mem_free;
}