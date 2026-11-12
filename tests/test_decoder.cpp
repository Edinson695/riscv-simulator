// Pruebas de la Fase 2: decodificación.
// Los words vienen del código máquina de los programas de prueba (riscvtest.s
// los documenta en sus comentarios) y de codificaciones armadas a mano.
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "decoder.hpp"

static int failures = 0;

// Comprueba que `raw` se decodifica como la instrucción y los campos indicados.
// Pasar -1 en un registro significa "no importa" (el formato no usa ese campo).
static void expect(uint32_t raw, Op op, int rd, int rs1, int rs2, int32_t imm, int line) {
    const Instruction i = decode(raw);
    const bool ok = i.op == op && i.raw == raw && (rd < 0 || i.rd == rd) &&
                    (rs1 < 0 || i.rs1 == rs1) && (rs2 < 0 || i.rs2 == rs2) && i.imm == imm;
    if (!ok) {
        std::printf("FALLO (linea %d): %08X -> %s rd=%d rs1=%d rs2=%d imm=%d\n", line,
                    static_cast<unsigned>(raw), mnemonic(i.op), i.rd, i.rs1, i.rs2,
                    static_cast<int>(i.imm));
        failures++;
    }
}

static void expect_invalid(uint32_t raw, int line) {
    if (decode(raw).op != Op::INVALID) {
        std::printf("FALLO (linea %d): %08X deberia ser invalida\n", line,
                    static_cast<unsigned>(raw));
        failures++;
    }
}

#define EXPECT(raw, op, rd, rs1, rs2, imm) expect(raw, op, rd, rs1, rs2, imm, __LINE__)
#define EXPECT_INVALID(raw) expect_invalid(raw, __LINE__)

int main() {
    // --- riscvtest.s ---
    EXPECT(0x00500113, Op::ADDI, 2, 0, -1, 5);     // addi x2, x0, 5
    EXPECT(0xFF718393, Op::ADDI, 7, 3, -1, -9);    // addi x7, x3, -9
    EXPECT(0x0023E233, Op::OR, 4, 7, 2, 0);        // or   x4, x7, x2
    EXPECT(0x0041F2B3, Op::AND, 5, 3, 4, 0);       // and  x5, x3, x4
    EXPECT(0x004282B3, Op::ADD, 5, 5, 4, 0);       // add  x5, x5, x4
    EXPECT(0x02728863, Op::BEQ, -1, 5, 7, 0x30);   // beq  x5, x7, end      (0x18 -> 0x48)
    EXPECT(0x0041A233, Op::SLT, 4, 3, 4, 0);       // slt  x4, x3, x4
    EXPECT(0x00020463, Op::BEQ, -1, 4, 0, 8);      // beq  x4, x0, around   (0x20 -> 0x28)
    EXPECT(0x402383B3, Op::SUB, 7, 7, 2, 0);       // sub  x7, x7, x2
    EXPECT(0x0471AA23, Op::SW, -1, 3, 7, 84);      // sw   x7, 84(x3)
    EXPECT(0x06002103, Op::LW, 2, 0, -1, 96);      // lw   x2, 96(x0)
    EXPECT(0x008001EF, Op::JAL, 3, -1, -1, 8);     // jal  x3, end          (0x40 -> 0x48)
    EXPECT(0x0221A023, Op::SW, -1, 3, 2, 0x20);    // sw   x2, 0x20(x3)
    EXPECT(0x00210063, Op::BEQ, -1, 2, 2, 0);      // beq  x2, x2, done     (salto a sí mismo)

    // --- quicksort y árbol simétrico ---
    EXPECT(0x00010137, Op::LUI, 2, -1, -1, 0x10000);   // li   sp, 0x10000
    EXPECT(0x00001537, Op::LUI, 10, -1, -1, 0x1000);   // li   a0, 0x1000
    EXPECT(0xFF010113, Op::ADDI, 2, 2, -1, -16);       // addi sp, sp, -16
    EXPECT(0x00112623, Op::SW, -1, 2, 1, 12);          // sw   ra, 12(sp)
    EXPECT(0x06C5D063, Op::BGE, -1, 11, 12, 0x60);     // bge  a1, a2, _qsort_end
    EXPECT(0x01C2CE63, Op::BLT, -1, 5, 28, 0x1C);      // blt  t0, t3, _if_end
    EXPECT(0x00051663, Op::BNE, -1, 10, 0, 12);        // bne  a0, zero, no_early_return
    EXPECT(0xFC9FF0EF, Op::JAL, 1, -1, -1, -56);       // call _qsort       (hacia atrás)
    EXPECT(0xFD1FF06F, Op::JAL, 0, -1, -1, -48);       // j    _for_begin   (hacia atrás)
    EXPECT(0x0000006F, Op::JAL, 0, -1, -1, 0);         // j    .
    EXPECT(0x00008067, Op::JALR, 0, 1, -1, 0);         // ret
    EXPECT(0x00261293, Op::SLLI, 5, 12, -1, 2);        // slli t0, a2, 2
    EXPECT(0xFFC2A283, Op::LW, 5, 5, -1, -4);          // lw   t0, -4(t0)
    EXPECT(0xFFF98693, Op::ADDI, 13, 19, -1, -1);      // addi a3, s3, -1

    // --- Instrucciones que los programas de prueba no usan ---
    EXPECT(0x00008503, Op::LB, 10, 1, -1, 0);          // lb    a0, 0(ra)
    EXPECT(0xFFE09503, Op::LH, 10, 1, -1, -2);         // lh    a0, -2(ra)
    EXPECT(0x7FF0C503, Op::LBU, 10, 1, -1, 2047);      // lbu   a0, 2047(ra)
    EXPECT(0x8000D503, Op::LHU, 10, 1, -1, -2048);     // lhu   a0, -2048(ra)
    EXPECT(0x8000A513, Op::SLTI, 10, 1, -1, -2048);    // slti  a0, ra, -2048
    EXPECT(0xFFF0B513, Op::SLTIU, 10, 1, -1, -1);      // sltiu a0, ra, -1
    EXPECT(0x0FF0C513, Op::XORI, 10, 1, -1, 255);      // xori  a0, ra, 255
    EXPECT(0x0FF0E513, Op::ORI, 10, 1, -1, 255);       // ori   a0, ra, 255
    EXPECT(0x0FF0F513, Op::ANDI, 10, 1, -1, 255);      // andi  a0, ra, 255
    EXPECT(0x01F0D513, Op::SRLI, 10, 1, -1, 31);       // srli  a0, ra, 31
    EXPECT(0x41F0D513, Op::SRAI, 10, 1, -1, 31);       // srai  a0, ra, 31
    EXPECT(0x00009513, Op::SLLI, 10, 1, -1, 0);        // slli  a0, ra, 0
    EXPECT(0xFFFFF517, Op::AUIPC, 10, -1, -1, -4096);  // auipc a0, 0xFFFFF
    EXPECT(0x80000537, Op::LUI, 10, -1, -1, INT32_MIN);// lui   a0, 0x80000
    EXPECT(0xFE208FA3, Op::SB, -1, 1, 2, -1);          // sb    sp, -1(ra)
    EXPECT(0x7E209FA3, Op::SH, -1, 1, 2, 2047);        // sh    sp, 2047(ra)
    EXPECT(0x00209533, Op::SLL, 10, 1, 2, 0);          // sll   a0, ra, sp
    EXPECT(0x0020B533, Op::SLTU, 10, 1, 2, 0);         // sltu  a0, ra, sp
    EXPECT(0x0020C533, Op::XOR, 10, 1, 2, 0);          // xor   a0, ra, sp
    EXPECT(0x0020D533, Op::SRL, 10, 1, 2, 0);          // srl   a0, ra, sp
    EXPECT(0x4020D533, Op::SRA, 10, 1, 2, 0);          // sra   a0, ra, sp
    EXPECT(0x7E20EFE3, Op::BLTU, -1, 1, 2, 4094);      // bltu  ra, sp, +4094 (máximo)
    EXPECT(0x8020F063, Op::BGEU, -1, 1, 2, -4096);     // bgeu  ra, sp, -4096 (mínimo)
    EXPECT(0x7FFFF0EF, Op::JAL, 1, -1, -1, 1048574);   // jal   ra, +1048574  (máximo)
    EXPECT(0x800000EF, Op::JAL, 1, -1, -1, -1048576);  // jal   ra, -1048576  (mínimo)
    EXPECT(0xFFC10567, Op::JALR, 10, 2, -1, -4);       // jalr  a0, -4(sp)

    // --- Codificaciones inválidas ---
    EXPECT_INVALID(0x00000000);  // Memoria en ceros.
    EXPECT_INVALID(0xFFFFFFFF);
    EXPECT_INVALID(0xAAAAAAAA);  // Relleno de CPUlator.
    EXPECT_INVALID(0x00000073);  // ecall (fuera de alcance).
    EXPECT_INVALID(0x0000B503);  // Carga con funct3 = 011.
    EXPECT_INVALID(0x0020B023);  // Almacenamiento con funct3 = 011.
    EXPECT_INVALID(0x0020A063);  // Branch con funct3 = 010.
    EXPECT_INVALID(0x00009567);  // jalr con funct3 distinto de 000.
    EXPECT_INVALID(0x02208533);  // funct7 = 0000001 (mul, extensión M).
    EXPECT_INVALID(0x4020C533);  // xor con funct7 = 0100000.
    EXPECT_INVALID(0x41F09513);  // slli con funct7 = 0100000.
    EXPECT_INVALID(0x81F0D513);  // srli/srai con funct7 desconocido.

    if (std::strcmp(mnemonic(Op::SLTIU), "sltiu") != 0 ||
        std::strcmp(mnemonic(Op::INVALID), "invalida") != 0) {
        std::printf("FALLO: mnemonic()\n");
        failures++;
    }

    if (failures == 0)
        std::printf("Todas las pruebas pasaron.\n");
    else
        std::printf("%d prueba(s) fallaron.\n", failures);

    return failures == 0 ? 0 : 1;
}
