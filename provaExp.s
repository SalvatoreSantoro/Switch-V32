    .section .data
array:      .word 1, 2, 3, 4, 5   # Initialize an array
value:      .word 10              # A value for computation
result:     .word 0               # Store result here

    .section .text
    .globl _start

_start:
    # Load address of array
    la t0, array         # t0 = base address of array

    # Load first element into t1
    lw t1, 0(t0)         # t1 = array[0] = 1

    # Load second element
    lw t2, 4(t0)         # t2 = array[1] = 2

    # ADD: t3 = t1 + t2
    add t3, t1, t2       # t3 = 1 + 2 = 3

    # SUB: t4 = t2 - t1
    sub t4, t2, t1       # t4 = 2 - 1 = 1

    # SLL (Shift Left Logical): t5 = t1 << 1
    sll t5, t1, t1       # t5 = 1 << 1 = 2

    # SLT (Set Less Than): t6 = (t1 < t2) ? 1 : 0
    slt t6, t1, t2       # t6 = 1

    # SLTU (Set Less Than Unsigned): t6 = (unsigned t1 < unsigned t2)
    sltu t6, t2, t1      # t6 = 0 because 2 > 1 (unsigned compare)

    # SRL (Shift Right Logical): t5 = t5 >> 1
    srl t5, t5, t1       # t5 = 2 >> 1 = 1

    # SRA (Shift Right Arithmetic): t5 = t5 >> 1 (signed)
    sra t5, t5, t1       # t5 = 1 >> 1 = 0

    # OR: t7 = t1 | t2

    # AND: t7 = t1 & t2

    # Immediate instructions
    # ADDI: add immediate
    addi s0, t1, 5       # s0 = 1 + 5 = 6

    # SLTI: set less than immediate
    slti s1, t1, 5       # s1 = (1 < 5) ? 1 : 0

    # SLTIU: set less than immediate unsigned
    sltiu s2, t2, 1      # s2 = (2 < 1 unsigned) ? 1 : 0 (0)

    # XORI: xor immediate
    xori s3, t2, 3       # s3 = 2 ^ 3 = 1

    # ORI: or immediate
    ori s4, t2, 1        # s4 = 2 | 1 = 3

    # ANDI: and immediate
    andi s5, t2, 3       # s5 = 2 & 3 = 2

    # SLLI: shift left logical immediate
    slli s6, t1, 2       # s6 = 1 << 2 = 4

    # SRLI: shift right logical immediate
    srli s7, t2, 1       # s7 = 2 >> 1 = 1

    # SRAI: shift right arithmetic immediate
    srai t3, t3, 1       # t3 = 3 >> 1 = 1

    # LUI: load upper immediate
    lui a0, 0x12345      # a0 = 0x12345000

    # AUIPC: add upper immediate to PC
    auipc a1, 0x10       # a1 = PC + 0x10000

    # JAL: jump and link
    jal ra, jump_target  # jump to jump_target, store return addr in ra

after_jump:
    # Load value
    la t0, value
    lw t1, 0(t0)

    # Store value (SW)
    la t0, result
    sw t1, 0(t0)

    # Branch Equal (BEQ)
    beq t1, t1, branch_equal   # Always taken

branch_not_equal:
    # Branch Not Equal (BNE)
    bne t1, t2, end            # if t1 != t2 (should not jump)

branch_equal:
    # BLT: branch less than
    blt t1, t2, after_blt      # if t1 < t2 (jump)

    # BGE: branch greater or equal
    bge t2, t1, after_blt      # if t2 >= t1 (jump)

after_blt:
    # BLTU: branch less than unsigned
    bltu t1, t2, after_bltu    # if unsigned t1 < t2 (jump)

    # BGEU: branch greater equal unsigned
    bgeu t2, t1, after_bltu    # if unsigned t2 >= t1 (jump)

after_bltu:
    # JALR: jump and link register
    jalr zero, 0(ra)           # jump back using ra

end:
    nop                        # end

jump_target:
    jal ra, after_jump         # Jump back after target
