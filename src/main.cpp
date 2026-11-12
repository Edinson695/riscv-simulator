// Simulador RISC-V (RV32I) - CS3051 Tarea 5
#include <exception>
#include <iostream>

#include "cli.hpp"
#include "cpu.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    // Para que las tildes se vean bien en la consola de Windows.
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc != 2) {
        std::cerr << "Uso: riscv-sim <programa.bin>\n";
        return 1;
    }

    Cpu cpu;

    try {
        cpu.load_program(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << ".\n";
        return 1;
    }

    std::cout << "\"" << argv[1] << "\" cargado a memoria.\n";

    Cli cli(cpu, argv[1], std::cin, std::cout);
    cli.run();

    return 0;
}
