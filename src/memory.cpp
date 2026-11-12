#include "memory.hpp"

uint8_t Memory::read8(uint32_t addr) const {
    auto it = pages_.find(addr >> PAGE_BITS);
    if (it == pages_.end())
        return 0;

    return (*it->second)[addr & (PAGE_SIZE - 1)];
}

void Memory::write8(uint32_t addr, uint8_t value) {
    auto& page = pages_[addr >> PAGE_BITS];
    if (!page)
        page = std::make_unique<Page>();  // Página nueva, inicializada en ceros.

    (*page)[addr & (PAGE_SIZE - 1)] = value;
}

// Los accesos de 16 y 32 bits se arman byte por byte. Así el orden little-endian
// queda explícito (el byte menos significativo va en la dirección más baja), no
// depende del endianness de la máquina host y funciona aunque el dato cruce el
// límite entre dos páginas.

uint16_t Memory::read16(uint32_t addr) const {
    return static_cast<uint16_t>(read8(addr) | (read8(addr + 1) << 8));
}

uint32_t Memory::read32(uint32_t addr) const {
    return static_cast<uint32_t>(read8(addr)) |
           (static_cast<uint32_t>(read8(addr + 1)) << 8) |
           (static_cast<uint32_t>(read8(addr + 2)) << 16) |
           (static_cast<uint32_t>(read8(addr + 3)) << 24);
}

void Memory::write16(uint32_t addr, uint16_t value) {
    write8(addr, static_cast<uint8_t>(value));
    write8(addr + 1, static_cast<uint8_t>(value >> 8));
}

void Memory::write32(uint32_t addr, uint32_t value) {
    write8(addr, static_cast<uint8_t>(value));
    write8(addr + 1, static_cast<uint8_t>(value >> 8));
    write8(addr + 2, static_cast<uint8_t>(value >> 16));
    write8(addr + 3, static_cast<uint8_t>(value >> 24));
}

void Memory::load(uint32_t addr, const uint8_t* data, std::size_t size) {
    for (std::size_t i = 0; i < size; i++)
        write8(addr + static_cast<uint32_t>(i), data[i]);
}

void Memory::clear() {
    pages_.clear();
}

std::size_t Memory::page_count() const {
    return pages_.size();
}
