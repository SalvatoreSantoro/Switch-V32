.section .text
.globl _start

_start:
    li t0, 5        # t0 = 5
    li t1, 5        # t1 = 5
    li t2, 3        # t2 = 3
    li t3, 7        # t3 = 7

    beq t0, t1, label_beq    # Branch if t0 == t1
    nop

label_beq:
    bne t0, t2, label_bne    # Branch if t0 != t2
    nop

label_bne:
    blt t2, t0, label_blt    # Branch if t2 < t0 (signed)
    nop

label_blt:
    bge t0, t2, label_bge    # Branch if t0 >= t2 (signed)
    nop

label_bge:
    bltu t2, t3, label_bltu  # Branch if t2 < t3 (unsigned)
    nop

label_bltu:
    bgeu t3, t2, label_bgeu  # Branch if t3 >= t2 (unsigned)
    nop

label_bgeu:
    # End of program, infinite loop
    j label_bgeu
