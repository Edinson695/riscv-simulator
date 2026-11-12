# riscv-simulator

Simulador del ISA base RISC-V de 32 bits (RV32I) en C++.
CS3051 Arquitectura de Computadores 2026-II — Tarea 5.

Estado: **Fase 4 terminada** (simulador funcional con terminal de comandos). Ver [PLAN.md](PLAN.md).

## Compilar (Windows, MinGW)

```
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Sin CMake:

```
g++ -std=c++20 -Iinclude src/main.cpp src/cpu.cpp src/memory.cpp src/decoder.cpp src/cli.cpp -o riscv-sim.exe
```

## Uso

```
.\build\riscv-sim.exe programs\riscvtest.bin
```

El programa se carga en la dirección `0x00000000` y el simulador espera comandos:

| Comando | Efecto |
|---------|--------|
| `step [n]` | Ejecuta una instrucción (o `n` instrucciones) |
| `run` | Ejecuta hasta que el programa se detenga (salto a sí mismo o error) |
| `pc` | Muestra el PC |
| `regs [r1 r2 ...]` | Muestra todos los registros, o solo los indicados (`x5`, `a0`, `sp`...) |
| `mem <inicio> [fin]` | Muestra los bytes de memoria del rango (decimal o `0x` hexadecimal) |
| `reset` | Vuelve a cargar el programa y reinicia PC y registros |
| `help` | Lista de comandos |
| `exit` | Sale |

Ejemplo:

```
"programs\riscvtest.bin" cargado a memoria.
> pc
pc = 0x00000000
> step
Ejecutando instrucción.
> step
Ejecutando instrucción.
> regs x2 x3
x2 = 0x00000005
x3 = 0x0000000C
> run
Se ejecutaron 17 instrucciones.
Programa detenido: la instrucción en pc = 0x00000050 salta a sí misma.
> mem 100 103
Memoria (0x64-0x67): 19 00 00 00
> exit
See you next time...
```

## Pruebas

```
.\build\test_state.exe
.\build\test_decoder.exe
.\build\test_execute.exe
.\build\test_cli.exe
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
