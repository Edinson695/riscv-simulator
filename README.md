# riscv-simulator

Simulador del ISA base RISC-V de 32 bits (RV32I) en C++.
CS3051 Arquitectura de Computadores 2026-II — Tarea 5.

Estado: **Fase 2 terminada** (memoria, registros, carga de programas y decodificación). Ver [PLAN.md](PLAN.md).

## Compilar y ejecutar (Windows, MinGW)

```
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
.\build\riscv-sim.exe programs\riscvtest.bin
```

Pruebas:

```
.\build\test_state.exe
.\build\test_decoder.exe
```

Sin CMake:

```
g++ -std=c++20 -Wall -Wextra -Iinclude src/*.cpp -o riscv-sim.exe
```

## Programas de prueba

| Binario | Fuente | Resultado esperado |
|---------|--------|--------------------|
| `programs/riscvtest.bin` | `riscvtest.s.asm` | 25 (`0x19`) en la dirección `0x64` |
| `programs/quicksort.bin` | `Quicksort (RISC-V).asm` | `{1, 2, 3, 4, 6, 8, 9}` en `0x1000` |
| `programs/arbol_simetrico.bin` | `Árbol simétrico (RISC-V).asm` | `a0 = 1` |

## Estructura

| Carpeta     | Contenido                                                        |
|-------------|------------------------------------------------------------------|
| `src/`      | Implementación (.cpp)                                            |
| `include/`  | Cabeceras (.hpp)                                                 |
| `programs/` | Programas de prueba: fuente `.s` y binario crudo `.bin`          |
| `tests/`    | Pruebas unitarias y de integración                               |
| `docs/`     | Enunciado, informe y material del video                          |
