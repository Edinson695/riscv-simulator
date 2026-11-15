#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

class Memory {
public:
    static constexpr uint32_t PAGE_BITS = 12;
    static constexpr uint32_t PAGE_SIZE = 1u << PAGE_BITS;  // 4096 bytes

    uint8_t read8(uint32_t addr) const;
    uint16_t read16(uint32_t addr) const;
    uint32_t read32(uint32_t addr) const;

    void write8(uint32_t addr, uint8_t value);
    void write16(uint32_t addr, uint16_t value);
    void write32(uint32_t addr, uint32_t value);

    // Copia `size` bytes a partir de la dirección `addr`.
    void load(uint32_t addr, const uint8_t* data, std::size_t size);

    void clear();

    std::size_t page_count() const;

private:
    using Page = std::array<uint8_t, PAGE_SIZE>;

    std::unordered_map<uint32_t, std::unique_ptr<Page>> pages_;
};
