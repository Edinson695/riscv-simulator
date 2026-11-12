#include "cli.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <istream>
#include <ostream>
#include <sstream>

namespace {

// Nombres ABI de los registros, en orden: ABI_NAMES[10] == "a0".
const char* const ABI_NAMES[32] = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

// Límite de instrucciones de "run", por si el programa nunca se detiene.
const uint64_t RUN_LIMIT = 10'000'000;

// Máximo de bytes que "mem" muestra de una vez.
const uint32_t MEM_LIMIT = 1024;

std::string hex32(uint32_t value) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%08X", static_cast<unsigned>(value));
    return buf;
}

// Hexadecimal sin ceros a la izquierda: 0x1000.
std::string hex_short(uint32_t value) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%X", static_cast<unsigned>(value));
    return buf;
}

std::string hex8(uint8_t value) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02X", static_cast<unsigned>(value));
    return buf;
}

// Convierte "123", "0x7B" o "0X7b" a un número de 32 bits.
// Devuelve false si el texto no es un número válido o no entra en 32 bits.
bool parse_u32(const std::string& text, uint32_t& value) {
    if (text.empty() || text[0] == '-' || text[0] == '+')
        return false;

    const bool is_hex = text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');

    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(text.c_str(), &end, is_hex ? 16 : 10);

    if (errno != 0 || end == text.c_str() || *end != '\0' || parsed > 0xFFFFFFFFull)
        return false;

    value = static_cast<uint32_t>(parsed);
    return true;
}

// Convierte "x5", "a0", "sp", "fp"... al número de registro (0-31).
bool parse_register(const std::string& name, uint32_t& index) {
    if (name.size() >= 2 && name[0] == 'x') {
        uint32_t n = 0;
        // Solo dígitos decimales después de la x (se rechaza "x0x1" o "x05").
        const std::string digits = name.substr(1);
        const bool plain = digits.find_first_not_of("0123456789") == std::string::npos &&
                           (digits.size() == 1 || digits[0] != '0');
        if (plain && parse_u32(digits, n) && n < 32) {
            index = n;
            return true;
        }
    }

    if (name == "fp") {  // Alias de s0.
        index = 8;
        return true;
    }

    for (uint32_t i = 0; i < 32; i++) {
        if (name == ABI_NAMES[i]) {
            index = i;
            return true;
        }
    }

    return false;
}

}  // namespace

Cli::Cli(Cpu& cpu, std::string program_path, std::istream& in, std::ostream& out)
    : cpu_(cpu), program_path_(std::move(program_path)), in_(in), out_(out) {}

void Cli::run() {
    std::string line;

    while (true) {
        out_ << "> " << std::flush;

        if (!std::getline(in_, line)) {
            out_ << "\n";  // Fin de la entrada (Ctrl+Z / Ctrl+D).
            break;
        }
        if (!execute(line))
            break;
    }

    out_ << "See you next time...\n";
}

bool Cli::execute(const std::string& line) {
    // Separar la línea en palabras.
    std::istringstream stream(line);
    std::vector<std::string> args;
    std::string word;
    while (stream >> word)
        args.push_back(word);

    if (args.empty())
        return true;  // Línea vacía: no hacer nada.

    const std::string command = args[0];
    args.erase(args.begin());

    if (command == "exit" || command == "quit") {
        return false;
    } else if (command == "pc") {
        cmd_pc();
    } else if (command == "step") {
        cmd_step(args);
    } else if (command == "run") {
        cmd_run();
    } else if (command == "regs") {
        cmd_regs(args);
    } else if (command == "mem") {
        cmd_mem(args);
    } else if (command == "reset") {
        cmd_reset();
    } else if (command == "help") {
        cmd_help();
    } else {
        out_ << "Comando desconocido: \"" << command << "\". Escribe \"help\" para ver los comandos.\n";
    }

    return true;
}

void Cli::cmd_pc() {
    out_ << "pc = " << hex32(cpu_.pc()) << "\n";
}

// step      -> ejecuta una instrucción
// step <n>  -> ejecuta n instrucciones
void Cli::cmd_step(const std::vector<std::string>& args) {
    uint32_t count = 1;

    if (args.size() > 1 || (args.size() == 1 && (!parse_u32(args[0], count) || count == 0))) {
        out_ << "Uso: step [cantidad]\n";
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        if (!cpu_.step()) {
            out_ << "Error: " << cpu_.error() << ". No se ejecutó.\n";
            return;
        }
        out_ << "Ejecutando instrucción.\n";
    }
}

// Ejecuta hasta que el programa salte a su propia dirección (bucle infinito con
// el que terminan los programas), ocurra un error o se llegue al límite.
void Cli::cmd_run() {
    uint64_t executed = 0;

    while (executed < RUN_LIMIT) {
        const uint32_t before = cpu_.pc();

        if (!cpu_.step()) {
            out_ << "Se ejecutaron " << executed << " instrucciones.\n";
            out_ << "Error: " << cpu_.error() << ". No se ejecutó.\n";
            return;
        }
        executed++;

        if (cpu_.pc() == before) {
            out_ << "Se ejecutaron " << executed << " instrucciones.\n";
            out_ << "Programa detenido: la instrucción en pc = " << hex32(before)
                 << " salta a sí misma.\n";
            return;
        }
    }

    out_ << "Se ejecutaron " << executed << " instrucciones.\n";
    out_ << "Ejecución pausada: se llegó al límite de instrucciones. Usa \"run\" para continuar.\n";
}

// regs            -> muestra los 32 registros
// regs x5 a0 sp   -> muestra solo los indicados
void Cli::cmd_regs(const std::vector<std::string>& args) {
    if (args.empty()) {
        // Los 32 registros en 4 columnas, con su nombre ABI.
        for (uint32_t row = 0; row < 8; row++) {
            for (uint32_t col = 0; col < 4; col++) {
                const uint32_t i = row + col * 8;
                char label[16];
                std::snprintf(label, sizeof(label), "x%-2u %-4s", static_cast<unsigned>(i), ABI_NAMES[i]);
                out_ << label << " = " << hex32(cpu_.reg(i)) << (col < 3 ? "    " : "\n");
            }
        }
        return;
    }

    // Validar todos los nombres antes de imprimir nada.
    std::vector<uint32_t> indexes;
    for (const std::string& name : args) {
        uint32_t index = 0;
        if (!parse_register(name, index)) {
            out_ << "Registro desconocido: \"" << name << "\". Usa x0-x31 o nombres como a0, sp, ra.\n";
            return;
        }
        indexes.push_back(index);
    }

    for (std::size_t i = 0; i < args.size(); i++)
        out_ << args[i] << " = " << hex32(cpu_.reg(indexes[i])) << "\n";
}

// mem <dir>           -> muestra un byte
// mem <inicio> <fin>  -> muestra los bytes del rango (ambos extremos incluidos)
void Cli::cmd_mem(const std::vector<std::string>& args) {
    uint32_t start = 0, end = 0;

    if (args.empty() || args.size() > 2 || !parse_u32(args[0], start) ||
        (args.size() == 2 && !parse_u32(args[1], end))) {
        out_ << "Uso: mem <inicio> [fin]   (direcciones en decimal o hexadecimal con 0x)\n";
        return;
    }
    if (args.size() == 1)
        end = start;

    if (end < start) {
        out_ << "Rango inválido: el fin es menor que el inicio.\n";
        return;
    }
    if (end - start >= MEM_LIMIT) {
        out_ << "Rango demasiado grande: el máximo es " << MEM_LIMIT << " bytes.\n";
        return;
    }

    const uint32_t size = end - start + 1;

    // Rangos cortos en una sola línea; los largos, 16 bytes por línea.
    if (size <= 16) {
        out_ << "Memoria (" << hex_short(start) << "-" << hex_short(end) << "):";
        for (uint32_t i = 0; i < size; i++)
            out_ << " " << hex8(cpu_.memory().read8(start + i));
        out_ << "\n";
        return;
    }

    out_ << "Memoria (" << hex_short(start) << "-" << hex_short(end) << "):\n";
    for (uint32_t i = 0; i < size; i++) {
        if (i % 16 == 0)
            out_ << hex32(start + i) << ":";
        out_ << " " << hex8(cpu_.memory().read8(start + i));
        if (i % 16 == 15 || i == size - 1)
            out_ << "\n";
    }
}

void Cli::cmd_reset() {
    try {
        cpu_.load_program(program_path_);
        out_ << "\"" << program_path_ << "\" cargado a memoria.\n";
    } catch (const std::exception& e) {
        out_ << "Error: " << e.what() << ".\n";
    }
}

void Cli::cmd_help() {
    out_ << "Comandos:\n"
            "  step [n]            Ejecuta una instrucción (o n instrucciones).\n"
            "  run                 Ejecuta hasta que el programa se detenga.\n"
            "  pc                  Muestra el PC.\n"
            "  regs [r1 r2 ...]    Muestra todos los registros, o solo los indicados (x5, a0, sp...).\n"
            "  mem <inicio> [fin]  Muestra los bytes de memoria del rango.\n"
            "  reset               Vuelve a cargar el programa y reinicia PC y registros.\n"
            "  help                Muestra esta ayuda.\n"
            "  exit                Sale del simulador.\n";
}
