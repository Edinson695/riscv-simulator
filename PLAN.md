# Plan de desarrollo — Simulador RISC-V (RV32I) en C++

Tarea 5 de CS3051 Arquitectura de Computadores 2026-II.
Este documento mapea todo lo que pide el enunciado y lo ordena en fases. Todavía no hay código.

## 0. Decisiones tomadas (11 nov 2026)

- **Alcance:** solo lo obligatorio (fases 0–5 y 7). Los extras de la Fase 6 quedan fuera.
- **Memoria:** páginas dispersas de 4 KiB.
- **Equipo:** una persona. **Entrega:** viernes 13 nov.
- **Informe por etapa:** al cerrar cada fase se escribe un informe corto (qué se hizo, decisiones,
  cómo se verificó, pendientes) en un documento compartido.

| Día | Fases |
|-----|-------|
| Miércoles 11 | 0 y 1 |
| Jueves 12 | 2, 3 y 4 |
| Viernes 13 | 5 y 7 |

## 1. Qué pide el enunciado

### Obligatorio

| # | Requisito | Fase |
|---|-----------|------|
| R1 | Soportar las 37 instrucciones del anexo A (RV32I sin `ecall`/`ebreak`/`fence`) | 2, 3 |
| R2 | Memoria little-endian, plana, direccionable por byte, espacio de 32 bits | 1 |
| R3 | Cargar un programa ya compilado desde un archivo binario crudo, en la dirección `0x00000000` | 1 |
| R4 | Ejecución paso por paso controlada por el usuario | 4 |
| R5 | Inspección de PC, registros y memoria en cualquier punto de la ejecución | 4 |

### Opcional (ideas adicionales) — fuera de alcance

| # | Extra | Costo | Valor para la demo | Fase |
|---|-------|-------|--------------------|------|
| E1 | Desensamblador sencillo | Bajo | Alto | 6 |
| E2 | `ecall` con syscalls estilo SPIM (imprimir, leer input, exit) | Bajo-medio | Alto | 6 |
| E3 | Carga directa de archivos `.elf` | Medio | Medio | 6 |
| E4 | Soporte para GDB (remote serial protocol) | Alto | Medio | 6 |

### Entregables (PDF en Canvas)

- Enlace al repositorio Git.
- Enlace a carpeta de Google Drive con el video demostrativo y el código fuente (ZIP o carpeta).
  El video debe explicar brevemente el simulador y mostrar **al menos 4 ejemplos de ejecución**.
- Informe breve (lenguaje, features, etc.).

### Lo que NO se pide

- Ensamblador (los programas llegan ya compilados desde CPUlator).
- Permisos ni segmentación de memoria.
- Interfaz gráfica (la terminal es suficiente).

## 2. Arquitectura propuesta

```
main  ──>  CLI (REPL)  ──>  CPU  ──>  Memory
               │             │
               │             ├──> Decoder  (palabra de 32 bits -> instrucción decodificada)
               │             └──> Executor (aplica la instrucción al estado)
               ├──> Loader        (.bin crudo; luego .elf)
               └──> Disassembler  (extra; reutiliza el Decoder)
```

| Módulo | Responsabilidad |
|--------|-----------------|
| `Memory` | Lectura/escritura de 8, 16 y 32 bits en little-endian |
| `CPU` | Estado: PC, `x0`–`x31`; ciclo fetch → decode → execute; estado de parada y errores |
| `Decoder` | Extraer opcode, rd, rs1, rs2, funct3, funct7 e inmediato con extensión de signo |
| `Loader` | Copiar el archivo a memoria desde `0x0` |
| `CLI` | Parsear comandos del usuario y mostrar el estado |
| `Disassembler` | Instrucción decodificada → texto (`addi x5, x0, 3`) |

Separar `Decoder` de la ejecución es la decisión clave: el desensamblador (E1) sale casi gratis
y cada parte se puede probar por separado.

## 3. Fases

### Fase 0 — Preparación ✅

- [ ] Crear el repositorio Git y subirlo (GitHub). *(pendiente)*
- [x] Definir sistema de build: CMake (funciona con Visual Studio, VS Code y g++).
- [x] Fijar estándar C++20 (garantiza el desplazamiento aritmético de enteros con signo).
- [x] Leer `src/main.cpp` del repo de referencia `Grazen0/toy-simulator`.
- [x] Generar en CPUlator los `.bin` crudos de los 3 programas de prueba (anexo B) y guardarlos
      en `programs/` junto con su `.s`.
- [x] Anotar, para cada programa, cómo termina (bucle infinito, salto a sí mismo, etc.) y si
      inicializa `sp`; esto condiciona la Fase 4.

**Listo cuando:** el proyecto vacío compila y los `.bin` están en `programs/`.

### Fase 1 — Estado arquitectural y carga ✅

- [x] `Memory` con **páginas dispersas**: mapa de número de página → bloque de 4 KiB, creado
      (en ceros) al primer acceso. Cubre los 4 GiB sin reservarlos, y el stack puede vivir en
      direcciones altas. Cuidar los accesos que cruzan el límite entre dos páginas.
- [x] Lecturas y escrituras de byte, halfword y word en little-endian.
- [x] Registros: 32 de 32 bits; `x0` siempre vale 0 (las escrituras se ignoran).
- [x] PC inicial en `0x00000000`.
- [x] `Loader`: leer el archivo en modo binario y copiarlo desde la dirección 0. Errores claros si
      el archivo no existe o está vacío.

**Listo cuando:** se carga un `.bin` y los primeros words leídos coinciden con la pestaña Memory de CPUlator.

### Fase 2 — Decodificación

- [ ] Extraer campos: opcode `[6:0]`, rd `[11:7]`, funct3 `[14:12]`, rs1 `[19:15]`, rs2 `[24:20]`, funct7 `[31:25]`.
- [ ] Inmediatos de los 5 formatos con extensión de signo:

| Formato | Bits del inmediato |
|---------|--------------------|
| I | `inst[31:20]` |
| S | `inst[31:25]`, `inst[11:7]` |
| B | `inst[31]`, `inst[7]`, `inst[30:25]`, `inst[11:8]`, `0` |
| U | `inst[31:12]` << 12 |
| J | `inst[31]`, `inst[19:12]`, `inst[20]`, `inst[30:21]`, `0` |

- [ ] Identificar la instrucción según la tabla del anexo de este documento.
- [ ] Codificación no reconocida → resultado "inválida" (no abortar aquí; lo decide la CPU).

**Listo cuando:** pruebas unitarias decodifican correctamente words conocidos de cada formato, incluyendo inmediatos negativos.

### Fase 3 — Ejecución de las 37 instrucciones

Orden sugerido (cada grupo se prueba antes de pasar al siguiente):

1. [ ] Aritmética/lógica inmediata: `addi slti sltiu xori ori andi slli srli srai`
2. [ ] Aritmética/lógica R: `add sub sll slt sltu xor srl sra or and`
3. [ ] Superiores: `lui auipc`
4. [ ] Cargas: `lb lh lw lbu lhu`
5. [ ] Almacenamiento: `sb sh sw`
6. [ ] Saltos condicionales: `beq bne blt bge bltu bgeu`
7. [ ] Saltos: `jal jalr`

Puntos donde suelen aparecer los errores:

- `x0` no se modifica nunca.
- Sumas y restas envuelven en 32 bits (operar sin signo).
- `slt`/`blt`/`bge` comparan con signo; `sltu`/`bltu`/`bgeu`, sin signo.
- `sltiu` extiende el signo del inmediato y luego compara sin signo.
- Desplazamientos usan solo los 5 bits bajos (shamt o `rs2`).
- `srli` y `srai` comparten funct3; se distinguen por funct7.
- `lb`/`lh` extienden signo; `lbu`/`lhu` extienden con ceros.
- Offsets de branch y `jal` son relativos al PC de la instrucción actual.
- `jalr`: destino = `(rs1 + imm)` con el bit 0 en cero; guardar `PC + 4` en `rd` aunque `rd == rs1`.
- El PC avanza 4 salvo que la instrucción lo cambie.
- Instrucción inválida: detener la CPU y reportar PC y word en hexadecimal.

**Listo cuando:** `riscvtest.s` deja 25 (`0x19`) en la dirección 100 (`0x64`).

### Fase 4 — Interfaz de terminal

Uso: `riscv-sim programa.bin`

| Comando | Efecto | ¿Obligatorio? |
|---------|--------|---------------|
| `step [n]` | Ejecuta 1 (o n) instrucciones | Sí (R4) |
| `pc` | Muestra el PC | Sí (R5) |
| `regs [x5 x14 ...]` | Muestra todos los registros o los indicados; aceptar también nombres ABI (`a0`, `sp`) | Sí (R5) |
| `mem <inicio> <fin>` | Vuelca bytes del rango | Sí (R5) |
| `exit` | Sale | Sí |
| `run` / `continue` | Ejecuta hasta detenerse | Recomendado |
| `break <dir>` | Punto de parada | Recomendado |
| `reset` | Recarga el programa y reinicia el estado | Recomendado |
| `help` | Lista de comandos | Recomendado |

- [ ] Parseo de números en hexadecimal (`0x...`) y decimal.
- [ ] Mensajes de error para comandos o argumentos inválidos (sin cerrar el simulador).
- [ ] Criterio de parada de `run`: breakpoint, instrucción inválida, salto a sí mismo
      (bucle infinito de fin de programa) y un límite máximo de pasos.

**Listo cuando:** se reproduce la sesión de ejemplo de la sección 4.1 del enunciado.

### Fase 5 — Validación

| Programa | Resultado esperado |
|----------|--------------------|
| `riscvtest.s` | 25 (`0x19`) en la dirección `0x64` |
| Quicksort | `{1, 2, 3, 4, 6, 8, 9}` en `0x1000` |
| Árbol simétrico | `a0 = 1` |
| Programa propio (4.º ejemplo del video) | A definir; idealmente usa cargas/almacenamientos de byte y halfword, shifts y comparaciones sin signo, que los otros tres apenas cubren |

- [ ] Comparar registros y memoria contra CPUlator en puntos intermedios, no solo al final.
- [ ] Pruebas unitarias por instrucción con casos límite (negativos, overflow, shamt 0 y 31).
- [ ] Probar un archivo con una instrucción inválida y verificar el reporte.

**Listo cuando:** los 4 programas dan el resultado esperado y coinciden con CPUlator.

### Fase 6 — Extras (fuera de alcance; solo si sobra tiempo)

1. [ ] **E1 Desensamblador:** comando `dis [dir] [n]` y mostrar la instrucción al hacer `step`.
2. [ ] **E2 `ecall`:** número de servicio en `a7`/`v0` según la doc de CPUlator; como mínimo
       print_int, print_string, read_int, print_char y exit.
3. [ ] **E3 ELF:** detectar la cabecera mágica, cargar los segmentos `PT_LOAD` y usar el entry point como PC.
4. [ ] **E4 GDB:** servidor del remote serial protocol sobre TCP (leer/escribir registros y memoria, step, continue, breakpoints). Solo si sobra tiempo.

Cada extra es independiente: se puede cortar aquí en cualquier punto sin afectar lo obligatorio.

### Fase 7 — Entregables

- [ ] `README.md` con cómo compilar, ejecutar y lista de comandos.
- [ ] Informe breve: lenguaje, arquitectura, decisiones (memoria, manejo de errores), features, pruebas.
- [ ] Video: explicación corta + 4 ejemplos de ejecución (los 3 programas sugeridos + el propio).
- [ ] Carpeta de Google Drive con el video y el código fuente.
- [ ] PDF final con los enlaces y el informe; subir a Canvas.

## 4. Dependencias entre fases

```
0 ─> 1 ─> 2 ─> 3 ─> 4 ─> 5 ─> 7
               │         └──> 6 (E2, E3, E4)
               └──> 6 (E1 puede empezar al terminar la Fase 2)
```


## 5. Decisiones pendientes

- Nombre y visibilidad del repositorio.
- Cuál será el cuarto programa de demostración.

## Anexo — Tabla de codificación

| Instrucción | Formato | opcode | funct3 | funct7 |
|-------------|---------|--------|--------|--------|
| `lb` | I | `0000011` | `000` | — |
| `lh` | I | `0000011` | `001` | — |
| `lw` | I | `0000011` | `010` | — |
| `lbu` | I | `0000011` | `100` | — |
| `lhu` | I | `0000011` | `101` | — |
| `addi` | I | `0010011` | `000` | — |
| `slli` | I | `0010011` | `001` | `0000000` |
| `slti` | I | `0010011` | `010` | — |
| `sltiu` | I | `0010011` | `011` | — |
| `xori` | I | `0010011` | `100` | — |
| `srli` | I | `0010011` | `101` | `0000000` |
| `srai` | I | `0010011` | `101` | `0100000` |
| `ori` | I | `0010011` | `110` | — |
| `andi` | I | `0010011` | `111` | — |
| `auipc` | U | `0010111` | — | — |
| `sb` | S | `0100011` | `000` | — |
| `sh` | S | `0100011` | `001` | — |
| `sw` | S | `0100011` | `010` | — |
| `add` | R | `0110011` | `000` | `0000000` |
| `sub` | R | `0110011` | `000` | `0100000` |
| `sll` | R | `0110011` | `001` | `0000000` |
| `slt` | R | `0110011` | `010` | `0000000` |
| `sltu` | R | `0110011` | `011` | `0000000` |
| `xor` | R | `0110011` | `100` | `0000000` |
| `srl` | R | `0110011` | `101` | `0000000` |
| `sra` | R | `0110011` | `101` | `0100000` |
| `or` | R | `0110011` | `110` | `0000000` |
| `and` | R | `0110011` | `111` | `0000000` |
| `lui` | U | `0110111` | — | — |
| `beq` | B | `1100011` | `000` | — |
| `bne` | B | `1100011` | `001` | — |
| `blt` | B | `1100011` | `100` | — |
| `bge` | B | `1100011` | `101` | — |
| `bltu` | B | `1100011` | `110` | — |
| `bgeu` | B | `1100011` | `111` | — |
| `jalr` | I | `1100111` | `000` | — |
| `jal` | J | `1101111` | — | — |

Extra E2: `ecall` = opcode `1110011`, resto en cero (`0x00000073`).
