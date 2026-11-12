// Pruebas de la Fase 3: ejecución de las 37 instrucciones y de los tres
// programas de prueba completos.
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>

#include "cpu.hpp"

static int failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::printf("FALLO (linea %d): %s\n", __LINE__, #cond);        \
            failures++;                                                    \
        }                                                                  \
    } while (0)

// --- Codificadores, para escribir las pruebas con nombres en vez de hexadecimal ---

static uint32_t enc_r(uint32_t f7, uint32_t rs2, uint32_t rs1, uint32_t f3, uint32_t rd) {
    return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | 0b0110011;
}
static uint32_t enc_i(uint32_t opcode, int32_t imm, uint32_t rs1, uint32_t f3, uint32_t rd) {
    return ((static_cast<uint32_t>(imm) & 0xFFF) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | opcode;
}
static uint32_t enc_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t f3) {
    const uint32_t u = static_cast<uint32_t>(imm);
    return (((u >> 5) & 0x7F) << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | ((u & 0x1F) << 7) | 0b0100011;
}
static uint32_t enc_b(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t f3) {
    const uint32_t u = static_cast<uint32_t>(imm);
    return (((u >> 12) & 1) << 31) | (((u >> 5) & 0x3F) << 25) | (rs2 << 20) | (rs1 << 15) |
           (f3 << 12) | (((u >> 1) & 0xF) << 8) | (((u >> 11) & 1) << 7) | 0b1100011;
}
static uint32_t enc_u(uint32_t opcode, uint32_t imm20, uint32_t rd) {
    return (imm20 << 12) | (rd << 7) | opcode;
}
static uint32_t enc_j(int32_t imm, uint32_t rd) {
    const uint32_t u = static_cast<uint32_t>(imm);
    return (((u >> 20) & 1) << 31) | (((u >> 1) & 0x3FF) << 21) | (((u >> 11) & 1) << 20) |
           (((u >> 12) & 0xFF) << 12) | (rd << 7) | 0b1101111;
}

static const uint32_t OP_LOAD = 0b0000011, OP_IMM = 0b0010011, OP_JALR = 0b1100111;
static const uint32_t OP_LUI = 0b0110111, OP_AUIPC = 0b0010111;

// Ejecuta una sola instrucción en `pc` con x1 = a y x2 = b, y devuelve la CPU.
static Cpu run1(uint32_t word, uint32_t a = 0, uint32_t b = 0, uint32_t pc = 0x100) {
    Cpu cpu;
    cpu.set_pc(pc);
    cpu.set_reg(1, a);
    cpu.set_reg(2, b);
    cpu.memory().write32(pc, word);
    if (!cpu.step()) {
        std::printf("FALLO: step() devolvio error: %s\n", cpu.error().c_str());
        failures++;
    }
    return cpu;
}

// Resultado en x3 de una operación R (x3 = x1 op x2).
static uint32_t r(uint32_t f7, uint32_t f3, uint32_t a, uint32_t b) {
    return run1(enc_r(f7, 2, 1, f3, 3), a, b).reg(3);
}
// Resultado en x3 de una operación con inmediato (x3 = x1 op imm).
static uint32_t i(uint32_t f3, uint32_t a, int32_t imm) {
    return run1(enc_i(OP_IMM, imm, 1, f3, 3), a).reg(3);
}
// PC resultante de un branch (x1 ? x2) con desplazamiento `off` desde 0x100.
static uint32_t br(uint32_t f3, uint32_t a, uint32_t b, int32_t off = 0x40) {
    return run1(enc_b(off, 2, 1, f3), a, b).pc();
}

static void test_register_ops() {
    CHECK(r(0x00, 0, 7, 5) == 12);                           // add
    CHECK(r(0x00, 0, 0xFFFFFFFF, 1) == 0);                   // add: da la vuelta
    CHECK(r(0x00, 0, 0x7FFFFFFF, 1) == 0x80000000);          // add: overflow con signo
    CHECK(r(0x20, 0, 5, 7) == 0xFFFFFFFE);                   // sub: 5 - 7 = -2
    CHECK(r(0x20, 0, 0x80000000, 1) == 0x7FFFFFFF);          // sub
    CHECK(r(0x00, 1, 1, 31) == 0x80000000);                  // sll
    CHECK(r(0x00, 1, 3, 33) == 6);                           // sll: solo 5 bits de rs2
    CHECK(r(0x00, 2, 0xFFFFFFFF, 1) == 1);                   // slt: -1 < 1
    CHECK(r(0x00, 2, 1, 0xFFFFFFFF) == 0);                   // slt
    CHECK(r(0x00, 2, 5, 5) == 0);                            // slt: iguales
    CHECK(r(0x00, 3, 0xFFFFFFFF, 1) == 0);                   // sltu: 4294967295 < 1 es falso
    CHECK(r(0x00, 3, 1, 0xFFFFFFFF) == 1);                   // sltu
    CHECK(r(0x00, 4, 0b1100, 0b1010) == 0b0110);             // xor
    CHECK(r(0x00, 5, 0x80000000, 31) == 1);                  // srl: entra un 0
    CHECK(r(0x00, 5, 0x80000000, 63) == 1);                  // srl: solo 5 bits de rs2
    CHECK(r(0x20, 5, 0x80000000, 31) == 0xFFFFFFFF);         // sra: copia el signo
    CHECK(r(0x20, 5, 0x40000000, 30) == 1);                  // sra de un positivo
    CHECK(r(0x20, 5, 0xFFFFFFF0, 36) == 0xFFFFFFFF);         // sra: solo 5 bits de rs2
    CHECK(r(0x00, 6, 0b1100, 0b1010) == 0b1110);             // or
    CHECK(r(0x00, 7, 0b1100, 0b1010) == 0b1000);             // and
}

static void test_immediate_ops() {
    CHECK(i(0, 10, -3) == 7);                                // addi
    CHECK(i(0, 0, -1) == 0xFFFFFFFF);                        // addi
    CHECK(i(0, 0x7FFFFFFF, 1) == 0x80000000);                // addi: da la vuelta
    CHECK(i(2, 0xFFFFFFFE, -1) == 1);                        // slti: -2 < -1
    CHECK(i(2, 0, -1) == 0);                                 // slti: 0 < -1 es falso
    CHECK(i(3, 0, -1) == 1);                                 // sltiu: 0 < 0xFFFFFFFF
    CHECK(i(3, 0xFFFFFFFF, -1) == 0);                        // sltiu
    CHECK(i(3, 0, 1) == 1);                                  // sltiu (seqz)
    CHECK(i(4, 0x0F0F0F0F, -1) == 0xF0F0F0F0);               // xori con -1 = not
    CHECK(i(6, 0x0F, 0xF0) == 0xFF);                         // ori
    CHECK(i(6, 0, -2048) == 0xFFFFF800);                     // ori: inmediato con signo
    CHECK(i(7, 0x12345678, 0xFF) == 0x78);                   // andi
    CHECK(i(7, 0x12345678, -16) == 0x12345670);              // andi: inmediato con signo
    CHECK(i(1, 1, 31) == 0x80000000);                        // slli
    CHECK(i(1, 0xFFFFFFFF, 0) == 0xFFFFFFFF);                // slli 0
    CHECK(i(5, 0x80000000, 31) == 1);                        // srli
    CHECK(i(5, 0x80000000, 0x400 | 31) == 0xFFFFFFFF);       // srai (funct7 = 0100000)
    CHECK(i(5, 0x80000000, 0x400 | 4) == 0xF8000000);        // srai
    CHECK(i(5, 0x7FFFFFFF, 0x400 | 4) == 0x07FFFFFF);        // srai de un positivo
}

static void test_upper() {
    CHECK(run1(enc_u(OP_LUI, 0x12345, 3)).reg(3) == 0x12345000);         // lui
    CHECK(run1(enc_u(OP_LUI, 0xFFFFF, 3)).reg(3) == 0xFFFFF000);
    CHECK(run1(enc_u(OP_AUIPC, 0x1, 3)).reg(3) == 0x1100);               // auipc: pc + 0x1000
    CHECK(run1(enc_u(OP_AUIPC, 0xFFFFF, 3)).reg(3) == 0xFFFFF100);       // auipc negativo
    CHECK(run1(enc_u(OP_AUIPC, 0, 3)).reg(3) == 0x100);                  // auipc 0 = pc
}

static void test_loads() {
    // x1 apunta a 0x2000. En memoria: 80 FF 7F 00 | F0 DE BC 9A
    auto load = [](uint32_t f3, int32_t imm) {
        Cpu cpu;
        cpu.memory().write32(0x2000, 0x007FFF80);
        cpu.memory().write32(0x2004, 0x9ABCDEF0);
        cpu.set_pc(0x100);
        cpu.set_reg(1, 0x2000);
        cpu.memory().write32(0x100, enc_i(OP_LOAD, imm, 1, f3, 3));
        CHECK(cpu.step());
        CHECK(cpu.pc() == 0x104);
        return cpu.reg(3);
    };

    CHECK(load(0, 0) == 0xFFFFFF80);   // lb: 0x80 con signo
    CHECK(load(0, 2) == 0x0000007F);   // lb: 0x7F
    CHECK(load(4, 0) == 0x00000080);   // lbu
    CHECK(load(4, 1) == 0x000000FF);   // lbu
    CHECK(load(1, 0) == 0xFFFFFF80);   // lh: 0xFF80 con signo
    CHECK(load(1, 2) == 0x0000007F);   // lh: 0x007F
    CHECK(load(5, 0) == 0x0000FF80);   // lhu
    CHECK(load(5, 6) == 0x00009ABC);   // lhu
    CHECK(load(2, 0) == 0x007FFF80);   // lw
    CHECK(load(2, 4) == 0x9ABCDEF0);   // lw

    // Desplazamiento negativo: x1 = 0x2008, lw x3, -4(x1).
    Cpu cpu;
    cpu.memory().write32(0x2004, 0x9ABCDEF0);
    cpu.set_reg(1, 0x2008);
    cpu.memory().write32(0, enc_i(OP_LOAD, -4, 1, 2, 3));
    CHECK(cpu.step());
    CHECK(cpu.reg(3) == 0x9ABCDEF0);
}

static void test_stores() {
    // x1 = 0x2000 (base), x2 = 0x11223344 (dato). La memoria empieza en 0xFFFFFFFF.
    auto store = [](uint32_t f3, int32_t imm) {
        Cpu cpu;
        cpu.memory().write32(0x2000, 0xFFFFFFFF);
        cpu.memory().write32(0x2004, 0xFFFFFFFF);
        cpu.set_reg(1, 0x2004);
        cpu.set_reg(2, 0x11223344);
        cpu.memory().write32(0, enc_s(imm, 2, 1, f3));
        CHECK(cpu.step());
        CHECK(cpu.pc() == 4);
        return cpu;
    };

    CHECK(store(0, 0).memory().read32(0x2004) == 0xFFFFFF44);    // sb: solo 1 byte
    CHECK(store(0, 3).memory().read32(0x2004) == 0x44FFFFFF);    // sb
    CHECK(store(1, 0).memory().read32(0x2004) == 0xFFFF3344);    // sh: solo 2 bytes
    CHECK(store(1, 2).memory().read32(0x2004) == 0x3344FFFF);    // sh
    CHECK(store(2, 0).memory().read32(0x2004) == 0x11223344);    // sw
    CHECK(store(2, -4).memory().read32(0x2000) == 0x11223344);   // sw con desplazamiento negativo
    CHECK(store(2, -4).memory().read32(0x2004) == 0xFFFFFFFF);   // ... y no toca el word vecino
    CHECK(store(0, -1).memory().read32(0x2000) == 0x44FFFFFF);   // sb -1
}

static void test_branches() {
    const uint32_t TAKEN = 0x140, NOT_TAKEN = 0x104;
    const uint32_t NEG = 0xFFFFFFFF;  // -1 con signo, 4294967295 sin signo

    CHECK(br(0, 5, 5) == TAKEN);          // beq
    CHECK(br(0, 5, 6) == NOT_TAKEN);
    CHECK(br(1, 5, 6) == TAKEN);          // bne
    CHECK(br(1, 5, 5) == NOT_TAKEN);
    CHECK(br(4, NEG, 1) == TAKEN);        // blt: -1 < 1
    CHECK(br(4, 1, NEG) == NOT_TAKEN);
    CHECK(br(4, 5, 5) == NOT_TAKEN);
    CHECK(br(5, 1, NEG) == TAKEN);        // bge: 1 >= -1
    CHECK(br(5, 5, 5) == TAKEN);
    CHECK(br(5, NEG, 1) == NOT_TAKEN);
    CHECK(br(6, 1, NEG) == TAKEN);        // bltu: 1 < 4294967295
    CHECK(br(6, NEG, 1) == NOT_TAKEN);
    CHECK(br(6, 5, 5) == NOT_TAKEN);
    CHECK(br(7, NEG, 1) == TAKEN);        // bgeu
    CHECK(br(7, 5, 5) == TAKEN);
    CHECK(br(7, 1, NEG) == NOT_TAKEN);

    CHECK(br(0, 5, 5, -0x20) == 0xE0);    // hacia atrás
    CHECK(br(0, 5, 5, 0) == 0x100);       // salto a sí mismo
}

static void test_jumps() {
    Cpu c = run1(enc_j(0x40, 5));                      // jal x5, +0x40
    CHECK(c.pc() == 0x140);
    CHECK(c.reg(5) == 0x104);

    c = run1(enc_j(-0x80, 1));                         // jal ra, -0x80
    CHECK(c.pc() == 0x80);
    CHECK(c.reg(1) == 0x104);

    c = run1(enc_j(0x40, 0));                          // j +0x40 (rd = x0)
    CHECK(c.pc() == 0x140);
    CHECK(c.reg(0) == 0);

    c = run1(enc_i(OP_JALR, 8, 1, 0, 5), 0x2000);      // jalr x5, 8(x1)
    CHECK(c.pc() == 0x2008);
    CHECK(c.reg(5) == 0x104);

    c = run1(enc_i(OP_JALR, -4, 1, 0, 0), 0x2000);     // jalr x0, -4(x1)
    CHECK(c.pc() == 0x1FFC);

    c = run1(enc_i(OP_JALR, 1, 1, 0, 5), 0x2000);      // el bit 0 del destino se pone en 0
    CHECK(c.pc() == 0x2000);

    c = run1(enc_i(OP_JALR, 4, 1, 0, 1), 0x2000);      // jalr x1, 4(x1): rd == rs1
    CHECK(c.pc() == 0x2004);
    CHECK(c.reg(1) == 0x104);
}

static void test_x0_and_errors() {
    // Escribir en x0 no tiene efecto.
    Cpu c = run1(enc_i(OP_IMM, 123, 0, 0, 0));         // addi x0, x0, 123
    CHECK(c.reg(0) == 0);
    CHECK(c.pc() == 0x104);
    CHECK(c.steps() == 1);

    // Instrucción inválida: no se ejecuta y el estado no cambia.
    Cpu cpu;
    cpu.set_pc(0x100);
    cpu.set_reg(5, 77);
    cpu.memory().write32(0x100, 0xAAAAAAAA);
    CHECK(!cpu.step());
    CHECK(!cpu.error().empty());
    CHECK(cpu.pc() == 0x100);
    CHECK(cpu.reg(5) == 77);
    CHECK(cpu.steps() == 0);

    // Memoria vacía (0x00000000) también es inválida.
    Cpu empty;
    CHECK(!empty.step());

    // PC que no es múltiplo de 4.
    Cpu mis;
    mis.memory().write32(0, enc_i(OP_JALR, 2, 0, 0, 0));   // jalr x0, 2(x0) -> pc = 2
    CHECK(mis.step());
    CHECK(mis.pc() == 2);
    CHECK(!mis.step());
    CHECK(!mis.error().empty());

    // Después de un error, un step correcto limpia el mensaje.
    cpu.memory().write32(0x100, enc_i(OP_IMM, 1, 0, 0, 5));
    CHECK(cpu.step());
    CHECK(cpu.error().empty());
    CHECK(cpu.reg(5) == 1);
}

// Ejecuta un programa hasta que salta a su propia dirección (así terminan los
// programas de prueba). Devuelve false si hubo error o no terminó.
static bool run_program(Cpu& cpu, const std::string& name) {
    const std::string path = std::string(PROGRAMS_DIR) + "/" + name;
    try {
        cpu.load_program(path);
    } catch (const std::exception& e) {
        std::printf("FALLO: %s\n", e.what());
        return false;
    }

    for (int n = 0; n < 1000000; n++) {
        const uint32_t before = cpu.pc();
        if (!cpu.step()) {
            std::printf("FALLO en %s: %s\n", name.c_str(), cpu.error().c_str());
            return false;
        }
        if (cpu.pc() == before)
            return true;
    }

    std::printf("FALLO: %s no termino\n", name.c_str());
    return false;
}

static void test_programs() {
    {
        Cpu cpu;
        CHECK(run_program(cpu, "riscvtest.bin"));
        CHECK(cpu.memory().read32(100) == 25);     // Resultado esperado: [100] = 25
        CHECK(cpu.memory().read32(96) == 7);
        CHECK(cpu.pc() == 0x50);
    }
    {
        Cpu cpu;
        CHECK(run_program(cpu, "quicksort.bin"));
        const uint32_t expected[7] = {1, 2, 3, 4, 6, 8, 9};
        for (uint32_t k = 0; k < 7; k++)
            CHECK(cpu.memory().read32(0x1000 + 4 * k) == expected[k]);
        CHECK(cpu.reg(2) == 0x10000);              // El stack quedó balanceado.
    }
    {
        Cpu cpu;
        CHECK(run_program(cpu, "arbol_simetrico.bin"));
        CHECK(cpu.reg(10) == 1);                   // a0 = 1 (es simétrico)
        CHECK(cpu.reg(2) == 0x10000);
    }
}

int main() {
    test_register_ops();
    test_immediate_ops();
    test_upper();
    test_loads();
    test_stores();
    test_branches();
    test_jumps();
    test_x0_and_errors();
    test_programs();

    if (failures == 0)
        std::printf("Todas las pruebas pasaron.\n");
    else
        std::printf("%d prueba(s) fallaron.\n", failures);

    return failures == 0 ? 0 : 1;
}
