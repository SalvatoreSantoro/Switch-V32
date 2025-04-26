    .text
    .globl _start

_start:
    # R-Type operations

    add  a0, t0, t1    # a0 = t0 + t1
    sub  a1, t1, t0    # a1 = t1 - t0
    sll  a2, t2, t3    # a2 = t2 << t3 (shift left logical)
    slt  a3, t0, t1    # a3 = (t0 < t1) ? 1 : 0 (signed compare)
    sltu a4, t4, t5    # a4 = (t4 < t5) ? 1 : 0 (unsigned compare)
    xor  a5, t2, t6    # a5 = t2 ^ t6 (bitwise XOR)
    srl  a6, t6, t3    # a6 = t6 >> t3 (logical shift right)
    sra  a7, t4, t3    # a7 = t4 >>> t3 (arithmetic shift right)
    or   s0, t2, t6    # s0 = t2 | t6 (bitwise OR)
    and  s1, t2, t6    # s1 = t2 & t6 (bitwise AND)

    # End: infinite loop
end:
    j end             # jump to itself forever
