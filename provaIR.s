    .section .text
    .globl _start

_start:
    # assume x1 = 5
    li x1, 5          # load immediate value 5 into x1

    # ADD Immediate
    addi x2, x1, 10   # x2 = x1 + 10

    # Set Less Than Immediate
    slti x3, x1, 7    # x3 = (x1 < 7) ? 1 : 0

    # Set Less Than Immediate Unsigned
    sltiu x4, x1, 7   # x4 = (unsigned)x1 < 7 ? 1 : 0

    # XOR Immediate
    xori x5, x1, 3    # x5 = x1 ^ 3

    # OR Immediate
    ori x6, x1, 8     # x6 = x1 | 8

    # AND Immediate
    andi x7, x1, 12   # x7 = x1 & 12

    # Shift Left Logical Immediate
    slli x8, x1, 2    # x8 = x1 << 2

    # Shift Right Logical Immediate
    srli x9, x1, 1    # x9 = logical x1 >> 1

    # Shift Right Arithmetic Immediate
    srai x10, x1, 1   # x10 = arithmetic x1 >> 1

