# demo_bytes.s - Cuarto programa de demostración.
#
# Ejercita lo que los otros tres programas casi no usan: accesos de byte y
# halfword (sb, sh, lb, lbu, lh, lhu), desplazamientos y comparaciones con y
# sin signo.
#
# Resultados esperados al terminar:
#   s1 = 0xFFFFFF80   lb  del byte 0x80 (extiende el signo)
#   s2 = 0x00000080   lbu del mismo byte (rellena con ceros)
#   s3 = 0xFFFFBEEF   lh  del halfword 0xBEEF
#   s4 = 0x0000BEEF   lhu del mismo halfword
#   a0 = 0x000001FF   suma de los 4 bytes: 0x80 + 0x7F + 0xFF + 0x01
#   a1 = 1            slt : -128 < 128         (con signo)
#   a2 = 0            sltu: 0xFFFFFF80 < 0x80  (sin signo) es falso
#   a3 = 0xFFFFFFF8   srai s1, 4  (copia el bit de signo)
#   a4 = 0x0000000F   srli s1, 28 (entran ceros)
#   a5 = 0x00001FF0   slli a0, 4
#   a6 = 0xFFFF0000   xor  s3, s4
#   Memoria 0x2000: 80 7F FF 01 | EF BE 00 00 | FF 01 00 00 | 00 00 FF FF

.global _start
_start:
    lui   s0, 0x2            # s0 = 0x2000, dirección del buffer

    # Escribir 4 bytes sueltos: 80 7F FF 01
    addi  t0, zero, 0x80
    sb    t0, 0(s0)
    addi  t0, zero, 0x7F
    sb    t0, 1(s0)
    addi  t0, zero, -1       # solo se guarda el byte bajo: 0xFF
    sb    t0, 2(s0)
    addi  t0, zero, 1
    sb    t0, 3(s0)

    # El mismo byte leído con y sin signo
    lb    s1, 0(s0)
    lbu   s2, 0(s0)

    # Un halfword, leído con y sin signo
    lui   t0, 0xC            # t0 = 0xC000
    addi  t0, t0, -273       # t0 = 0xBEEF
    sh    t0, 4(s0)
    lh    s3, 4(s0)
    lhu   s4, 4(s0)

    # Sumar los 4 bytes con un bucle
    addi  a0, zero, 0        # suma
    addi  t1, zero, 0        # i
    addi  t2, zero, 4        # n
loop:
    add   t3, s0, t1
    lbu   t4, 0(t3)
    add   a0, a0, t4
    addi  t1, t1, 1
    bltu  t1, t2, loop

    # Comparaciones y desplazamientos
    slt   a1, s1, s2
    sltu  a2, s1, s2
    srai  a3, s1, 4
    srli  a4, s1, 28
    slli  a5, a0, 4
    xor   a6, s3, s4

    # Guardar resultados
    sw    a0, 8(s0)
    sw    a6, 12(s0)

end:
    j     end                # fin: salto a sí mismo
