#include "cpu.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

void Cpu::reset() {
    pc_ = 0;
    regs_.fill(0);
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
