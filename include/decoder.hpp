#pragma once

#include <cstdint>

enum class Op {

    LB, LH, LW, LBU, LHU,

    ADDI, SLLI, SLTI, SLTIU, XORI, SRLI, SRAI, ORI, ANDI,

    AUIPC, LUI,

    SB, SH, SW,

    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,

    BEQ, BNE, BLT, BGE, BLTU, BGEU,

    JALR, JAL,

    INVALID,
};

struct Instruction {
    Op op = Op::INVALID;
    uint32_t raw = 0;
    uint8_t rd = 0;    
    uint8_t rs1 = 0;   
    uint8_t rs2 = 0;   

    // Inmediato con el signo ya extendido. Su significado depende del formato:
    //   I, S: valor de 12 bits con signo.
    //   B, J: desplazamiento en bytes relativo al PC de la instrucción.
    //   U:    los 20 bits altos ya desplazados (imm << 12).
    //   slli/srli/srai: cantidad de bits a desplazar (0-31).
    int32_t imm = 0;
};

// Decodifica un word de 32 bits. Si no corresponde a ninguna de las 37
// instrucciones, devuelve una Instruction con op == Op::INVALID.
Instruction decode(uint32_t raw);

const char* mnemonic(Op op);
