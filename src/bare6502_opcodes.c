#include <bare6502.h>

bool bare6502_opcode_has_operand[OPCODE_MAX] = {
//  ADC,   AND,   ASL,   BCC,   BCS,   BEQ,   BIT,   BMI
    true,  true,  true,  true,  true,  true,  true,  true,
//  BNE,   BPL,   BRK,   BVC,   BVS,   CLC,   CLD,   CLI
    true,  true,  false, true,  true,  false, false, false,
//  CLV,   CMP,   CPX,   CPY,   DEC,   DEX,   DEY,   EOR
    false, true,  true,  true,  true,  false, false, true,
//  INC,   INX,   INY,   JMP,   JSR,   LDA,   LDX,   LDY
    true,  false, false, true,  true,  true,  true,  true,
//  LSR,   NOP,   ORA,   PHA,   PHP,   PLA,   PLP,   ROL
    true,  false, true,  false, false, false, false, true,
//  ROR,   RTI,   RTS,   SBC,   SEC,   SED,   SEI,   STA
    true,  false, false, true,  false, false, false, true,
//  STX,   STY,   TAX,   TAY,   TSX,   TXA,   TXS,   TYA
    true,  true,  false, false, false, false, false, false,
//  NID,   TRB,   TSB,   STZ,   BRA,   PHY,   PLY,   PHX
    true,  true,  true,  true,  true,  false, false, false,
//  PLX,
    false,

//  BBR0,  BBR1,  BBR2,  BBR3,  BBR4,  BBR5,  BBR6,  BBR7
    true,  true,  true,  true,  true,  true,  true,  true,
//  BBS0,  BBS1,  BBS2,  BBS3,  BBS4,  BBS5,  BBS6,  BBS7
    true,  true,  true,  true,  true,  true,  true,  true,
//  SMB0,  SMB1,  SMB2,  SMB3,  SMB4,  SMB5,  SMB6,  SMB7
    true,  true,  true,  true,  true,  true,  true,  true,
//  RMB0,  RMB1,  RMB2,  RMB3,  RMB4,  RMB5,  RMB6,  RMB7
    true,  true,  true,  true,  true,  true,  true,  true,
};

bare6502_opcode_t bare6502_opcode_matrix[CHIP_TYPE_MAX][256] = {
    #define OP(n, m)    { .mnemonic = #n", "#m, .opcode = n, .mode = m }

    [MOS6502] = {
    //  00            01            02            03            04            05            06            07            08            09            0A            0B            0C            0D            0E            0F
        OP(BRK, IMP), OP(ORA, IDX), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(ORA, ZPG), OP(ASL, ZPG), OP(NID, IMP), OP(PHP, IMP), OP(ORA, IMM), OP(ASL, ACC), OP(NID, IMP), OP(NID, IMP), OP(ORA, ABS), OP(ASL, ABS), OP(NID, IMP),
    //  10            11            12            13            14            15            16            17            18            19            1A            1B            1C            1D            1E            1F
        OP(BPL, REL), OP(ORA, IDY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(ORA, ZPX), OP(ASL, ZPX), OP(NID, IMP), OP(CLC, IMP), OP(ORA, ABY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(ORA, ABX), OP(ASL, ABX), OP(NID, IMP),
    //  20            21            22            23            24            25            26            27            28            29            2A            2B            2C            2D            2E            2F
        OP(JSR, ABS), OP(AND, IDX), OP(NID, IMP), OP(NID, IMP), OP(BIT, ZPG), OP(AND, ZPG), OP(ROL, ZPG), OP(NID, IMP), OP(PLP, IMP), OP(AND, IMM), OP(ROL, ACC), OP(NID, IMP), OP(BIT, ABS), OP(AND, ABS), OP(ROL, ABS), OP(NID, IMP),
    //  30            31            32            33            34            35            36            37            38            39            3A            3B            3C            3D            3E            3F
        OP(BMI, REL), OP(AND, IDY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(AND, ZPX), OP(ROL, ZPX), OP(NID, IMP), OP(SEC, IMP), OP(AND, ABY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(AND, ABX), OP(ROL, ABX), OP(NID, IMP),
    //  40            41            42            43            44            45            46            47            48            49            4A            4B            4C            4D            4E            4F
        OP(RTI, IMP), OP(EOR, IDX), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(EOR, ZPG), OP(LSR, ZPG), OP(NID, IMP), OP(PHA, IMP), OP(EOR, IMM), OP(LSR, ACC), OP(NID, IMP), OP(JMP, ABS), OP(EOR, ABS), OP(LSR, ABS), OP(NID, IMP),
    //  50            51            52            53            54            55            56            57            58            59            5A            5B            5C            5D            5E            5F
        OP(BVC, REL), OP(EOR, IDY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(EOR, ZPX), OP(LSR, ZPX), OP(NID, IMP), OP(CLI, IMP), OP(EOR, ABY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(EOR, ABX), OP(LSR, ABX), OP(NID, IMP),
    //  60            61            62            63            64            65            66            67            68            69            6A            6B            6C            6D            6E            6F
        OP(RTS, IMP), OP(ADC, IDX), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(ADC, ZPG), OP(ROR, ZPG), OP(NID, IMP), OP(PLA, IMP), OP(ADC, IMM), OP(ROR, ACC), OP(NID, IMP), OP(JMP, IND), OP(ADC, ABS), OP(ROR, ABS), OP(NID, IMP),
    //  70            71            72            73            74            75            76            77            78            79            7A            7B            7C            7D            7E            7F
        OP(BVS, REL), OP(ADC, IDY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(ADC, ZPX), OP(ROR, ZPX), OP(NID, IMP), OP(SEI, IMP), OP(ADC, ABY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(ADC, ABX), OP(ROR, ABX), OP(NID, IMP),
    //  80            81            82            83            84            85            86            87            88            89            8A            8B            8C            8D            8E            8F
        OP(NID, IMP), OP(STA, IDX), OP(NID, IMP), OP(NID, IMP), OP(STY, ZPG), OP(STA, ZPG), OP(STX, ZPG), OP(NID, IMP), OP(DEY, IMP), OP(NID, IMP), OP(TXA, IMP), OP(NID, IMP), OP(STY, ABS), OP(STA, ABS), OP(STX, ABS), OP(NID, IMP),
    //  90            91            92            93            94            95            96            97            98            99            9A            9B            9C            9D            9E            9F
        OP(BCC, REL), OP(STA, IDY), OP(NID, IMP), OP(NID, IMP), OP(STY, ZPX), OP(STA, ZPX), OP(STX, ZPY), OP(NID, IMP), OP(TYA, IMP), OP(STA, ABY), OP(TXS, IMP), OP(NID, IMP), OP(STY, ABX), OP(STA, ABX), OP(NID, IMP), OP(NID, IMP),
    //  A0            A1            A2            A3            A4            A5            A6            A7            A8            A9            AA            AB            AC            AD            AE            AF
        OP(LDY, IMM), OP(LDA, IDX), OP(LDX, IMM), OP(NID, IMP), OP(LDY, ZPG), OP(LDA, ZPG), OP(LDX, ZPG), OP(NID, IMP), OP(TAY, IMP), OP(LDA, IMM), OP(TAX, IMP), OP(NID, IMP), OP(LDY, ABS), OP(LDA, ABS), OP(LDX, ABS), OP(NID, IMP),
    //  B0            B1            B2            B3            B4            B5            B6            B7            B8            B9            BA            BB            BC            BD            BE            BF
        OP(BCS, REL), OP(LDA, IDY), OP(NID, IMP), OP(NID, IMP), OP(LDY, ZPX), OP(LDA, ZPX), OP(LDX, ZPY), OP(NID, IMP), OP(CLV, IMP), OP(LDA, ABY), OP(TSX, IMP), OP(NID, IMP), OP(LDY, ABX), OP(LDA, ABX), OP(LDX, ABY), OP(NID, IMP),
    //  C0            C1            C2            C3            C4            C5            C6            C7            C8            C9            CA            CB            CC            CD            CE            CF
        OP(CPY, IMM), OP(CMP, IDX), OP(NID, IMP), OP(NID, IMP), OP(CPY, ZPG), OP(CMP, ZPG), OP(DEC, ZPG), OP(NID, IMP), OP(INY, IMP), OP(CMP, IMM), OP(DEX, IMP), OP(NID, IMP), OP(CPY, ABS), OP(CMP, ABS), OP(DEC, ABS), OP(NID, IMP),
    //  D0            D1            D2            D3            D4            D5            D6            D7            D8            D9            DA            DB            DC            DD            DE            DF
        OP(BNE, REL), OP(CMP, IDY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(CMP, ZPX), OP(DEC, ZPX), OP(NID, IMP), OP(CLD, IMP), OP(CMP, ABY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(CMP, ABX), OP(DEC, ABX), OP(NID, IMP),
    //  E0            E1            E2            E3            E4            E5            E6            E7            E8            E9            EA            EB            EC            ED            EE            EF
        OP(CPX, IMM), OP(SBC, IDX), OP(NID, IMP), OP(NID, IMP), OP(CPX, ZPG), OP(SBC, ZPG), OP(INC, ZPG), OP(NID, IMP), OP(INX, IMP), OP(SBC, IMM), OP(NOP, IMP), OP(SBC, IMM), OP(CPX, ABS), OP(SBC, ABS), OP(INC, ABS), OP(NID, IMP),
    //  F0            F1            F2            F3            F4            F5            F6            F7            F8            F9            FA            FB            FC            FD            FE            FF
        OP(BEQ, REL), OP(SBC, IDY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(SBC, ZPX), OP(INC, ZPX), OP(NID, IMP), OP(SED, IMP), OP(SBC, ABY), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(SBC, ABX), OP(INC, ABX), OP(NID, IMP),
    },

    [MOS65C02] = {
    //  00            01            02            03            04            05            06            07            08            09            0A            0B            0C            0D            0E            0F
        OP(BRK, IMP), OP(ORA, IDX), OP(NID, IMP), OP(NID, IMP), OP(TSB, IDZ), OP(ORA, ZPG), OP(ASL, ZPG), OP(NID, IMP), OP(PHP, IMP), OP(ORA, IMM), OP(ASL, ACC), OP(NID, IMP), OP(TSB, ACC), OP(ORA, ABS), OP(ASL, ABS), OP(BBR0, ZPG),
    //  10            11            12            13            14            15            16            17            18            19            1A            1B            1C            1D            1E            1F
        OP(BPL, REL), OP(ORA, IDY), OP(ORA, IDZ), OP(NID, IMP), OP(TRB, IDZ), OP(ORA, ZPX), OP(ASL, ZPX), OP(NID, IMP), OP(CLC, IMP), OP(ORA, ABY), OP(INC, ACC), OP(NID, IMP), OP(TRB, ACC), OP(ORA, ABX), OP(ASL, ABX), OP(BBR1, ZPG),
    //  20            21            22            23            24            25            26            27            28            29            2A            2B            2C            2D            2E            2F
        OP(JSR, ABS), OP(AND, IDX), OP(NID, IMP), OP(NID, IMP), OP(BIT, ZPG), OP(AND, ZPG), OP(ROL, ZPG), OP(NID, IMP), OP(PLP, IMP), OP(AND, IMM), OP(ROL, ACC), OP(NID, IMP), OP(BIT, ABS), OP(AND, ABS), OP(ROL, ABS), OP(BBR2, ZPG),
    //  30            31            32            33            34            35            36            37            38            39            3A            3B            3C            3D            3E            3F
        OP(BMI, REL), OP(AND, IDY), OP(AND, IDZ), OP(NID, IMP), OP(BIT, ZPX), OP(AND, ZPX), OP(ROL, ZPX), OP(NID, IMP), OP(SEC, IMP), OP(AND, ABY), OP(DEC, ACC), OP(NID, IMP), OP(BIT, ABX), OP(AND, ABX), OP(ROL, ABX), OP(BBR3, ZPG),
    //  40            41            42            43            44            45            46            47            48            49            4A            4B            4C            4D            4E            4F
        OP(RTI, IMP), OP(EOR, IDX), OP(NID, IMP), OP(NID, IMP), OP(NID, IMP), OP(EOR, ZPG), OP(LSR, ZPG), OP(NID, IMP), OP(PHA, IMP), OP(EOR, IMM), OP(LSR, ACC), OP(NID, IMP), OP(JMP, ABS), OP(EOR, ABS), OP(LSR, ABS), OP(BBR4, ZPG),
    //  50            51            52            53            54            55            56            57            58            59            5A            5B            5C            5D            5E            5F
        OP(BVC, REL), OP(EOR, IDY), OP(EOR, IDZ), OP(NID, IMP), OP(NID, IMP), OP(EOR, ZPX), OP(LSR, ZPX), OP(NID, IMP), OP(CLI, IMP), OP(EOR, ABY), OP(PHY, IMP), OP(NID, IMP), OP(NID, IMP), OP(EOR, ABX), OP(LSR, ABX), OP(BBR5, ZPG),
    //  60            61            62            63            64            65            66            67            68            69            6A            6B            6C            6D            6E            6F
        OP(RTS, IMP), OP(ADC, IDX), OP(NID, IMP), OP(NID, IMP), OP(STZ, ZPG), OP(ADC, ZPG), OP(ROR, ZPG), OP(NID, IMP), OP(PLA, IMP), OP(ADC, IMM), OP(ROR, ACC), OP(NID, IMP), OP(JMP, IND), OP(ADC, ABS), OP(ROR, ABS), OP(BBR6, ZPG),
    //  70            71            72            73            74            75            76            77            78            79            7A            7B            7C            7D            7E            7F
        OP(BVS, REL), OP(ADC, IDY), OP(ADC, IDZ), OP(NID, IMP), OP(STZ, ZPX), OP(ADC, ZPX), OP(ROR, ZPX), OP(NID, IMP), OP(SEI, IMP), OP(ADC, ABY), OP(PLY, IMP), OP(NID, IMP), OP(JMP, ABX), OP(ADC, ABX), OP(ROR, ABX), OP(BBR7, ZPG),
    //  80            81            82            83            84            85            86            87            88            89            8A            8B            8C            8D            8E            8F
        OP(BRA, REL), OP(STA, IDX), OP(NID, IMP), OP(NID, IMP), OP(STY, ZPG), OP(STA, ZPG), OP(STX, ZPG), OP(NID, IMP), OP(DEY, IMP), OP(BIT, IMM), OP(TXA, IMP), OP(NID, IMP), OP(STY, ABS), OP(STA, ABS), OP(STX, ABS), OP(BBS0, ZPG),
    //  90            91            92            93            94            95            96            97            98            99            9A            9B            9C            9D            9E            9F
        OP(BCC, REL), OP(STA, IDY), OP(STA, IDZ), OP(NID, IMP), OP(STY, ZPX), OP(STA, ZPX), OP(STX, ZPY), OP(NID, IMP), OP(TYA, IMP), OP(STA, ABY), OP(TXS, IMP), OP(NID, IMP), OP(STZ, ABS), OP(STA, ABX), OP(STZ, ABX), OP(BBS1, ZPG),
    //  A0            A1            A2            A3            A4            A5            A6            A7            A8            A9            AA            AB            AC            AD            AE            AF
        OP(LDY, IMM), OP(LDA, IDX), OP(LDX, IMM), OP(NID, IMP), OP(LDY, ZPG), OP(LDA, ZPG), OP(LDX, ZPG), OP(NID, IMP), OP(TAY, IMP), OP(LDA, IMM), OP(TAX, IMP), OP(NID, IMP), OP(LDY, ABS), OP(LDA, ABS), OP(LDX, ABS), OP(BBS2, ZPG),
    //  B0            B1            B2            B3            B4            B5            B6            B7            B8            B9            BA            BB            BC            BD            BE            BF
        OP(BCS, REL), OP(LDA, IDY), OP(LDA, IDZ), OP(NID, IMP), OP(LDY, ZPX), OP(LDA, ZPX), OP(LDX, ZPY), OP(NID, IMP), OP(CLV, IMP), OP(LDA, ABY), OP(TSX, IMP), OP(NID, IMP), OP(LDY, ABX), OP(LDA, ABX), OP(LDX, ABY), OP(BBS3, ZPG),
    //  C0            C1            C2            C3            C4            C5            C6            C7            C8            C9            CA            CB            CC            CD            CE            CF
        OP(CPY, IMM), OP(CMP, IDX), OP(NID, IMP), OP(NID, IMP), OP(CPY, ZPG), OP(CMP, ZPG), OP(DEC, ZPG), OP(NID, IMP), OP(INY, IMP), OP(CMP, IMM), OP(DEX, IMP), OP(NID, IMP), OP(CPY, ABS), OP(CMP, ABS), OP(DEC, ABS), OP(BBS4, ZPG),
    //  D0            D1            D2            D3            D4            D5            D6            D7            D8            D9            DA            DB            DC            DD            DE            DF
        OP(BNE, REL), OP(CMP, IDY), OP(CMP, IMP), OP(NID, IMP), OP(NID, IMP), OP(CMP, ZPX), OP(DEC, ZPX), OP(NID, IMP), OP(CLD, IMP), OP(CMP, ABY), OP(PHX, IMP), OP(NID, IMP), OP(NID, IMP), OP(CMP, ABX), OP(DEC, ABX), OP(BBS5, ZPG),
    //  E0            E1            E2            E3            E4            E5            E6            E7            E8            E9            EA            EB            EC            ED            EE            EF
        OP(CPX, IMM), OP(SBC, IDX), OP(NID, IMP), OP(NID, IMP), OP(CPX, ZPG), OP(SBC, ZPG), OP(INC, ZPG), OP(NID, IMP), OP(INX, IMP), OP(SBC, IMM), OP(NOP, IMP), OP(SBC, IMM), OP(CPX, ABS), OP(SBC, ABS), OP(INC, ABS), OP(BBS6, ZPG),
    //  F0            F1            F2            F3            F4            F5            F6            F7            F8            F9            FA            FB            FC            FD            FE            FF
        OP(BEQ, REL), OP(SBC, IDY), OP(SBC, IDZ), OP(NID, IMP), OP(NID, IMP), OP(SBC, ZPX), OP(INC, ZPX), OP(NID, IMP), OP(SED, IMP), OP(SBC, ABY), OP(PLX, IMP), OP(NID, IMP), OP(NID, IMP), OP(SBC, ABX), OP(INC, ABX), OP(BBS7, ZPG),
    }
};