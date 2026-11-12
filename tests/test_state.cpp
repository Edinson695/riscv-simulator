// Pruebas de la Fase 1: memoria, registros y carga de programas.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>

#include "cpu.hpp"

static int failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::printf("FALLO (linea %d): %s\n", __LINE__, #cond);        \
            failures++;                                                    \
        }                                                                  \
    } while (0)

static void test_memory() {
    Memory mem;

    // Memoria sin tocar: lee 0 y no reserva páginas.
    CHECK(mem.read32(0x12345678) == 0);
    CHECK(mem.page_count() == 0);

    // Little-endian: el byte menos significativo va en la dirección más baja.
    mem.write32(0x1000, 0x10200405);
    CHECK(mem.read8(0x1000) == 0x05);
    CHECK(mem.read8(0x1001) == 0x04);
    CHECK(mem.read8(0x1002) == 0x20);
    CHECK(mem.read8(0x1003) == 0x10);
    CHECK(mem.read16(0x1000) == 0x0405);
    CHECK(mem.read16(0x1002) == 0x1020);
    CHECK(mem.read32(0x1000) == 0x10200405);

    mem.write16(0x2000, 0xBEEF);
    CHECK(mem.read8(0x2000) == 0xEF);
    CHECK(mem.read8(0x2001) == 0xBE);

    mem.write8(0x2002, 0x7C);
    CHECK(mem.read32(0x2000) == 0x007CBEEF);

    // Un word que cruza el límite entre dos páginas.
    mem.write32(0x3FFE, 0xAABBCCDD);
    CHECK(mem.read32(0x3FFE) == 0xAABBCCDD);
    CHECK(mem.read8(0x3FFF) == 0xCC);
    CHECK(mem.read8(0x4000) == 0xBB);

    // Direcciones altas (por ejemplo, un stack en el tope del espacio).
    mem.write32(0xFFFFFFF0, 0xDEADBEEF);
    CHECK(mem.read32(0xFFFFFFF0) == 0xDEADBEEF);

    // En la última dirección el acceso da la vuelta a la dirección 0.
    mem.write32(0xFFFFFFFE, 0x11223344);
    CHECK(mem.read8(0xFFFFFFFE) == 0x44);
    CHECK(mem.read8(0xFFFFFFFF) == 0x33);
    CHECK(mem.read8(0x00000000) == 0x22);
    CHECK(mem.read8(0x00000001) == 0x11);

    mem.clear();
    CHECK(mem.page_count() == 0);
    CHECK(mem.read32(0x1000) == 0);
}

static void test_registers() {
    Cpu cpu;

    CHECK(cpu.pc() == 0);
    for (uint32_t i = 0; i < 32; i++)
        CHECK(cpu.reg(i) == 0);

    cpu.set_reg(5, 0x123);
    cpu.set_reg(31, 0xFFFFFFFF);
    CHECK(cpu.reg(5) == 0x123);
    CHECK(cpu.reg(31) == 0xFFFFFFFF);

    // x0 siempre vale 0.
    cpu.set_reg(0, 0xDEADBEEF);
    CHECK(cpu.reg(0) == 0);

    cpu.set_pc(0x40);
    cpu.reset();
    CHECK(cpu.pc() == 0);
    CHECK(cpu.reg(5) == 0);
}

static void test_loader() {
    const char* path = "test_state_tmp.bin";

    // Incluye 0x1A y 0x0D 0x0A, que se corrompen si el archivo no se abre en modo binario.
    const unsigned char bytes[] = {0x13, 0x01, 0x50, 0x00, 0x1A, 0x0D, 0x0A, 0xFF};
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    }

    Cpu cpu;
    cpu.set_reg(7, 99);
    cpu.memory().write32(0x5000, 0xCAFEBABE);

    CHECK(cpu.load_program(path) == sizeof(bytes));
    CHECK(cpu.memory().read32(0) == 0x00500113);  // addi x2, x0, 5
    CHECK(cpu.memory().read32(4) == 0xFF0A0D1A);
    CHECK(cpu.memory().read32(8) == 0);

    // Cargar un programa reinicia el estado anterior.
    CHECK(cpu.pc() == 0);
    CHECK(cpu.reg(7) == 0);
    CHECK(cpu.memory().read32(0x5000) == 0);

    std::remove(path);

    bool threw = false;
    try {
        cpu.load_program("archivo_que_no_existe.bin");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
}

int main() {
    test_memory();
    test_registers();
    test_loader();

    if (failures == 0)
        std::printf("Todas las pruebas pasaron.\n");
    else
        std::printf("%d prueba(s) fallaron.\n", failures);

    return failures == 0 ? 0 : 1;
}
