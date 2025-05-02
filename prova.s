.section .data
test_data:
    .byte   0x7F          # 127   -> +ve byte
    .byte   0x80          # -128  -> negative byte (0x80)
    .hword  0x7FFF        # 32767 -> +ve halfword
    .hword  0x8000        # -32768 -> negative halfword (0x8000)
    .word   0x80000000    # -2147483648 -> word (most negative 32-bit)

.section .text
.globl _start
_start:
    la   t0, test_data     # Load base address

    # Test LB (signed byte)
    lb   t1, 0(t0)          # t1 = 0x7F  ( 127)
    lb   t2, 1(t0)          # t2 = 0xFFFFFF80 (-128 sign-extended)

    # Test LBU (unsigned byte)
    lbu  t3, 0(t0)          # t3 = 0x0000007F
    lbu  t4, 1(t0)          # t4 = 0x00000080

    # Test LH (signed halfword)
    lh   t5, 2(t0)          # t5 = 0x7FFF (32767)
    lh   t6, 4(t0)          # t6 = 0xFFFF8000 (-32768 sign-extended)

    # Test LHU (unsigned halfword)
    lhu  t4, 2(t0)          # t7 = 0x00007FFF
    lhu  t5, 4(t0)          # t8 = 0x00008000

    # Test LW (word) - just loads directly, no extension involved
    lw   t6, 6(t0)          # t9 = 0x80000000

    # You can add breakpoints here or use a debug monitor to inspect t1–t9

    # Infinite loop to stop execution (optional)
    j    .
