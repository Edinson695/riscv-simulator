#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "cpu.hpp"

// Terminal interactiva del simulador. Lee comandos línea por línea y deja que el
// usuario controle la ejecución e inspeccione el estado de la CPU.
//
// Recibe los streams por parámetro para poder probarla sin teclado ni pantalla.
class Cli {
public:
    Cli(Cpu& cpu, std::string program_path, std::istream& in, std::ostream& out);

    // Atiende comandos hasta recibir "exit" o que se acabe la entrada.
    void run();

    // Ejecuta una sola línea de comando. Devuelve false si el comando pide salir.
    bool execute(const std::string& line);

private:
    void cmd_pc();
    void cmd_step(const std::vector<std::string>& args);
    void cmd_run();
    void cmd_regs(const std::vector<std::string>& args);
    void cmd_mem(const std::vector<std::string>& args);
    void cmd_reset();
    void cmd_help();

    Cpu& cpu_;
    std::string program_path_;
    std::istream& in_;
    std::ostream& out_;
};
