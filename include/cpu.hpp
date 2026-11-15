#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "memory.hpp"

// Estado arquitectural de una CPU RISC-V de 32 bits: PC, 32 registros y memoria.
class Cpu {
public:
    static constexpr uint32_t LOAD_ADDRESS = 0x00000000;

    uint32_t pc() const { return pc_; }
    void set_pc(uint32_t value) { pc_ = value; }

    // Lee el registro x<index> (0-31).
    uint32_t reg(uint32_t index) const { return regs_[index & 31]; }

    // Escribe el registro x<index>. x0 siempre vale 0: escribir en él no hace nada.
    void set_reg(uint32_t index, uint32_t value) {
        if ((index & 31) != 0)
            regs_[index & 31] = value;
    }

    Memory& memory() { return memory_; }
    const Memory& memory() const { return memory_; }

    std::size_t load_program(const std::string& path);

    void reset();

    bool step();

    const std::string& error() const { return error_; }

    uint64_t steps() const { return steps_; }

private:
    uint32_t pc_ = 0;
    std::array<uint32_t, 32> regs_{};
    Memory memory_;
    std::string error_;
    uint64_t steps_ = 0;
};
