#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <bus.h>

typedef struct      bare6502                bare6502_t;
typedef enum        bare6502_state          bare6502_state_e;
typedef enum        bare6502_type           bare6502_type_e;
typedef enum        bare6502_address_mode   bare6502_address_mode_e;
typedef enum        bare6502_opcode         bare6502_opcode_e;
typedef struct      bare6502_opcode_entry   bare6502_opcode_t;

enum bare6502_state {
    STOPPED,
    RUNNING,
    IN_NMI,
    IN_IRQ,
    HALTED,
};

enum bare6502_type {
    MOS6502,
    MOS65C02,
    CHIP_TYPE_MAX,
};

struct bare6502 {
    uint32_t                instructions;       // Total number of instructions
    double                  time;               // Total time

    bare6502_state_e        state;              // Current state
    bare6502_type_e         type;               // Chip type

    uint16_t                ea;                 // Effective address
    uint8_t                 operand;            // Operand
    uint8_t                 operand2;           // Operand 2

    bare6502_opcode_t*      opcode;             // Current opcode
    uint8_t                 opcode_byte;        // Current opcode byte
    uint16_t                opcode_address;     // Address of opcode

    // Registers
    union {
        uint16_t    pc;
        uint8_t     pc_l;
        uint8_t     pc_h;
    };

    union {
        uint8_t     p;
        struct {
            uint8_t     c   : 1;
            uint8_t     z   : 1;
            uint8_t     i   : 1;
            uint8_t     d   : 1;
            uint8_t     b   : 1;
            uint8_t     res : 1;
            uint8_t     v   : 1;
            uint8_t     n   : 1;
        };
    };

    bus_t       bus;

    uint8_t     sp;
    uint8_t     a;
    uint8_t     x;
    uint8_t     y;

    void        (*step)(bare6502_t* chip);
};

enum bare6502_address_mode {
    IMP,    // Implied
    ACC,    // Accumulator
    IMM,    // Immediate
    ZPG,    // Zero Page
    ZPX,    // Zero Page X
    ZPY,    // Zero Page Y
    REL,    // Relative
    ABS,    // Absolute
    ABX,    // Absolute X
    ABY,    // Absolute Y
    IND,    // Indirect
    IDX,    // Indirect X
    IDY,    // Indirect Y
    IDZ,    // Indirect Zero Page

    ADDRESS_MODE_MAX,
};

enum bare6502_opcode {
    ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI,
    BNE, BPL, BRK, BVC, BVS, CLC, CLD, CLI,
    CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR,
    INC, INX, INY, JMP, JSR, LDA, LDX, LDY,
    LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL,
    ROR, RTI, RTS, SBC, SEC, SED, SEI, STA,
    STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
    NID, TRB, TSB, STZ, BRA, PHY, PLY, PHX,
    PLX,

    BBR0, BBR1, BBR2, BBR3, BBR4, BBR5, BBR6, BBR7,
    BBS0, BBS1, BBS2, BBS3, BBS4, BBS5, BBS6, BBS7,
    SMB0, SMB1, SMB2, SMB3, SMB4, SMB5, SMB6, SMB7,
    RMB0, RMB1, RMB2, RMB3, RMB4, RMB5, RMB6, RMB7,

    OPCODE_MAX,
};

struct bare6502_opcode_entry {
    const char*             mnemonic;
    bare6502_opcode_e       opcode;
    bare6502_address_mode_e mode;
};

extern bool                 bare6502_opcode_has_operand[OPCODE_MAX];
extern bare6502_opcode_t    bare6502_opcode_matrix[CHIP_TYPE_MAX][256];

extern bare6502_t*      bare6502_alloc();
extern void             bare6502_free(bare6502_t* chip);
extern void             bare6502_init(bare6502_t* chip, bare6502_type_e type);
