#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bare6502.h>
#include <peripheral.h>
#include <json.h>
#include <utils.h>

bare6502_t* chips[16]   = { 0 };
device_t*   devices[16] = { 0 };
bus_t*      buses[32]   = { 0 };

uint32_t    next_chip = 0;
uint32_t    next_device = 0;
uint32_t    next_bus = 0;

bus_t* find_bus_by_name(const char* name) {
    for (int i = 0; i < next_bus; i++) {
        if (!buses[i])
            continue;

        if (strcmp(buses[i]->name, name) == 0)
            return buses[i];
    }

    return NULL;
}

typed(json_element) __json_load(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open file %s\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    uint32_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(size);
    fread(buffer, 1, size, file);
    fclose(file);

    result(json_element) element = json_parse(buffer);

    if (result_is_err(json_element)(&element))
        exit(1);

    return result_unwrap(json_element)(&element);
}

uint32_t __json_get_element(typed(json_element) elem0, const char* key, typed(json_element_type) type, typed(json_element)* result) {
    typed(json_element) elem1;

    if (elem0.type == JSON_ELEMENT_TYPE_OBJECT) {
        result(json_element) res = json_object_find(elem0.value.as_object, key);

        if (result_is_err(json_element)(&res))
            return 1;

        elem1 = result_unwrap(json_element)(&res);
    } else {
        return 1;
    }

    if (elem1.type != type)
        return 1;

    *result = elem1;
    return 0;
}

int32_t load_devices(typed(json_element) root) {
    typed(json_element) elem0;

    if (__json_get_element(root, "devices", JSON_ELEMENT_TYPE_OBJECT, &elem0)) {
        printf("Failed to get devices object\n");
        return 1;
    }

    for (int i = 0; i < elem0.value.as_object->count; i++) {
        typed(json_element) elem1;
        typed(json_entry)* entry0 = elem0.value.as_object->entries[i];

        if (entry0->element.type != JSON_ELEMENT_TYPE_OBJECT)
            continue;

        if (__json_get_element(entry0->element, "type", JSON_ELEMENT_TYPE_STRING, &elem1))
            continue;

        printf("Device %s:\n", entry0->key);

        printf("    - Type: %s\n", elem1.value.as_string);

        device_t* dev = device_alloc();
        memcpy(dev->bus.name, entry0->key, 16);

        if (strcmp(elem1.value.as_string, "memory") == 0) {
            if (__json_get_element(entry0->element, "size", JSON_ELEMENT_TYPE_STRING, &elem1)) {
                printf("Failed to get size field\n");
                device_free(dev);
                continue;
            }
            uint32_t size = (uint32_t)strtol(elem1.value.as_string, NULL, 0);
            printf("    - Size: 0x%04x\n", size);

            if (__json_get_element(entry0->element, "is_rom", JSON_ELEMENT_TYPE_BOOLEAN, &elem1)) {
                printf("Failed to get is_rom field\n");
                device_free(dev);
                continue;
            }
            bool is_rom = elem1.value.as_boolean;
            printf("    - Is ROM: %s\n", is_rom ? "true" : "false");

            mem_init(dev, size, is_rom);

            if (!__json_get_element(entry0->element, "load", JSON_ELEMENT_TYPE_STRING, &elem1)) {
                printf("    - Load: %s ", elem1.value.as_string);

                FILE* file = fopen(elem1.value.as_string, "r");
                if (!file) {
                    printf("(CAN'T OPEN)\n");
                    continue;
                }

                fseek(file, 0, SEEK_END);
                uint32_t to_read = ftell(file);
                fseek(file, 0, SEEK_SET);

                if (to_read > size) {
                    printf("(TOO BIG)\n");
                    fclose(file);
                    continue;
                }

                printf("(OK, size 0x%04x)\n", to_read);

                fread((uint8_t*)dev->data, 1, to_read, file);
                fclose(file);
            }
        } else if (strcmp(elem1.value.as_string, "acia") == 0) {
            acia_init(dev);
            acia_data_t* data = (acia_data_t*)dev->data;
            printf("    - PTY: %s\n", data->pty_name);
        } else if (strcmp(elem1.value.as_string, "display") == 0) {
            display_init(dev);
        } else if (strcmp(elem1.value.as_string, "timer") == 0) {
            timer_init(dev);
        } else {
            printf("Unknown peripheral type %s\n", elem1.value.as_string);
            return 1;
        }

        devices[next_device++] = dev;
        buses[next_bus++] = &dev->bus;
    }

    return 0;
}

int32_t load_chips(typed(json_element) root) {
    typed(json_element) elem0;

    if (__json_get_element(root, "chips", JSON_ELEMENT_TYPE_OBJECT, &elem0)) {
        printf("Failed to get chips object\n");
        return 1;
    }

    for (int i = 0; i < elem0.value.as_object->count; i++) {
        typed(json_element) elem1;
        typed(json_entry)* entry0 = elem0.value.as_object->entries[i];

        if (entry0->element.type != JSON_ELEMENT_TYPE_OBJECT)
            continue;

        if (__json_get_element(entry0->element, "type", JSON_ELEMENT_TYPE_STRING, &elem1))
            continue;

        printf("Chip %s:\n", entry0->key);

        printf("    - Type: %s\n", elem1.value.as_string);

        bare6502_t* chip = bare6502_alloc();
        memcpy(chip->bus.name, entry0->key, 16);

        if (strcmp(elem1.value.as_string, "6502") == 0) {
            bare6502_init(chip, MOS6502);
        } else if (strcmp(elem1.value.as_string, "65C02") == 0) {
            bare6502_init(chip, MOS65C02);
        } else {
            printf("Unknown chip type %s\n", elem1.value.as_string);
            return 1;
        }

        if (!__json_get_element(entry0->element, "reset", JSON_ELEMENT_TYPE_STRING, &elem1)) {
            printf("    - Reset: %s\n", elem1.value.as_string);
            chip->pc = (uint32_t)strtol(elem1.value.as_string, NULL, 0);
        }

        chips[next_chip++] = chip;
        buses[next_bus++] = &chip->bus;
    }

    return 0;
}

int32_t load_links(typed(json_element) root) {
    typed(json_element) elem0;

    if (__json_get_element(root, "links", JSON_ELEMENT_TYPE_OBJECT, &elem0)) {
        printf("Failed to get links object\n");
        return 1;
    }

    for (int i = 0; i < elem0.value.as_object->count; i++) {
        typed(json_entry)* entry0 = elem0.value.as_object->entries[i];

        if (entry0->element.type != JSON_ELEMENT_TYPE_OBJECT)
            continue;

        bus_t* bus0 = find_bus_by_name(entry0->key);
        if (!bus0)
            continue;

        for (int j = 0; j < entry0->element.value.as_object->count; j++) {
            typed(json_entry)* entry1 = entry0->element.value.as_object->entries[j];

            switch (entry1->element.type) {
                case JSON_ELEMENT_TYPE_OBJECT: {
                    for (int k = 0; k < entry1->element.value.as_object->count; k++) {
                        typed(json_entry)* entry2 = entry1->element.value.as_object->entries[k];

                        if (entry2->element.type != JSON_ELEMENT_TYPE_STRING)
                            continue;

                        bus_t* bus1 = find_bus_by_name(entry2->key);
                        if (!bus1)
                            continue;

                        printf("%s <-%s[%s]-> %s\n", entry0->key, entry1->key, entry2->element.value.as_string, entry2->key);

                        uint32_t base = (uint32_t)strtol(entry2->element.value.as_string, NULL, 0);
                        bus_link_t* link = bus_attach_by_name(bus0, bus1, entry1->key, entry1->key);

                        link->base = base;
                    }
                } break;

                case JSON_ELEMENT_TYPE_ARRAY: {
                    for (int k = 0; k < entry1->element.value.as_array->count; k++) {
                        typed(json_element) elem2 = entry1->element.value.as_array->elements[k];

                        if (elem2.type != JSON_ELEMENT_TYPE_STRING)
                            continue;

                        bus_t* bus1 = find_bus_by_name(elem2.value.as_string);
                        if (!bus1)
                            continue;

                        printf("%s <-%s-> %s\n", entry0->key, entry1->key, elem2.value.as_string);

                        bus_attach_by_name(bus0, bus1, entry1->key, entry1->key);
                    }
                } break;
            }
        }
    }
}

int32_t load_config(const char* filename) {
    typed(json_element) root = __json_load(filename);
    typed(json_element) elem0;

    if (load_devices(root))
        return 1;

    if (load_chips(root))
        return 1;

    if (load_links(root))
        return 1;

    printf("Configuration loaded!\n");

    return 0;
}

int main(int argc, char* argv[]) {
    int opt;

    char config_path[1024] = { 0 };

    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
            case 'c':
                memcpy(config_path, optarg, sizeof(config_path));
                break;

            default:
                printf("Usage: %s [-c config]\n", argv[0]);
                return 1;
        }
    }

    if (strlen(config_path) == 0) {
        printf("No config file specified, using default\n");
        strcpy(config_path, "config.json");
    }

    printf("Using config file %s\n", config_path);

    load_config(config_path);

    if (next_chip == 0) {
        printf("No chips configured\n");
        exit(1);
    }

    printf("Press any key to start\n");
    getchar();

    printf("Running...\n");

    while (1) {
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

    printf("Done!\n");

    for (uint32_t i = 0; i < next_chip; i++) {
        bare6502_t* chip = chips[i];

        double instr_time = chip->time / chip->instructions;

        printf("Chip %s\n", chip->bus.name);
        printf("    - Seconds emulated: %lf s\n", chip->time / 1e9);
        printf("    - Instructions: %lu\n", chip->instructions);
        printf("    - Instruction time: %lf ns\n", instr_time);
        printf("    - Instruction speed: %.2f MHz\n", 1000 / instr_time);
    }

    return 0;
}