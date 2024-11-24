#include <bare6502.h>
#include <utils.h>

#define TEST_BIT(v, x) ((v) & (1 << x))

bare6502_t* bare6502_alloc() {
    bare6502_t* chip = malloc(sizeof(bare6502_t));
    memset(chip, 0, sizeof(bare6502_t));

    return chip;
}

void bare6502_free(bare6502_t* chip) {
    free(chip);
}

static inline uint8_t bare6502_read_byte(bare6502_t* chip, uint16_t addr) {
    return bus_io_bus_read(&chip->bus, "bus", addr) & 0xFF;
}

static inline void bare6502_write_byte(bare6502_t* chip, uint16_t addr, uint8_t byte) {
    bus_io_bus_write(&chip->bus, "bus", addr, byte);
}

static inline void bare6502_push(bare6502_t* chip, uint8_t byte) {
    bare6502_write_byte(chip, 0x100 + chip->sp--, byte);
}

static inline uint8_t bare6502_pop(bare6502_t* chip) {
    return bare6502_read_byte(chip, 0x100 | ++chip->sp);
}

void bare6502_interrupt(bare6502_t* chip, uint16_t vector) {
    // chip->pc++;

    bare6502_push(chip, (chip->pc >> 8) & 0xFF);
    bare6502_push(chip, chip->pc & 0xFF);
    bare6502_push(chip, chip->p | 0x20);

    chip->i = 1;

    chip->pc        = vector;
    uint8_t vec_lo  = bare6502_read_byte(chip, chip->pc++);
    uint8_t vec_hi  = bare6502_read_byte(chip, chip->pc++);
    chip->pc        = (vec_hi << 8) | vec_lo;
}

uint16_t bare6502_alu(bare6502_t* chip, uint8_t a, uint8_t b, bare6502_opcode_e opcode) {
    uint16_t res = 0;

    switch (opcode) {
        case ADC:
            res = a + b + chip->c;
            break;
        case SBC:
            res = a + (b ^ 0xFF) + chip->c;
            break;
        case INC:
        case INX:
        case INY:
            res = a + 1;
            break;
        case DEC:
        case DEX:
        case DEY:
            res = a - 1;
            break;
        case CMP:
        case CPX:
        case CPY:
            res = a - b;
            break;
        case AND:
        case BIT:
            res = a & b;
            break;
        case SMB0:
        case SMB1:
        case SMB2:
        case SMB3:
        case SMB4:
        case SMB5:
        case SMB6:
        case SMB7:
            res = a | (1 << b);
            break;
        case RMB0:
        case RMB1:
        case RMB2:
        case RMB3:
        case RMB4:
        case RMB5:
        case RMB6:
        case RMB7:
            res = a & ~(1 << b);
            break;
        case ORA:
            res = a | b;
            break;
        case EOR:
            res = a ^ b;
            break;
        case ASL:
            res = a << 1;
            break;
        case LSR:
            res = a >> 1;
            break;
        case ROL:
            res = (a << 1) | chip->c;
            break;
        case ROR:
            res = (a >> 1) | (chip->c << 7);
            break;
        case TSB:
            res = a | b;
            break;
        case TRB:
            res = a & ~b;
            break;
        case TXA:
        case TXS:
        case TSX:
        case TAX:
        case TAY:
        case TYA:
        case LDA:
        case LDX:
        case LDY:
        case PLA:
        case PLY:
        case PLX:
            res = a;
            break;

        default:
            printf("Unknown opcode %d\n", opcode);
            exit(1);
    };

    // Carry calculation
    switch (opcode) {
        case ADC:
        case SBC:
            chip->c = (res & 0x100) > 0;
            break;
        case CMP:
        case CPX:
        case CPY:
            chip->c = a >= b;
            break;
        case ASL:
        case ROL:
            chip->c = (a & 0x80) > 0;
            break;
        case ROR:
        case LSR:
            chip->c = (a & 0x01) > 0;
            break;
    };

    // Zero calculation
    switch (opcode) {
        case ADC:
        case SBC:
        case INC:
        case INX:
        case INY:
        case DEC:
        case DEX:
        case DEY:
        case CMP:
        case CPX:
        case CPY:
        case AND:
        case ORA:
        case EOR:
        case BIT:
        case ASL:
        case LSR:
        case ROL:
        case ROR:
        case TSB:
        case TRB:
        case TXA:
        case TXS:
        case TSX:
        case TAX:
        case TAY:
        case TYA:
        case LDA:
        case LDX:
        case LDY:
        case PLA:
        case PLY:
        case PLX:
            chip->z = (res & 0xFF) == 0;
            break;
    };

    // Negative calculation
    switch (opcode) {
        case ADC:
        case SBC:
        case INC:
        case INX:
        case INY:
        case DEC:
        case DEX:
        case DEY:
        case CMP:
        case CPX:
        case CPY:
        case AND:
        case ORA:
        case EOR:
        case ASL:
        case LSR:
        case ROL:
        case ROR:
        case TXA:
        case TXS:
        case TSX:
        case TAX:
        case TAY:
        case TYA:
        case LDA:
        case LDX:
        case LDY:
        case PLA:
        case PLY:
        case PLX:
            chip->n = (res & 0x80) > 0;
            break;
        case BIT:
            chip->n = (b & 0x80) > 0;
            break;
    };

    // Overflow calculation
    switch (opcode) {
        case ADC:
            chip->v = ((a ^ b ^ 0x80) & (a ^ res) & 0x80) ? 1 : 0;
            break;
        case SBC:
            chip->v = ((a ^ b) & (a ^ res) & 0x80) ? 1 : 0;
            break;
        case BIT:
            chip->v = (b & 0x40) ? 1 : 0;
            break;
    };

    return res;
}

static inline uint32_t bare6502_execute(bare6502_t* chip) {
    bare6502_opcode_e opcode = chip->opcode->opcode;

    switch (opcode) {
        case ADC:
        case SBC:
            chip->a = bare6502_alu(chip, chip->a, chip->operand, opcode);
            return 0;

        case INC:
        case DEC:
        case ASL:
        case LSR:
        case ROL:
        case ROR:
        case TSB:
        case TRB:
            chip->operand = bare6502_alu(chip, chip->operand, 0, opcode);
            return 1;

        case CMP:
            bare6502_alu(chip, chip->a, chip->operand, opcode);
            return 0;

        case CPX:
            bare6502_alu(chip, chip->x, chip->operand, opcode);
            return 0;

        case CPY:
            bare6502_alu(chip, chip->y, chip->operand, opcode);
            return 0;

        case INX:
            chip->x = bare6502_alu(chip, chip->x, 0, opcode);
            return 0;

        case INY:
            chip->y = bare6502_alu(chip, chip->y, 0, opcode);
            return 0;

        case DEX:
            chip->x = bare6502_alu(chip, chip->x, 0, opcode);
            return 0;

        case DEY:
            chip->y = bare6502_alu(chip, chip->y, 0, opcode);
            return 0;

        case AND:
        case ORA:
        case EOR:
            chip->a = bare6502_alu(chip, chip->a, chip->operand, opcode);
            return 0;

        case BIT:
            bare6502_alu(chip, chip->a, chip->operand, opcode);
            return 0;

        case SMB0:
        case RMB0:
            chip->operand = bare6502_alu(chip, chip->operand, 0, opcode);
            return 1;

        case SMB1:
        case RMB1:
            chip->operand = bare6502_alu(chip, chip->operand, 1, opcode);
            return 1;

        case SMB2:
        case RMB2:
            chip->operand = bare6502_alu(chip, chip->operand, 2, opcode);
            return 1;

        case SMB3:
        case RMB3:
            chip->operand = bare6502_alu(chip, chip->operand, 3, opcode);
            return 1;

        case SMB4:
        case RMB4:
            chip->operand = bare6502_alu(chip, chip->operand, 4, opcode);
            return 1;

        case SMB5:
        case RMB5:
            chip->operand = bare6502_alu(chip, chip->operand, 5, opcode);
            return 1;

        case SMB6:
        case RMB6:
            chip->operand = bare6502_alu(chip, chip->operand, 6, opcode);
            return 1;

        case SMB7:
        case RMB7:
            chip->operand = bare6502_alu(chip, chip->operand, 7, opcode);
            return 1;

        case PHA:
            bare6502_push(chip, chip->a);
            return 0;

        case PHP:
            bare6502_push(chip, chip->p | 0x30);
            return 0;

        case PHX:
            bare6502_push(chip, chip->x);
            return 0;

        case PHY:
            bare6502_push(chip, chip->y);
            return 0;

        case PLA:
            chip->a = bare6502_alu(chip, bare6502_pop(chip), 0, opcode);
            return 0;

        case PLP:
            chip->p = bare6502_pop(chip) | 0x20;
            return 0;

        case PLX:
            chip->x = bare6502_alu(chip, bare6502_pop(chip), 0, opcode);
            return 0;

        case PLY:
            chip->y = bare6502_alu(chip, bare6502_pop(chip), 0, opcode);
            return 0;

        case TAX:
            chip->x = bare6502_alu(chip, chip->a, 0, opcode);
            return 0;

        case TAY:
            chip->y = bare6502_alu(chip, chip->a, 0, opcode);
            return 0;

        case TSX:
            chip->x = bare6502_alu(chip, chip->sp, 0, opcode);
            return 0;

        case TXA:
            chip->a = bare6502_alu(chip, chip->x, 0, opcode);
            return 0;

        case TXS:
            chip->sp = chip->x;
            return 0;

        case TYA:
            chip->a = bare6502_alu(chip, chip->y, 0, opcode);
            return 0;

        case LDA:
            chip->a = bare6502_alu(chip, chip->operand, 0, opcode);
            return 0;

        case LDX:
            chip->x = bare6502_alu(chip, chip->operand, 0, opcode);
            return 0;

        case LDY:
            chip->y = bare6502_alu(chip, chip->operand, 0, opcode);
            return 0;

        case STA:
            chip->operand = chip->a;
            return 1;

        case STX:
            chip->operand = chip->x;
            return 1;

        case STY:
            chip->operand = chip->y;
            return 1;

        case BCC:
            if (!chip->c)
                chip->pc = chip->ea;

            return 0;

        case BCS:
            if (chip->c)
                chip->pc = chip->ea;

            return 0;

        case BEQ:
            if (chip->z)
                chip->pc = chip->ea;

            return 0;

        case BNE:
            if (!chip->z)
                chip->pc = chip->ea;

            return 0;

        case BMI:
            if (chip->n)
                chip->pc = chip->ea;

            return 0;

        case BPL:
            if (!chip->n)
                chip->pc = chip->ea;

            return 0;

        case BVS:
            if (chip->v)
                chip->pc = chip->ea;

            return 0;

        case BVC:
            if (!chip->v)
                chip->pc = chip->ea;

            return 0;

        case BRA:
            chip->pc = chip->ea;
            return 0;

        case BBR0:
            if (!TEST_BIT(chip->operand, 0))
                chip->pc += chip->operand2;

            return 0;

        case BBR1:
            if (!TEST_BIT(chip->operand, 1))
                chip->pc += chip->operand2;

            return 0;

        case BBR2:
            if (!TEST_BIT(chip->operand, 2))
                chip->pc += chip->operand2;

            return 0;

        case BBR3:
            if (!TEST_BIT(chip->operand, 3))
                chip->pc += chip->operand2;

            return 0;

        case BBR4:
            if (!TEST_BIT(chip->operand, 4))
                chip->pc += chip->operand2;

            return 0;

        case BBR5:
            if (!TEST_BIT(chip->operand, 5))
                chip->pc += chip->operand2;

            return 0;

        case BBR6:
            if (!TEST_BIT(chip->operand, 6))
                chip->pc += chip->operand2;

            return 0;

        case BBR7:
            if (!TEST_BIT(chip->operand, 7))
                chip->pc += chip->operand2;

            return 0;

        case BBS0:
            if (TEST_BIT(chip->operand, 0))
                chip->pc += chip->operand2;

            return 0;

        case BBS1:
            if (TEST_BIT(chip->operand, 1))
                chip->pc += chip->operand2;

            return 0;

        case BBS2:
            if (TEST_BIT(chip->operand, 2))
                chip->pc += chip->operand2;

            return 0;

        case BBS3:
            if (TEST_BIT(chip->operand, 3))
                chip->pc += chip->operand2;

            return 0;

        case BBS4:
            if (TEST_BIT(chip->operand, 4))
                chip->pc += chip->operand2;

            return 0;

        case BBS5:
            if (TEST_BIT(chip->operand, 5))
                chip->pc += chip->operand2;

            return 0;

        case BBS6:
            if (TEST_BIT(chip->operand, 6))
                chip->pc += chip->operand2;

            return 0;

        case BBS7:
            if (TEST_BIT(chip->operand, 7))
                chip->pc += chip->operand2;

            return 0;

        case JMP:
            chip->pc = chip->ea;
            return 0;

        case JSR:
            chip->pc--;

            bare6502_push(chip, chip->pc >> 8);
            bare6502_push(chip, chip->pc & 0xff);

            chip->pc = chip->ea;
            return 0;

        case RTS:
            chip->pc = bare6502_pop(chip);
            chip->pc |= bare6502_pop(chip) << 8;
            chip->pc++;
            return 0;

        case RTI:
            chip->p = bare6502_pop(chip);
            chip->pc = bare6502_pop(chip);
            chip->pc |= bare6502_pop(chip) << 8;
            return 0;

        case BRK:
            chip->b = 1;
            chip->pc++;
            bare6502_interrupt(chip, 0xFFFE);
            return 0;

        case CLC:
            chip->c = 0;
            return 0;

        case CLD:
            chip->d = 0;
            return 0;

        case CLI:
            chip->i = 0;
            return 0;

        case CLV:
            chip->v = 0;
            return 0;

        case SEC:
            chip->c = 1;
            return 0;

        case SED:
            chip->d = 1;
            return 0;

        case SEI:
            chip->i = 1;
            return 0;

        case STZ:
            chip->operand = 0;
            return 1;

        case NID:
            chip->state = HALTED;
            return 0;

        case NOP:
            return 0;

        default:
            printf("Unknown opcode: %s\n", chip->opcode->mnemonic);
            exit(1);
    }
}

static inline void bare6502_apply_address_mode(bare6502_t* chip, bare6502_address_mode_e mode) {
    uint16_t addr0 = 0, addr1 = 0;

    switch (mode) {
        case IMM:
            chip->ea = chip->pc++;
            break;
        case ZPG:
            addr0  = bare6502_read_byte(chip, chip->pc++);
            addr0 &= 0xFF;

            chip->ea = addr0;
            break;
        case ZPX:
            addr0  = bare6502_read_byte(chip, chip->pc++) + chip->x;
            addr0 &= 0xFF;

            chip->ea = addr0;
            break;
        case ZPY:
            addr0  = bare6502_read_byte(chip, chip->pc++) + chip->y;
            addr0 &= 0xFF;

            chip->ea = addr0;
            break;
        case REL:
            addr0  = bare6502_read_byte(chip, chip->pc++);
            addr0 &= 0xFF;
            addr0  = (chip->pc + (int8_t)addr0);

            chip->ea = addr0;
            break;
        case ABS:
            addr0  = bare6502_read_byte(chip, chip->pc++);
            addr0 |= bare6502_read_byte(chip, chip->pc++) << 8;

            chip->ea = addr0;
            break;
        case ABX:
            addr0  = bare6502_read_byte(chip, chip->pc++);
            addr0 |= bare6502_read_byte(chip, chip->pc++) << 8;

            addr0 += chip->x;

            chip->ea = addr0;
            break;
        case ABY:
            addr0  = bare6502_read_byte(chip, chip->pc++);
            addr0 |= bare6502_read_byte(chip, chip->pc++) << 8;

            addr0 += chip->y;

            chip->ea = addr0;
            break;
        case IND:
            addr0  = bare6502_read_byte(chip, chip->pc++);
            addr0 |= bare6502_read_byte(chip, chip->pc++) << 8;

            addr1  = bare6502_read_byte(chip, addr0);
            addr1 |= bare6502_read_byte(chip, addr0 + 1) << 8;

            chip->ea = addr1;
            break;
        case IDX:
            addr0  = bare6502_read_byte(chip, chip->pc++) + chip->x;
            addr0 &= 0xFF;

            addr1  = bare6502_read_byte(chip, addr0);
            addr1 |= bare6502_read_byte(chip, addr0 + 1) << 8;

            chip->ea = addr1;
            break;
        case IDY:
            addr0  = bare6502_read_byte(chip, chip->pc++);
            addr0 &= 0xFF;

            addr1  = bare6502_read_byte(chip, addr0);
            addr1 |= bare6502_read_byte(chip, addr0 + 1) << 8;

            addr1 += chip->y;

            chip->ea = addr1;
            break;

        case IDZ:
            addr0  = bare6502_read_byte(chip, chip->pc++);
            addr0 &= 0xFF;

            addr1  = bare6502_read_byte(chip, addr0);
            addr1 |= bare6502_read_byte(chip, addr0 + 1) << 8;

            chip->ea = addr1;
            break;

        case IMP:
        case ACC:
            break;

        default:
            printf("Unhandled address mode %d\n", mode);
            chip->state = HALTED;
    }
}

void bare6502_reset(bare6502_t* chip, uint32_t address) {
    chip->p         = 0x34;
    chip->pc        = address;

    uint8_t vec_lo  = bare6502_read_byte(chip, chip->pc++);
    uint8_t vec_hi  = bare6502_read_byte(chip, chip->pc++);
    chip->pc        = (vec_hi << 8) | vec_lo;

    printf("%s: Resetting to 0x%04x\n", chip->bus.name, chip->pc);
}

void bare6502_step(bare6502_t* chip) {
    switch (chip->state) {
        case STOPPED:
            if (!chip->pc)
                bare6502_reset(chip, 0xFFFC);

            chip->state      = RUNNING;
            chip->time       = 0;
            break;

        case IN_NMI:
        case IN_IRQ:
        case RUNNING: {
            double start = get_time_in_nanoseconds();

            chip->opcode_address = chip->pc;
            chip->opcode_byte = bare6502_read_byte(chip, chip->pc++);
            chip->opcode      = &bare6502_opcode_matrix[chip->type][chip->opcode_byte];

            if (chip->opcode->opcode == NID) {
                printf("Code: ");

                #define SIZE 10
                #define HALF (SIZE / 2)

                for (int i = -HALF; i < HALF; i++)
                    printf(i ? "%02x " : "[%02x] ", bare6502_read_byte(chip, chip->opcode_address + i));

                printf("\n");
                printf("Unimplemented opcode 0x%02x\n", chip->opcode_byte);
                chip->state = HALTED;
                return;
            }

            bare6502_apply_address_mode(chip, chip->opcode->mode);

            switch (chip->opcode_byte) {
                case 0x00:
                    if (chip->state != RUNNING)
                        break;

                    // printf("Got BRK\n");
                    chip->state = IN_IRQ;
                    break;
                case 0x40:
                    if (chip->state != IN_NMI && chip->state != IN_IRQ) {
                        chip->state = HALTED;
                        printf("Trying to return from NMI/IRQ without being in NMI/IRQ\n");
                        return;
                    } else {
                        // printf("Returning from NMI/IRQ\n");
                        chip->state = RUNNING;
                    }
            }

            // printf("pc: 0x%04x, opcode: 0x%02x (%s)\n", chip->pc - 1, chip->opcode_byte, chip->opcode->mnemonic);
            // printf("a: 0x%02x, x: 0x%02x, y: 0x%02x, sp: 0x%02x, p: 0x%02x, flags: %c%c%c%c%c%c%c\n", chip->a, chip->x, chip->y, chip->sp, chip->p,
            //     chip->n ? 'N' : 'n',
            //     chip->v ? 'V' : 'v',
            //     chip->b ? 'B' : 'b',
            //     chip->d ? 'D' : 'd',
            //     chip->i ? 'I' : 'i',
            //     chip->z ? 'Z' : 'z',
            //     chip->c ? 'C' : 'c'
            // );

            if (bare6502_opcode_has_operand[chip->opcode->opcode]) {
                if (chip->opcode->mode == ACC)
                    chip->operand = chip->a;
                else
                    chip->operand = bare6502_read_byte(chip, chip->ea);

                switch (chip->opcode->opcode) {
                    case BBR0:
                    case BBR1:
                    case BBR2:
                    case BBR3:
                    case BBR4:
                    case BBR5:
                    case BBR6:
                    case BBR7:
                    case BBS0:
                    case BBS1:
                    case BBS2:
                    case BBS3:
                    case BBS4:
                    case BBS5:
                    case BBS6:
                    case BBS7:
                        chip->operand2 = bare6502_read_byte(chip, chip->pc++);
                        break;
                }
            }

            if (bare6502_execute(chip)) {
                if (chip->opcode->mode == ACC)
                    chip->a = chip->operand;
                else
                    bare6502_write_byte(chip, chip->ea, chip->operand);
            }

            if (chip->pc == chip->opcode_address) {
                printf("Deadlock dectected at 0x%04x, join HALT\n", chip->pc);
                chip->state = HALTED;
            }

            chip->instructions++;
            chip->time += get_time_in_nanoseconds() - start;
        } break;

        case HALTED:
            break;
    }
}

void bare6502_pin_write(bus_link_t* link, uint32_t data) {
    bare6502_t* chip = bus_container_of(link->dst_bus, bare6502_t);

    if (strcmp(link->dst->name, "irq") == 0) {
        if (data && chip->state == RUNNING && !chip->i) {
            // printf("Got IRQ at 0x%04x\n", chip->pc);
            bare6502_interrupt(chip, 0xFFFE);
            chip->state = IN_IRQ;
        }
    } else if (strcmp(link->dst->name, "nmi") == 0) {
        if (data && chip->state == RUNNING) {
            // printf("Got NMI at 0x%04x\n", chip->pc);
            bare6502_interrupt(chip, 0xFFFA);
            chip->state = IN_NMI;
        }
    } else if (strcmp(link->dst->name, "reset") == 0) {
        bare6502_reset(chip, 0xFFFC);
    }
}

bus_entry_t bare6502_bus_entries[] = {
    {
        .name   = "bus",
        .type   = BUS_TYPE_BUS,

        .address_width  = 16,
        .data_width     = 8,

        .io_bus_read    = NULL,
        .io_bus_write   = NULL,
    },
    {
        .name   = "irq",
        .type   = BUS_TYPE_PIN,

        .io_pin_read  = NULL,
        .io_pin_write = bare6502_pin_write,
    },
    {
        .name   = "nmi",
        .type   = BUS_TYPE_PIN,

        .io_pin_read  = NULL,
        .io_pin_write = bare6502_pin_write,
    },
    {
        .name   = "reset",
        .type   = BUS_TYPE_PIN,

        .io_pin_read  = NULL,
        .io_pin_write = bare6502_pin_write,
    }
};

void bare6502_init(bare6502_t* chip, bare6502_type_e type) {
    chip->type = type;

    chip->bus.entries       = bare6502_bus_entries;
    chip->bus.entry_count   = sizeof(bare6502_bus_entries) / sizeof(bus_entry_t);
    chip->step              = bare6502_step;
}