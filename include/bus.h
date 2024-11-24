#pragma once

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define bus_container_of(ptr, type) \
    ((type *)((char *)(ptr) - offsetof(type, bus)))

#define BUS_MAX_LINKS 16

typedef struct  bus             bus_t;
typedef struct  bus_entry       bus_entry_t;
typedef struct  bus_link        bus_link_t;
typedef enum    bus_entry_type  bus_entry_type_e;

enum bus_entry_type {
    BUS_TYPE_DISABLED,
    BUS_TYPE_BUS,
    BUS_TYPE_PIN,
    BUS_TYPE_MAX,
};

struct bus_link {
    bus_entry_t*  src;
    bus_entry_t*  dst;

    bus_t*        src_bus;
    bus_t*        dst_bus;

    // Used for BUS_TYPE_BUS
    uint32_t      base;
};

struct bus_entry {
    char                name[16];
    bus_entry_type_e    type;

    // Using only for BUS_TYPE_BUS
    uint32_t            address_width;
    uint32_t            data_width;

    union {
        uint32_t    (*io_bus_read)(bus_link_t* entry, uint32_t address);
        uint32_t    (*io_pin_read)(bus_link_t* entry);
    };

    union {
        void        (*io_bus_write)(bus_link_t* entry, uint32_t address, uint32_t data);
        void        (*io_pin_write)(bus_link_t* entry, uint32_t data);
    };
};

struct bus {
    char                name[16];

    // Used for BUS_TYPE_BUS
    uint32_t            size;

    bus_entry_t*        entries;
    uint32_t            entry_count;

    bus_link_t          links[BUS_MAX_LINKS];
};

extern bus_entry_type_e bus_name_to_type(const char* name);

extern bus_entry_t*    bus_find_entry_by_name(bus_t* bus, const char* name);
extern bus_entry_t*    bus_find_entry_by_type(bus_t* bus, bus_entry_type_e type);

extern uint32_t        bus_io_bus_read(bus_t* bus, const char* name, uint32_t address);
extern void            bus_io_bus_write(bus_t* bus, const char* name, uint32_t address, uint32_t data);

extern uint32_t        bus_io_pin_read(bus_t* bus, const char* name);
extern void            bus_io_pin_write(bus_t* bus, const char* name, uint32_t data);

extern bus_link_t*     bus_attach_by_name(bus_t* bus0, bus_t* bus1, const char* name0, const char* name1);
extern int32_t         bus_detach_by_name(bus_t* bus0, bus_t* bus1, const char* name0, const char* name1);