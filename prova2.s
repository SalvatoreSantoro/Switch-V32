    .section .data           # Data section for initialized data
msg:    .asciz "Hello, RISC-V!\n"  # String message

    .section .bss            # BSS section for uninitialized data
count:  .skip 4              # Reserve space for a 4-byte integer

    .section .text           # Code section
    .globl _start            # Declare the entry point

_start:
    # Initialize count to 0
    li      t0, 0            # Load immediate 0 into register t0
    la      t1, count        # Load address of 'count' into t1
    sw      t0, 0(t1)        # Store value of t0 (0) into count

    # Print the message "Hello, RISC-V!"
    la      a0, msg          # Load address of 'msg' into a0 (argument register for print)
    li      a7, 4            # Load the system call number for 'print_string' into a7
    ecall                    # Make the system call (print the message)

    # Increment the count value by 1
    la      t1, count        # Load address of 'count' into t1
    lw      t0, 0(t1)        # Load the value of 'count' into t0
    addi    t0, t0, 1        # Increment t0 by 1
    sw      t0, 0(t1)        # Store updated value back to 'count'

    # Exit the program
    li      a7, 10           # Load the system call number for 'exit' into a7
    ecall                    # Make the system call (exit the program)

