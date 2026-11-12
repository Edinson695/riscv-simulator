#include "decoder.hpp"

namespace {

// Extrae los bits [hi:lo] de un word.
constexpr uint32_t bits(uint32_t word, int hi, int lo) {
    return (word >> lo) & ((1u << (hi - lo + 1)) - 1);
}

// Extiende el signo de un valor de `width` bits a 32 bits.
constexpr int32_t sign_extend(uint32_t value, int width) {
    const uint32_t sign_bit = 1u << (width - 1);
    return static_cast<int32_t>((value ^ sign_bit) - sign_bit);
}

// Inmediatos de cada formato. A la derecha, qué bits de la instrucción forman
// el inmediato, del más significativo al menos significativo.

constexpr int32_t imm_i(uint32_t raw) {  // inst[31:20]
    return sign_extend(bits(raw, 31, 20), 12);
}

constexpr int32_t imm_s(uint32_t raw) {  // inst[31:25] inst[11:7]
    return sign_extend((bits(raw, 31, 25) << 5) | bits(raw, 11, 7), 12);
}

constexpr int32_t imm_b(uint32_t raw) {  // inst[31] inst[7] inst[30:25] inst[11:8] 0
    return sign_extend((bits(raw, 31, 31) << 12) | (bits(raw, 7, 7) << 11) |
                           (bits(raw, 30, 25) << 5) | (bits(raw, 11, 8) << 1),
                       13);
}

constexpr int32_t imm_u(uint32_t raw) {  // inst[31:12] seguido de 12 ceros
    return static_cast<int32_t>(raw & 0xFFFFF000u);
}

constexpr int32_t imm_j(uint32_t raw) {  // inst[31] inst[19:12] inst[20] inst[30:21] 0
    return sign_extend((bits(raw, 31, 31) << 20) | (bits(raw, 19, 12) << 12) |
                           (bits(raw, 20, 20) << 11) | (bits(raw, 30, 21) << 1),
                       21);
}

}  // namespace

Instruction decode(uint32_t raw) {
    Instruction inst;
    inst.raw = raw;
    inst.rd = static_cast<uint8_t>(bits(raw, 11, 7));
    inst.rs1 = static_cast<uint8_t>(bits(raw, 19, 15));
    inst.rs2 = static_cast<uint8_t>(bits(raw, 24, 20));

    const uint32_t opcode = bits(raw, 6, 0);
    const uint32_t funct3 = bits(raw, 14, 12);
    const uint32_t funct7 = bits(raw, 31, 25);

    switch (opcode) {
        case 0b0000011: {  // Cargas
            static constexpr Op ops[8] = {Op::LB,  Op::LH,  Op::LW,      Op::INVALID,
                                          Op::LBU, Op::LHU, Op::INVALID, Op::INVALID};
            inst.op = ops[funct3];
            inst.imm = imm_i(raw);
            break;
        }

        case 0b0010011: {  // Aritmética y lógica con inmediato
            static constexpr Op ops[8] = {Op::ADDI, Op::SLLI, Op::SLTI, Op::SLTIU,
                                          Op::XORI, Op::SRLI, Op::ORI,  Op::ANDI};
            inst.op = ops[funct3];
            inst.imm = imm_i(raw);

            // En los desplazamientos el inmediato es el shamt (5 bits, en el campo
            // de rs2) y funct7 distingue el lógico del aritmético.
            if (funct3 == 0b001 || funct3 == 0b101) {
                inst.imm = static_cast<int32_t>(bits(raw, 24, 20));

                if (funct3 == 0b101 && funct7 == 0b0100000)
                    inst.op = Op::SRAI;
                else if (funct7 != 0b0000000)
                    inst.op = Op::INVALID;
            }
            break;
        }

        case 0b0010111:
            inst.op = Op::AUIPC;
            inst.imm = imm_u(raw);
            break;

        case 0b0110111:
            inst.op = Op::LUI;
            inst.imm = imm_u(raw);
            break;

        case 0b0100011: {  // Almacenamiento
            static constexpr Op ops[8] = {Op::SB,      Op::SH,      Op::SW,      Op::INVALID,
                                          Op::INVALID, Op::INVALID, Op::INVALID, Op::INVALID};
            inst.op = ops[funct3];
            inst.imm = imm_s(raw);
            break;
        }

        case 0b0110011: {  // Aritmética y lógica entre registros
            if (funct7 == 0b0000000) {
                static constexpr Op ops[8] = {Op::ADD, Op::SLL, Op::SLT, Op::SLTU,
                                              Op::XOR, Op::SRL, Op::OR,  Op::AND};
                inst.op = ops[funct3];
            } else if (funct7 == 0b0100000 && funct3 == 0b000) {
                inst.op = Op::SUB;
            } else if (funct7 == 0b0100000 && funct3 == 0b101) {
                inst.op = Op::SRA;
            }
            break;
        }

        case 0b1100011: {  // Saltos condicionales
            static constexpr Op ops[8] = {Op::BEQ, Op::BNE, Op::INVALID, Op::INVALID,
                                          Op::BLT, Op::BGE, Op::BLTU,    Op::BGEU};
            inst.op = ops[funct3];
            inst.imm = imm_b(raw);
            break;
        }

        case 0b1100111:
            if (funct3 == 0b000)
                inst.op = Op::JALR;
            inst.imm = imm_i(raw);
            break;

        case 0b1101111:
            inst.op = Op::JAL;
            inst.imm = imm_j(raw);
            break;

        default:
            break;  // Opcode desconocido: queda como INVALID.
    }

    return inst;
}

const char* mnemonic(Op op) {
    switch (op) {
        case Op::LB:    return "lb";
        case Op::LH:    return "lh";
        case Op::LW:    return "lw";
        case Op::LBU:   return "lbu";
        case Op::LHU:   return "lhu";
        case Op::ADDI:  return "addi";
        case Op::SLLI:  return "slli";
        case Op::SLTI:  return "slti";
        case Op::SLTIU: return "sltiu";
        case Op::XORI:  return "xori";
        case Op::SRLI:  return "srli";
        case Op::SRAI:  return "srai";
        case Op::ORI:   return "ori";
        case Op::ANDI:  return "andi";
        case Op::AUIPC: return "auipc";
        case Op::LUI:   return "lui";
        case Op::SB:    return "sb";
        case Op::SH:    return "sh";
        case Op::SW:    return "sw";
        case Op::ADD:   return "add";
        case Op::SUB:   return "sub";
        case Op::SLL:   return "sll";
        case Op::SLT:   return "slt";
        case Op::SLTU:  return "sltu";
        case Op::XOR:   return "xor";
        case Op::SRL:   return "srl";
        case Op::SRA:   return "sra";
        case Op::OR:    return "or";
        case Op::AND:   return "and";
        case Op::BEQ:   return "beq";
        case Op::BNE:   return "bne";
        case Op::BLT:   return "blt";
        case Op::BGE:   return "bge";
        case Op::BLTU:  return "bltu";
        case Op::BGEU:  return "bgeu";
        case Op::JALR:  return "jalr";
        case Op::JAL:   return "jal";
        case Op::INVALID: break;
    }
    return "invalida";
}
