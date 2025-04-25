    .section .text
    .global _start

_start:
    # Get address of the string using PC-relative addressing (position independent)
    auipc t0, 0            # t0 = PC

    # Call reverse_string(t0)
    mv a0, t0              # argument: pointer to string
    jal ra, reverse_string

    # Exit (ECALL)
    li a7, 10              # syscall for exit
    ecall

# ---------------------------
# Function: reverse_string
# Reverses a null-terminated string in-place.
# Input: a0 - pointer to string
# ---------------------------
reverse_string:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw a1, 8(sp)
    sw a2, 4(sp)

    mv a1, a0        # start pointer
    mv t1, a0        # temp pointer for end search

find_end:
    lbu t2, 0(t1)
    beq t2, zero, reverse_loop_prep
    addi t1, t1, 1
    j find_end

reverse_loop_prep:
    addi a2, t1, -1   # end pointer = t1 - 1

reverse_loop:
    bge a1, a2, reverse_done

    lbu t3, 0(a1)     # load char at start
    lbu t4, 0(a2)     # load char at end

    sb t4, 0(a1)      # store end char at start
    sb t3, 0(a2)      # store start char at end

    addi a1, a1, 1    # move start++
    addi a2, a2, -1   # move end--

    j reverse_loop

reverse_done:
    lw ra, 12(sp)
    lw a1, 8(sp)
    lw a2, 4(sp)
    addi sp, sp, 16
    ret

# ---------------------------
# Position-independent .data section
# ---------------------------
    .section .data
str:
    .ascii "hello world!\0"   # This string will be reversed in place

# ---------------------------
# .bss Section - Uninitialized Data
# ---------------------------
    .section .bss
buffer: 
    .skip 64                # Allocate 64 bytes of uninitialized memory

# ---------------------------
# .rodata Section - Read-Only Data
# ---------------------------
    .section .rodata
msg1:
    .string "String reversal completed successfully.\n"

# ---------------------------
# .text Section - Additional Functions (For Future Extension)
# ---------------------------
    .section .text
print_msg:
    # Assume we have a basic print function here (to be implemented for your environment)
    # This is just a placeholder function.
    li a7, 4          # syscall for print (in some environments)
    la a0, msg1       # load address of msg1 into a0
    ecall
    ret
