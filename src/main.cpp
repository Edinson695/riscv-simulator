// Simulador RISC-V (RV32I) - CS3051 Tarea 5
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iostream>

#include "cpu.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    // Para que las tildes se vean bien en la consola de Windows.
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc < 2) {
        std::cerr << "Uso: riscv-sim <programa.bin>\n";
        return 1;
    }

    Cpu cpu;

    try {
        std::size_t size = cpu.load_program(argv[1]);
        std::cout << "\"" << argv[1] << "\" cargado a memoria (" << size << " bytes).\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << ".\n";
        return 1;
    }

    // Verificación temporal de la Fase 1: mostrar las primeras instrucciones tal
    // como quedaron en memoria. Se reemplaza por la terminal de comandos (Fase 4).
    std::printf("pc = 0x%08X\n", static_cast<unsigned>(cpu.pc()));
    for (uint32_t addr = 0; addr < 32; addr += 4) {
        std::printf("0x%08X: %08X\n", static_cast<unsigned>(addr),
                    static_cast<unsigned>(cpu.memory().read32(addr)));
    }

    return 0;
}
