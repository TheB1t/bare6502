#include <bus.h>

bus_entry_type_e bus_name_to_field_type(const char* name) {
    if (strcmp(name, "bus") == 0)
        return BUS_TYPE_BUS;

    if (strcmp(name, "pin") == 0)
        return BUS_TYPE_PIN;

    return BUS_TYPE_MAX;
}

uint32_t bus_io_bus_read(bus_t* bus, const char* name, uint32_t address) {
    for (int i = 0; i < BUS_MAX_LINKS; i++) {
        bus_link_t* link = &bus->links[i];

        if (!link->dst || !link->dst->io_bus_read)
            continue;

        if (strcmp(link->src->name, name) != 0)
            continue;

        if (address < link->base || address >= link->base + link->dst_bus->size)
            continue;

        return link->dst->io_bus_read(link, address - link->base);
    }

    return 0;
}

void bus_io_bus_write(bus_t* bus, const char* name, uint32_t address, uint32_t data) {
    for (int i = 0; i < BUS_MAX_LINKS; i++) {
        bus_link_t* link = &bus->links[i];

        if (!link->dst || !link->dst->io_bus_write)
            continue;

        if (strcmp(link->src->name, name) != 0)
            continue;

        if (address < link->base || address >= link->base + link->dst_bus->size)
            continue;


        link->dst->io_bus_write(link, address - link->base, data);
        return;
    }
}

uint32_t bus_io_pin_read(bus_t* bus, const char* name) {
    for (int i = 0; i < BUS_MAX_LINKS; i++) {
        bus_link_t* link = &bus->links[i];

        if (!link->dst || !link->dst->io_pin_read)
            continue;

        if (strcmp(link->src->name, name) != 0)
            continue;

        return link->dst->io_pin_read(link);
    }

    return 0;
}

void bus_io_pin_write(bus_t* bus, const char* name, uint32_t data) {
    for (int i = 0; i < BUS_MAX_LINKS; i++) {
        bus_link_t* link = &bus->links[i];

        if (!link->dst || !link->dst->io_pin_write)
            continue;

        if (strcmp(link->src->name, name) != 0)
            continue;

        link->dst->io_pin_write(link, data);
        return;
    }
}

bus_link_t* bus_find_free_link(bus_t* bus) {
    for (int i = 0; i < BUS_MAX_LINKS; i++) {
        if (bus->links[i].dst)
            continue;

        return &bus->links[i];
    }

    return NULL;
}

bus_entry_t* bus_find_entry_by_name(bus_t* bus, const char* name) {
    for (int i = 0; i < bus->entry_count; i++) {
        bus_entry_t* entry = &bus->entries[i];

        if (entry->type == BUS_TYPE_DISABLED)
            continue;

        if (strcmp(entry->name, name) == 0)
            return entry;
    }

    return NULL;
}

bus_link_t* bus_attach_by_name(bus_t* bus0, bus_t* bus1, const char* name0, const char* name1) {
    bus_entry_t* entry0 = bus_find_entry_by_name(bus0, name0);
    bus_entry_t* entry1 = bus_find_entry_by_name(bus1, name1);

    if (!entry0 || !entry1)
        return NULL;

    bus_link_t* link = bus_find_free_link(bus0);

    if (!link)
        return NULL;

    link->src = entry0;
    link->dst = entry1;

    link->src_bus = bus0;
    link->dst_bus = bus1;

    return link;
}

int32_t bus_detach_by_name(bus_t* bus0, bus_t* bus1, const char* name0, const char* name1) {
    bus_entry_t* entry0 = bus_find_entry_by_name(bus0, name0);
    bus_entry_t* entry1 = bus_find_entry_by_name(bus1, name1);

    if (!entry0 || !entry1)
        return -1;

    for (int i = 0; i < BUS_MAX_LINKS; i++) {
        bus_link_t* link = &bus0->links[i];

        if (!link->dst)
            continue;

        if (link->src != entry0 || link->src_bus != bus0)
            continue;

        if (link->dst != entry1 || link->dst_bus != bus1)
            continue;

        link->src = NULL;
        link->dst = NULL;

        link->src_bus = NULL;
        link->dst_bus = NULL;

        return 0;
    }

    return -1;
}