.section .text
.globl _start
_start:
    lui     x1, 0x12345       # Load upper immediate
    auipc   x2, 0x10          # Add upper immediate to PC
    jal     x3, label1        # Jump and link
label1:
    jalr    x4, 0(x3)         # Jump and link register (return to jal)
    beq     x1, x1, label2    # Always taken
    nop                      # Padding
label2:
    bne     x1, x2, label3    # Different values
    nop
label3:
    blt     x2, x1, label4    # x2 < x1 (false here, depends)
    nop
label4:
    bge     x1, x2, label5    # x1 >= x2 (true)
    nop
label5:
    bltu    x2, x1, label6    # unsigned less than
    nop
label6:
    bgeu    x1, x2, label7    # unsigned greater equal
    nop
label7:
    # Loads: LB, LH, LW, LBU, LHU
    la      x5, data
    lb      x6, 0(x5)         # Load byte
    lh      x7, 0(x5)         # Load halfword
    lw      x8, 0(x5)         # Load word
    lbu     x9, 0(x5)         # Load byte unsigned
    lhu     x10, 0(x5)        # Load halfword unsigned

    # Stores: SB, SH, SW
    sb      x6, 1(x5)         # Store byte
    sh      x7, 2(x5)         # Store halfword
    sw      x8, 4(x5)         # Store word

    # Immediate ops: ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
    addi    x11, x0, 42       # x11 = 42
    slti    x12, x11, 50      # x12 = (x11 < 50) ? 1 : 0
    sltiu   x13, x11, 50      # unsigned comparison
    xori    x14, x11, 0xFF    # xor immediate
    ori     x15, x11, 0x01    # or immediate
    andi    x16, x11, 0x0F    # and immediate
    slli    x17, x11, 2       # shift left logical immediate
    srli    x18, x11, 2       # shift right logical immediate
    srai    x19, x11, 2       # shift right arithmetic immediate

    # Register ops: ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
    add     x20, x11, x11     # x20 = x11 + x11
    sub     x21, x20, x11     # x21 = x20 - x11
    sll     x22, x11, x11     # shift left logical
    slt     x23, x11, x12     # signed less than
    sltu    x24, x11, x12     # unsigned less than
    xor     x25, x11, x12     # xor
    srl     x26, x11, x12     # shift right logical
    sra     x27, x11, x12     # shift right arithmetic
    or      x28, x11, x12     # or
    and     x29, x11, x12     # and

    # Done
    nop                      # idle

.data
data:
    .word 0x11223344
