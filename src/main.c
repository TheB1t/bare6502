#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <toml.h>
#include <bare6502.h>
#include <peripheral.h>
#include <utils.h>

#define CONFIG_NAME_MAX      16
#define CONFIG_PATH_MAX      256
#define CONFIG_MAX_DEVICES   16
#define CONFIG_MAX_CHIPS     16
#define CONFIG_MAX_LINKS     128

typedef enum config_link_type {
    CONFIG_LINK_TYPE_BUS,
    CONFIG_LINK_TYPE_PIN,
} config_link_type_e;

typedef struct parsed_device {
    char        name[CONFIG_NAME_MAX];
    char        type[CONFIG_NAME_MAX];
    bool        has_type;

    bool        is_rom;
    bool        has_is_rom;

    uint32_t    size;
    bool        has_size;

    char        load[CONFIG_PATH_MAX];
    bool        has_load;
} parsed_device_t;

typedef struct parsed_chip {
    char        name[CONFIG_NAME_MAX];
    char        type[CONFIG_NAME_MAX];
    bool        has_type;

    uint32_t    reset;
    bool        has_reset;
} parsed_chip_t;

typedef struct parsed_link {
    config_link_type_e   type;
    char                 src_bus[CONFIG_NAME_MAX];
    char                 src_entry[CONFIG_NAME_MAX];
    char                 dst_bus[CONFIG_NAME_MAX];
    uint32_t             base;
} parsed_link_t;

typedef struct parsed_config {
    parsed_device_t   devices[CONFIG_MAX_DEVICES];
    uint32_t          device_count;

    parsed_chip_t     chips[CONFIG_MAX_CHIPS];
    uint32_t          chip_count;

    parsed_link_t     links[CONFIG_MAX_LINKS];
    uint32_t          link_count;
} parsed_config_t;

bare6502_t* chips[16]   = { 0 };
device_t*   devices[16] = { 0 };
bus_t*      buses[32]   = { 0 };

uint32_t    next_chip = 0;
uint32_t    next_device = 0;
uint32_t    next_bus = 0;
volatile sig_atomic_t terminate_requested = 0;

static void handle_sigterm(int signum) {
    (void)signum;
    terminate_requested = 1;
}

static int32_t setup_signal_handlers(void) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));

    action.sa_handler = handle_sigterm;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGTERM, &action, NULL) != 0) {
        perror("sigaction(SIGTERM)");
        return 1;
    }

    return 0;
}

static int32_t set_name(char dst[CONFIG_NAME_MAX], const char* src, const char* field_name) {
    size_t len = strlen(src);
    if (len >= CONFIG_NAME_MAX) {
        printf("%s '%s' is too long (max %d)\n", field_name, src, CONFIG_NAME_MAX - 1);
        return 1;
    }

    memset(dst, 0, CONFIG_NAME_MAX);
    memcpy(dst, src, len);
    return 0;
}

static int32_t parse_u32_literal(const char* value, uint32_t* out) {
    char* end = NULL;
    unsigned long parsed;

    if (!value || !out)
        return 1;

    errno = 0;
    parsed = strtoul(value, &end, 0);
    if (errno != 0 || !end || *end != '\0' || parsed > UINT32_MAX)
        return 1;

    *out = (uint32_t)parsed;
    return 0;
}

static int32_t toml_read_required_string(toml_table_t* tab, const char* key, char* out, size_t out_size, const char* scope) {
    toml_datum_t value;

    if (!toml_key_exists(tab, key)) {
        printf("Missing key '%s' in %s\n", key, scope);
        return 1;
    }

    value = toml_string_in(tab, key);
    if (!value.ok) {
        printf("Key '%s' in %s must be string\n", key, scope);
        return 1;
    }

    if (strlen(value.u.s) >= out_size) {
        printf("Value of '%s' in %s is too long\n", key, scope);
        free(value.u.s);
        return 1;
    }

    memset(out, 0, out_size);
    memcpy(out, value.u.s, strlen(value.u.s));
    free(value.u.s);
    return 0;
}

static int32_t toml_read_optional_string(toml_table_t* tab, const char* key, char* out, size_t out_size, bool* present, const char* scope) {
    toml_datum_t value;

    *present = false;

    if (!toml_key_exists(tab, key))
        return 0;

    value = toml_string_in(tab, key);
    if (!value.ok) {
        printf("Key '%s' in %s must be string\n", key, scope);
        return 1;
    }

    if (strlen(value.u.s) >= out_size) {
        printf("Value of '%s' in %s is too long\n", key, scope);
        free(value.u.s);
        return 1;
    }

    memset(out, 0, out_size);
    memcpy(out, value.u.s, strlen(value.u.s));
    free(value.u.s);

    *present = true;
    return 0;
}

static int32_t toml_read_optional_bool(toml_table_t* tab, const char* key, bool* out, bool* present, const char* scope) {
    toml_datum_t value;

    *present = false;

    if (!toml_key_exists(tab, key))
        return 0;

    value = toml_bool_in(tab, key);
    if (!value.ok) {
        printf("Key '%s' in %s must be bool\n", key, scope);
        return 1;
    }

    *out = value.u.b ? true : false;
    *present = true;
    return 0;
}

static int32_t toml_read_optional_u32(toml_table_t* tab, const char* key, uint32_t* out, bool* present, const char* scope) {
    toml_datum_t int_value;
    toml_datum_t str_value;

    *present = false;

    if (!toml_key_exists(tab, key))
        return 0;

    int_value = toml_int_in(tab, key);
    if (int_value.ok) {
        if (int_value.u.i < 0 || int_value.u.i > UINT32_MAX) {
            printf("Key '%s' in %s is out of uint32 range\n", key, scope);
            return 1;
        }

        *out = (uint32_t)int_value.u.i;
        *present = true;
        return 0;
    }

    str_value = toml_string_in(tab, key);
    if (!str_value.ok) {
        printf("Key '%s' in %s must be int or string\n", key, scope);
        return 1;
    }

    if (parse_u32_literal(str_value.u.s, out)) {
        printf("Key '%s' in %s has invalid numeric string '%s'\n", key, scope, str_value.u.s);
        free(str_value.u.s);
        return 1;
    }

    free(str_value.u.s);
    *present = true;
    return 0;
}

static int32_t toml_read_required_u32(toml_table_t* tab, const char* key, uint32_t* out, const char* scope) {
    bool present = false;

    if (toml_read_optional_u32(tab, key, out, &present, scope))
        return 1;

    if (!present) {
        printf("Missing key '%s' in %s\n", key, scope);
        return 1;
    }

    return 0;
}

static bool key_is_allowed(const char* key, const char* const* allowed, uint32_t allowed_count) {
    for (uint32_t i = 0; i < allowed_count; i++) {
        if (strcmp(key, allowed[i]) == 0)
            return true;
    }

    return false;
}

static int32_t ensure_allowed_keys(toml_table_t* tab, const char* const* allowed, uint32_t allowed_count, const char* scope) {
    for (int32_t i = 0;; i++) {
        const char* key = toml_key_in(tab, i);
        if (!key)
            break;

        if (!key_is_allowed(key, allowed, allowed_count)) {
            printf("Unknown key '%s' in %s\n", key, scope);
            return 1;
        }
    }

    return 0;
}

static parsed_device_t* get_or_add_device(parsed_config_t* cfg, const char* name) {
    for (uint32_t i = 0; i < cfg->device_count; i++) {
        if (strcmp(cfg->devices[i].name, name) == 0)
            return &cfg->devices[i];
    }

    if (cfg->device_count >= CONFIG_MAX_DEVICES)
        return NULL;

    parsed_device_t* dev = &cfg->devices[cfg->device_count++];
    memset(dev, 0, sizeof(*dev));

    if (set_name(dev->name, name, "Device name"))
        return NULL;

    return dev;
}

static parsed_chip_t* get_or_add_chip(parsed_config_t* cfg, const char* name) {
    for (uint32_t i = 0; i < cfg->chip_count; i++) {
        if (strcmp(cfg->chips[i].name, name) == 0)
            return &cfg->chips[i];
    }

    if (cfg->chip_count >= CONFIG_MAX_CHIPS)
        return NULL;

    parsed_chip_t* chip = &cfg->chips[cfg->chip_count++];
    memset(chip, 0, sizeof(*chip));

    if (set_name(chip->name, name, "Chip name"))
        return NULL;

    return chip;
}

static int32_t add_link(parsed_config_t* cfg, config_link_type_e type, const char* src_bus, const char* src_entry, const char* dst_bus, uint32_t base) {
    if (cfg->link_count >= CONFIG_MAX_LINKS)
        return 1;

    parsed_link_t* link = &cfg->links[cfg->link_count++];
    memset(link, 0, sizeof(*link));

    link->type = type;
    link->base = base;

    if (set_name(link->src_bus, src_bus, "Link source bus"))
        return 1;

    if (set_name(link->src_entry, src_entry, "Link source entry"))
        return 1;

    if (set_name(link->dst_bus, dst_bus, "Link destination bus"))
        return 1;

    return 0;
}

static int32_t parse_devices_table(toml_table_t* root, parsed_config_t* cfg) {
    toml_table_t* devices_tab = toml_table_in(root, "devices");
    const char* const allowed_keys[] = { "type", "is_rom", "size", "load" };

    if (!devices_tab)
        return 0;

    for (int32_t i = 0;; i++) {
        const char* item_name = toml_key_in(devices_tab, i);
        toml_table_t* item_tab;
        char scope[64] = { 0 };

        if (!item_name)
            break;

        item_tab = toml_table_in(devices_tab, item_name);
        if (!item_tab) {
            printf("devices.%s must be a table\n", item_name);
            return 1;
        }

        snprintf(scope, sizeof(scope), "devices.%s", item_name);

        if (ensure_allowed_keys(item_tab, allowed_keys, sizeof(allowed_keys) / sizeof(allowed_keys[0]), scope))
            return 1;

        parsed_device_t* dev = get_or_add_device(cfg, item_name);
        if (!dev) {
            printf("Failed to allocate device '%s'\n", item_name);
            return 1;
        }

        if (toml_read_required_string(item_tab, "type", dev->type, sizeof(dev->type), scope))
            return 1;
        dev->has_type = true;

        if (toml_read_optional_bool(item_tab, "is_rom", &dev->is_rom, &dev->has_is_rom, scope))
            return 1;

        if (toml_read_optional_u32(item_tab, "size", &dev->size, &dev->has_size, scope))
            return 1;

        if (toml_read_optional_string(item_tab, "load", dev->load, sizeof(dev->load), &dev->has_load, scope))
            return 1;
    }

    return 0;
}

static int32_t parse_chips_table(toml_table_t* root, parsed_config_t* cfg) {
    toml_table_t* chips_tab = toml_table_in(root, "chips");
    const char* const allowed_keys[] = { "type", "reset" };

    if (!chips_tab)
        return 0;

    for (int32_t i = 0;; i++) {
        const char* item_name = toml_key_in(chips_tab, i);
        toml_table_t* item_tab;
        char scope[64] = { 0 };

        if (!item_name)
            break;

        item_tab = toml_table_in(chips_tab, item_name);
        if (!item_tab) {
            printf("chips.%s must be a table\n", item_name);
            return 1;
        }

        snprintf(scope, sizeof(scope), "chips.%s", item_name);

        if (ensure_allowed_keys(item_tab, allowed_keys, sizeof(allowed_keys) / sizeof(allowed_keys[0]), scope))
            return 1;

        parsed_chip_t* chip = get_or_add_chip(cfg, item_name);
        if (!chip) {
            printf("Failed to allocate chip '%s'\n", item_name);
            return 1;
        }

        if (toml_read_required_string(item_tab, "type", chip->type, sizeof(chip->type), scope))
            return 1;
        chip->has_type = true;

        if (toml_read_optional_u32(item_tab, "reset", &chip->reset, &chip->has_reset, scope))
            return 1;
    }

    return 0;
}

static int32_t parse_links_table(toml_table_t* root, parsed_config_t* cfg) {
    toml_table_t* links_tab = toml_table_in(root, "links");

    if (!links_tab)
        return 0;

    for (int32_t i = 0;; i++) {
        const char* src_bus_name = toml_key_in(links_tab, i);
        toml_table_t* link_tab;
        toml_table_t* bus_tab;

        if (!src_bus_name)
            break;

        link_tab = toml_table_in(links_tab, src_bus_name);
        if (!link_tab) {
            printf("links.%s must be a table\n", src_bus_name);
            return 1;
        }

        bus_tab = toml_table_in(link_tab, "bus");
        if (bus_tab) {
            for (int32_t j = 0;; j++) {
                const char* dst_bus_name = toml_key_in(bus_tab, j);
                uint32_t base = 0;
                char scope[64] = { 0 };

                if (!dst_bus_name)
                    break;

                if (toml_array_in(bus_tab, dst_bus_name) || toml_table_in(bus_tab, dst_bus_name)) {
                    printf("links.%s.bus.%s must be int/string value\n", src_bus_name, dst_bus_name);
                    return 1;
                }

                snprintf(scope, sizeof(scope), "links.%s.bus", src_bus_name);
                if (toml_read_required_u32(bus_tab, dst_bus_name, &base, scope))
                    return 1;

                if (add_link(cfg, CONFIG_LINK_TYPE_BUS, src_bus_name, "bus", dst_bus_name, base)) {
                    printf("Failed to add bus link links.%s.bus.%s\n", src_bus_name, dst_bus_name);
                    return 1;
                }
            }
        }

        for (int32_t j = 0;; j++) {
            const char* entry_name = toml_key_in(link_tab, j);
            toml_array_t* peers;

            if (!entry_name)
                break;

            if (strcmp(entry_name, "bus") == 0)
                continue;

            if (toml_table_in(link_tab, entry_name)) {
                printf("links.%s.%s: nested tables are unsupported\n", src_bus_name, entry_name);
                return 1;
            }

            peers = toml_array_in(link_tab, entry_name);
            if (!peers) {
                printf("links.%s.%s must be an array of bus names\n", src_bus_name, entry_name);
                return 1;
            }

            for (int32_t p = 0; p < toml_array_nelem(peers); p++) {
                toml_datum_t dst = toml_string_at(peers, p);

                if (!dst.ok) {
                    printf("links.%s.%s[%d] must be string\n", src_bus_name, entry_name, p);
                    return 1;
                }

                if (add_link(cfg, CONFIG_LINK_TYPE_PIN, src_bus_name, entry_name, dst.u.s, 0)) {
                    printf("Failed to add pin link links.%s.%s[%d]\n", src_bus_name, entry_name, p);
                    free(dst.u.s);
                    return 1;
                }

                free(dst.u.s);
            }
        }
    }

    return 0;
}

static int32_t parse_toml_config(const char* filename, parsed_config_t* cfg) {
    FILE* file = fopen(filename, "r");
    char errbuf[512] = { 0 };
    toml_table_t* root;

    if (!file) {
        printf("Failed to open file %s\n", filename);
        return 1;
    }

    root = toml_parse_file(file, errbuf, sizeof(errbuf));
    fclose(file);

    if (!root) {
        printf("Failed to parse TOML config %s: %s\n", filename, errbuf);
        return 1;
    }

    memset(cfg, 0, sizeof(*cfg));

    for (int32_t i = 0;; i++) {
        const char* root_key = toml_key_in(root, i);
        if (!root_key)
            break;

        if (strcmp(root_key, "devices") != 0 && strcmp(root_key, "chips") != 0 && strcmp(root_key, "links") != 0) {
            printf("Unknown root key '%s'\n", root_key);
            toml_free(root);
            return 1;
        }
    }

    if (parse_devices_table(root, cfg) || parse_chips_table(root, cfg) || parse_links_table(root, cfg)) {
        toml_free(root);
        return 1;
    }

    toml_free(root);
    return 0;
}

bus_t* find_bus_by_name(const char* name) {
    for (int32_t i = 0; i < next_bus; i++) {
        if (!buses[i])
            continue;

        if (strcmp(buses[i]->name, name) == 0)
            return buses[i];
    }

    return NULL;
}

static int32_t load_devices_from_config(parsed_config_t* cfg) {
    for (uint32_t i = 0; i < cfg->device_count; i++) {
        parsed_device_t* item = &cfg->devices[i];

        if (!item->has_type) {
            printf("Device %s has no type\n", item->name);
            return 1;
        }

        printf("Device %s:\n", item->name);
        printf("    - Type: %s\n", item->type);

        device_t* dev = device_alloc();
        if (!dev)
            return 1;

        memcpy(dev->bus.name, item->name, CONFIG_NAME_MAX);

        if (strcmp(item->type, "memory") == 0) {
            if (!item->has_size || !item->has_is_rom) {
                printf("Device %s memory config is incomplete\n", item->name);
                device_free(dev);
                return 1;
            }

            printf("    - Size: 0x%04x\n", item->size);
            printf("    - Is ROM: %s\n", item->is_rom ? "true" : "false");

            mem_init(dev, item->size, item->is_rom);

            if (item->has_load) {
                printf("    - Load: %s ", item->load);

                FILE* file = fopen(item->load, "rb");
                if (!file) {
                    printf("(CAN'T OPEN)\n");
                    device_free(dev);
                    return 1;
                }

                fseek(file, 0, SEEK_END);
                uint32_t to_read = (uint32_t)ftell(file);
                fseek(file, 0, SEEK_SET);

                if (to_read > item->size) {
                    printf("(TOO BIG)\n");
                    fclose(file);
                    device_free(dev);
                    return 1;
                }

                printf("(OK, size 0x%04x)\n", to_read);
                fread((uint8_t*)dev->data, 1, to_read, file);
                fclose(file);
            }
        } else if (strcmp(item->type, "acia") == 0) {
            acia_init(dev);
            acia_data_t* data = (acia_data_t*)dev->data;
            printf("    - PTY: %s\n", data->pty_name);
        } else if (strcmp(item->type, "display") == 0) {
            display_init(dev);
        } else if (strcmp(item->type, "timer") == 0) {
            timer_init(dev);
        } else {
            printf("Unknown peripheral type %s\n", item->type);
            device_free(dev);
            return 1;
        }

        if (next_device >= 16 || next_bus >= 32) {
            printf("Device/BUS table overflow\n");
            device_free(dev);
            return 1;
        }

        devices[next_device++] = dev;
        buses[next_bus++] = &dev->bus;
    }

    return 0;
}

static int32_t load_chips_from_config(parsed_config_t* cfg) {
    for (uint32_t i = 0; i < cfg->chip_count; i++) {
        parsed_chip_t* item = &cfg->chips[i];

        if (!item->has_type) {
            printf("Chip %s has no type\n", item->name);
            return 1;
        }

        printf("Chip %s:\n", item->name);
        printf("    - Type: %s\n", item->type);

        bare6502_t* chip = bare6502_alloc();
        if (!chip)
            return 1;

        memcpy(chip->bus.name, item->name, CONFIG_NAME_MAX);

        if (strcmp(item->type, "6502") == 0) {
            bare6502_init(chip, MOS6502);
        } else if (strcmp(item->type, "65C02") == 0) {
            bare6502_init(chip, MOS65C02);
        } else {
            printf("Unknown chip type %s\n", item->type);
            bare6502_free(chip);
            return 1;
        }

        if (item->has_reset) {
            printf("    - Reset: 0x%04x\n", item->reset);
            chip->pc = item->reset;
        }

        if (next_chip >= 16 || next_bus >= 32) {
            printf("Chip/BUS table overflow\n");
            bare6502_free(chip);
            return 1;
        }

        chips[next_chip++] = chip;
        buses[next_bus++] = &chip->bus;
    }

    return 0;
}

static int32_t load_links_from_config(parsed_config_t* cfg) {
    for (uint32_t i = 0; i < cfg->link_count; i++) {
        parsed_link_t* l = &cfg->links[i];

        bus_t* bus0 = find_bus_by_name(l->src_bus);
        bus_t* bus1 = find_bus_by_name(l->dst_bus);

        if (!bus0 || !bus1) {
            printf("Failed to resolve link %s -> %s\n", l->src_bus, l->dst_bus);
            return 1;
        }

        bus_link_t* link = bus_attach_by_name(bus0, bus1, l->src_entry, l->src_entry);
        if (!link) {
            printf("Failed to attach link %s <-%s-> %s\n", l->src_bus, l->src_entry, l->dst_bus);
            return 1;
        }

        if (l->type == CONFIG_LINK_TYPE_BUS) {
            link->base = l->base;
            printf("%s <-%s[0x%04x]-> %s\n", l->src_bus, l->src_entry, l->base, l->dst_bus);
        } else {
            printf("%s <-%s-> %s\n", l->src_bus, l->src_entry, l->dst_bus);
        }
    }

    return 0;
}

int32_t load_config(const char* filename) {
    parsed_config_t cfg;

    if (parse_toml_config(filename, &cfg)) {
        printf("Failed to parse TOML config\n");
        return 1;
    }

    if (load_devices_from_config(&cfg))
        return 1;

    if (load_chips_from_config(&cfg))
        return 1;

    if (load_links_from_config(&cfg))
        return 1;

    return 0;
}

int main(int argc, char* argv[]) {
    int opt;

    char config_path[1024] = { 0 };

    if (setup_signal_handlers())
        return 1;

    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
            case 'c':
                strncpy(config_path, optarg, sizeof(config_path) - 1);
                config_path[sizeof(config_path) - 1] = '\0';
                break;

            default:
                printf("Usage: %s [-c config]\n", argv[0]);
                return 1;
        }
    }

    if (strlen(config_path) == 0) {
        printf("No config file specified, using default\n");
        strcpy(config_path, "pixel.toml");
    }

    printf("Using config file %s\n", config_path);

    if (load_config(config_path))
        printf("Failed to load config\n");
    else
        printf("Config loaded\n");

    if (next_chip == 0) {
        printf("No chips configured\n");
        exit(1);
    }

    printf("Press any key to start\n");
    getchar();

    if (terminate_requested)
        printf("SIGTERM received, graceful shutdown requested\n");

    printf("Running...\n");

    while (!terminate_requested) {
        uint8_t halted = 0;
        for (uint32_t i = 0; i < next_chip; i++) {
            bare6502_t* chip = chips[i];

            if (chip->step)
                chip->step(chip);

            if (chip->state == HALTED)
                halted++;
        }

        if (halted == next_chip)
            break;

        for (uint32_t i = 0; i < next_device; i++) {
            device_t* device = devices[i];
            if (device->step)
                device->step(device);
        }
    }

    if (terminate_requested)
        printf("Stopped by SIGTERM\n");

    printf("Done!\n");

    for (uint32_t i = 0; i < next_chip; i++) {
        bare6502_t* chip = chips[i];
        bare6502_sync_stats(chip);

        double instr_time = chip->instructions ? (chip->time / chip->instructions) : 0;

        printf("Chip %s\n", chip->bus.name);
        printf("    - Seconds emulated: %lf s\n", chip->time / 1e9);
        printf("    - Instructions: %lu\n", chip->instructions);
        printf("    - Instruction time: %lf ns\n", instr_time);
        printf("    - Instruction speed: %.2f MHz\n", instr_time > 0 ? (1000 / instr_time) : 0);
    }

    for (uint32_t i = 0; i < next_device; i++) {
        device_t* device = devices[i];
        if (device)
            device_free(device);
    }

    for (uint32_t i = 0; i < next_chip; i++) {
        bare6502_t* chip = chips[i];
        if (chip)
            bare6502_free(chip);
    }

    return 0;
}
