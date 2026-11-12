#pragma once

#include <cstdint>

// Las 37 instrucciones del anexo A, más INVALID para lo que no se reconoce.
enum class Op {
    // Cargas (formato I)
    LB, LH, LW, LBU, LHU,
    // Aritmética y lógica con inmediato (formato I)
    ADDI, SLLI, SLTI, SLTIU, XORI, SRLI, SRAI, ORI, ANDI,
    // Formato U
    AUIPC, LUI,
    // Almacenamiento (formato S)
    SB, SH, SW,
    // Aritmética y lógica entre registros (formato R)
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
    // Saltos condicionales (formato B)
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    // Saltos incondicionales (formatos I y J)
    JALR, JAL,

    INVALID,
};

// Una instrucción ya separada en sus campos.
struct Instruction {
    Op op = Op::INVALID;
    uint32_t raw = 0;  // Word original de 32 bits.
    uint8_t rd = 0;    // Registro destino.
    uint8_t rs1 = 0;   // Primer registro fuente.
    uint8_t rs2 = 0;   // Segundo registro fuente.

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

// Nombre de la instrucción en minúsculas ("addi", "beq", ...).
const char* mnemonic(Op op);
