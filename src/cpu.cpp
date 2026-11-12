#include "cpu.hpp"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "decoder.hpp"

void Cpu::reset() {
    pc_ = 0;
    regs_.fill(0);
    error_.clear();
    steps_ = 0;
}

std::size_t Cpu::load_program(const std::string& path) {
    // std::ios::binary es indispensable en Windows: sin él, el byte 0x1A se toma
    // como fin de archivo y los 0x0D 0x0A se convierten en 0x0A.
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("no se pudo abrir \"" + path + "\"");

    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

    if (bytes.empty())
        throw std::runtime_error("\"" + path + "\" está vacío");
    if (bytes.size() > 0x100000000ull)
        throw std::runtime_error("\"" + path + "\" no entra en un espacio de 32 bits");

    reset();
    memory_.clear();
    memory_.load(LOAD_ADDRESS, bytes.data(), bytes.size());

    return bytes.size();
}

bool Cpu::step() {
    error_.clear();

    char msg[96];

    // Las instrucciones ocupan 4 bytes y deben estar alineadas a 4.
    if (pc_ % 4 != 0) {
        std::snprintf(msg, sizeof(msg), "PC desalineado (pc = 0x%08X)", static_cast<unsigned>(pc_));
        error_ = msg;
        return false;
    }

    // Fetch y decode.
    const Instruction inst = decode(memory_.read32(pc_));

    if (inst.op == Op::INVALID) {
        std::snprintf(msg, sizeof(msg), "instruccion invalida 0x%08X en pc = 0x%08X",
                      static_cast<unsigned>(inst.raw), static_cast<unsigned>(pc_));
        error_ = msg;
        return false;
    }

    // Operandos. Las versiones con signo se usan en comparaciones y en el
    // desplazamiento aritmético; todo lo demás se opera sin signo, de modo que
    // sumas y restas dan la vuelta en 32 bits como en el hardware.
    const uint32_t a = reg(inst.rs1);
    const uint32_t b = reg(inst.rs2);
    const uint32_t imm = static_cast<uint32_t>(inst.imm);
    const int32_t sa = static_cast<int32_t>(a);
    const int32_t sb = static_cast<int32_t>(b);

    // Por defecto se avanza a la siguiente instrucción; los saltos lo cambian.
    uint32_t next_pc = pc_ + 4;

    switch (inst.op) {
        // --- Cargas: rd = memoria[rs1 + imm] ---
        // lb y lh extienden el signo; lbu y lhu rellenan con ceros.
        case Op::LB:
            set_reg(inst.rd, static_cast<uint32_t>(static_cast<int8_t>(memory_.read8(a + imm))));
            break;
        case Op::LH:
            set_reg(inst.rd, static_cast<uint32_t>(static_cast<int16_t>(memory_.read16(a + imm))));
            break;
        case Op::LW:
            set_reg(inst.rd, memory_.read32(a + imm));
            break;
        case Op::LBU:
            set_reg(inst.rd, memory_.read8(a + imm));
            break;
        case Op::LHU:
            set_reg(inst.rd, memory_.read16(a + imm));
            break;

        // --- Aritmética y lógica con inmediato ---
        case Op::ADDI:
            set_reg(inst.rd, a + imm);
            break;
        case Op::SLTI:
            set_reg(inst.rd, sa < inst.imm ? 1 : 0);
            break;
        case Op::SLTIU:
            // El inmediato ya viene con el signo extendido; se compara sin signo.
            set_reg(inst.rd, a < imm ? 1 : 0);
            break;
        case Op::XORI:
            set_reg(inst.rd, a ^ imm);
            break;
        case Op::ORI:
            set_reg(inst.rd, a | imm);
            break;
        case Op::ANDI:
            set_reg(inst.rd, a & imm);
            break;
        case Op::SLLI:
            set_reg(inst.rd, a << imm);  // imm es el shamt (0-31).
            break;
        case Op::SRLI:
            set_reg(inst.rd, a >> imm);
            break;
        case Op::SRAI:
            // En C++20, >> sobre un entero con signo es aritmético (copia el bit de signo).
            set_reg(inst.rd, static_cast<uint32_t>(sa >> imm));
            break;

        // --- Formato U ---
        case Op::LUI:
            set_reg(inst.rd, imm);
            break;
        case Op::AUIPC:
            set_reg(inst.rd, pc_ + imm);
            break;

        // --- Almacenamiento: memoria[rs1 + imm] = rs2 ---
        case Op::SB:
            memory_.write8(a + imm, static_cast<uint8_t>(b));
            break;
        case Op::SH:
            memory_.write16(a + imm, static_cast<uint16_t>(b));
            break;
        case Op::SW:
            memory_.write32(a + imm, b);
            break;

        // --- Aritmética y lógica entre registros ---
        // Los desplazamientos usan solo los 5 bits bajos de rs2.
        case Op::ADD:
            set_reg(inst.rd, a + b);
            break;
        case Op::SUB:
            set_reg(inst.rd, a - b);
            break;
        case Op::SLL:
            set_reg(inst.rd, a << (b & 31));
            break;
        case Op::SLT:
            set_reg(inst.rd, sa < sb ? 1 : 0);
            break;
        case Op::SLTU:
            set_reg(inst.rd, a < b ? 1 : 0);
            break;
        case Op::XOR:
            set_reg(inst.rd, a ^ b);
            break;
        case Op::SRL:
            set_reg(inst.rd, a >> (b & 31));
            break;
        case Op::SRA:
            set_reg(inst.rd, static_cast<uint32_t>(sa >> (b & 31)));
            break;
        case Op::OR:
            set_reg(inst.rd, a | b);
            break;
        case Op::AND:
            set_reg(inst.rd, a & b);
            break;

        // --- Saltos condicionales: el desplazamiento es relativo al PC actual ---
        case Op::BEQ:
            if (a == b) next_pc = pc_ + imm;
            break;
        case Op::BNE:
            if (a != b) next_pc = pc_ + imm;
            break;
        case Op::BLT:
            if (sa < sb) next_pc = pc_ + imm;
            break;
        case Op::BGE:
            if (sa >= sb) next_pc = pc_ + imm;
            break;
        case Op::BLTU:
            if (a < b) next_pc = pc_ + imm;
            break;
        case Op::BGEU:
            if (a >= b) next_pc = pc_ + imm;
            break;

        // --- Saltos incondicionales: rd = dirección de retorno (PC + 4) ---
        case Op::JAL:
            set_reg(inst.rd, pc_ + 4);
            next_pc = pc_ + imm;
            break;
        case Op::JALR:
            // El destino se calcula antes de escribir rd (pueden ser el mismo
            // registro) y se le pone el bit 0 en cero, como indica la especificación.
            next_pc = (a + imm) & ~1u;
            set_reg(inst.rd, pc_ + 4);
            break;

        case Op::INVALID:
            break;  // Ya se descartó arriba.
    }

    pc_ = next_pc;
    steps_++;
    return true;
}
