// Pruebas de la Fase 4: terminal de comandos.
// Se le pasa a la Cli una entrada de texto y se compara la salida completa.
#include <cstdio>
#include <sstream>
#include <string>

#include "cli.hpp"
#include "cpu.hpp"

static int failures = 0;

// Corre una sesión completa sobre riscvtest.bin y devuelve todo lo que imprimió.
static std::string session(const std::string& input) {
    Cpu cpu;
    const std::string path = std::string(PROGRAMS_DIR) + "/riscvtest.bin";
    cpu.load_program(path);

    std::istringstream in(input);
    std::ostringstream out;
    Cli cli(cpu, path, in, out);
    cli.run();
    return out.str();
}

static void expect(const std::string& input, const std::string& expected, int line) {
    const std::string actual = session(input);
    if (actual != expected) {
        std::printf("FALLO (linea %d)\n--- esperado ---\n%s--- obtenido ---\n%s----------------\n",
                    line, expected.c_str(), actual.c_str());
        failures++;
    }
}

#define EXPECT(input, expected) expect(input, expected, __LINE__)

int main() {
    // La misma interacción del ejemplo del enunciado (sección 4.1).
    EXPECT("pc\nstep\nstep\nstep\npc\nregs x2 x7\nstep\nmem 0x0 0x3\nexit\n",
           "> pc = 0x00000000\n"
           "> Ejecutando instrucción.\n"
           "> Ejecutando instrucción.\n"
           "> Ejecutando instrucción.\n"
           "> pc = 0x0000000C\n"
           "> x2 = 0x00000005\n"
           "x7 = 0x00000003\n"
           "> Ejecutando instrucción.\n"
           "> Memoria (0x0-0x3): 13 01 50 00\n"
           "> See you next time...\n");

    // step con cantidad, y nombres ABI de registros.
    EXPECT("step 3\nregs sp gp t2\nexit\n",
           "> Ejecutando instrucción.\n"
           "Ejecutando instrucción.\n"
           "Ejecutando instrucción.\n"
           "> sp = 0x00000005\n"
           "gp = 0x0000000C\n"
           "t2 = 0x00000003\n"
           "> See you next time...\n");

    // run hasta el final: el resultado esperado es 25 en la dirección 100.
    EXPECT("run\npc\nmem 100 103\nmem 100\nexit\n",
           "> Se ejecutaron 19 instrucciones.\n"
           "Programa detenido: la instrucción en pc = 0x00000050 salta a sí misma.\n"
           "> pc = 0x00000050\n"
           "> Memoria (0x64-0x67): 19 00 00 00\n"
           "> Memoria (0x64-0x64): 19\n"
           "> See you next time...\n");

    // Rango largo: 16 bytes por línea.
    EXPECT("mem 0x0 0x13\nexit\n",
           "> Memoria (0x0-0x13):\n"
           "0x00000000: 13 01 50 00 93 01 C0 00 93 83 71 FF 33 E2 23 00\n"
           "0x00000010: B3 F2 41 00\n"
           "> See you next time...\n");

    // reset devuelve el estado al inicio.
    {
        const std::string out = session("step 5\nreset\npc\nregs x2\nexit\n");
        if (out.find("cargado a memoria.\n> pc = 0x00000000\n> x2 = 0x00000000\n") == std::string::npos) {
            std::printf("FALLO: reset\n%s", out.c_str());
            failures++;
        }
    }

    // Entradas inválidas: se avisa y el simulador sigue funcionando.
    EXPECT("foo\nregs x32\nregs x05\nmem\nmem abc\nmem 8 4\nmem 0 0x400\nstep 0\nstep -1\nstep 1 2\n\n   \npc\nexit\n",
           "> Comando desconocido: \"foo\". Escribe \"help\" para ver los comandos.\n"
           "> Registro desconocido: \"x32\". Usa x0-x31 o nombres como a0, sp, ra.\n"
           "> Registro desconocido: \"x05\". Usa x0-x31 o nombres como a0, sp, ra.\n"
           "> Uso: mem <inicio> [fin]   (direcciones en decimal o hexadecimal con 0x)\n"
           "> Uso: mem <inicio> [fin]   (direcciones en decimal o hexadecimal con 0x)\n"
           "> Rango inválido: el fin es menor que el inicio.\n"
           "> Rango demasiado grande: el máximo es 1024 bytes.\n"
           "> Uso: step [cantidad]\n"
           "> Uso: step [cantidad]\n"
           "> Uso: step [cantidad]\n"
           "> > > pc = 0x00000000\n"
           "> See you next time...\n");

    // Instrucción inválida: step avisa y no avanza. (0x200 está fuera del programa.)
    {
        Cpu cpu;
        cpu.set_pc(0x2000);
        std::istringstream in("step\npc\nrun\n");
        std::ostringstream out;
        Cli cli(cpu, "", in, out);
        cli.run();
        const std::string expected =
            "> Error: instrucción inválida 0x00000000 en pc = 0x00002000. No se ejecutó.\n"
            "> pc = 0x00002000\n"
            "> Se ejecutaron 0 instrucciones.\n"
            "Error: instrucción inválida 0x00000000 en pc = 0x00002000. No se ejecutó.\n"
            "> \nSee you next time...\n";  // La entrada se acaba sin "exit".
        if (out.str() != expected) {
            std::printf("FALLO: instruccion invalida\n%s", out.str().c_str());
            failures++;
        }
    }

    if (failures == 0)
        std::printf("Todas las pruebas pasaron.\n");
    else
        std::printf("%d prueba(s) fallaron.\n", failures);

    return failures == 0 ? 0 : 1;
}
