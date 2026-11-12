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

    // Verificación temporal de la Fase 3: ejecutar hasta que el programa salte a
    // su propia dirección (así terminan los programas de prueba) y mostrar el
    // estado final. Se reemplaza por la terminal de comandos (Fase 4).
    const uint64_t MAX_STEPS = 10'000'000;
    bool finished = false;

    while (cpu.steps() < MAX_STEPS) {
        const uint32_t before = cpu.pc();
        if (!cpu.step()) {
            std::cerr << "Error: " << cpu.error() << ".\n";
            break;
        }
        if (cpu.pc() == before) {
            finished = true;
            break;
        }
    }

    std::cout << (finished ? "Programa terminado" : "Ejecucion detenida") << " tras "
              << cpu.steps() << " instrucciones.\n";
    std::printf("pc  = 0x%08X\n", static_cast<unsigned>(cpu.pc()));
    for (uint32_t i = 1; i < 32; i++) {
        if (cpu.reg(i) != 0)
            std::printf("x%-2u = 0x%08X\n", static_cast<unsigned>(i), static_cast<unsigned>(cpu.reg(i)));
    }

    return 0;
}
