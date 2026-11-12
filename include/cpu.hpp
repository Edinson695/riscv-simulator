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

    // Reinicia PC, registros y memoria, y carga un binario crudo en LOAD_ADDRESS.
    // Devuelve la cantidad de bytes cargados. Lanza std::runtime_error si el
    // archivo no se puede abrir, está vacío o no entra en el espacio de 32 bits.
    std::size_t load_program(const std::string& path);

    // PC y registros a cero. No toca la memoria.
    void reset();

    // Ejecuta la instrucción apuntada por el PC: la lee de memoria, la decodifica,
    // aplica su efecto y actualiza el PC.
    //
    // Devuelve false si no se pudo ejecutar (instrucción inválida o PC desalineado).
    // En ese caso el estado no cambia y error() describe el problema.
    bool step();

    // Descripción del último error de step(). Vacío si no hubo error.
    const std::string& error() const { return error_; }

    // Cantidad de instrucciones ejecutadas desde la última carga o reset.
    uint64_t steps() const { return steps_; }

private:
    uint32_t pc_ = 0;
    std::array<uint32_t, 32> regs_{};
    Memory memory_;
    std::string error_;
    uint64_t steps_ = 0;
};
