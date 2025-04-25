    .section .data
msg:    .asciz "Hello, RISC-V!\n"  # Declare a null-terminated string

    .section .bss
    .lcomm buffer, 64  # Reserve 64 bytes of space in memory

    .section .init
    .globl _start

# Initialization code that runs before the main program
init:
    # Set up some initial state or prepare the environment
    li a0, 0            # Load an immediate value (just an example)
    li a7, 64           # Syscall number for 'write' (64 in RISC-V Linux)
    li a1, 1            # File descriptor 1 (stdout)
    li a2, 10           # Length for a simple newline
    ecall               # Call the system call (to print something if needed)

    # After initialization, jump to the main program
    j _start            # Jump to main program

    .section .text
_start:
    # Main program code
    # Print the message
    la a0, msg          # Load the address of the string into a0
    li a7, 64           # Syscall number for 'write' (64 in RISC-V Linux)
    li a1, 1            # File descriptor 1 (stdout)
    li a2, 14           # Length of the string "Hello, RISC-V!\n"
    ecall               # Make the system call to print the message

    # Example of using .fini section (code to run before program exits)
    j fini               # Jump to fini section

    .section .fini
fini:
    # Code to run before program exits
    li a7, 93           # Syscall number for 'exit' (93 in RISC-V Linux)
    li a0, 0            # Return code 0
    ecall               # Make the system call to exit
