.section .text
    .org 0x0          # Place code at virtual address 0x0 when linked/relocated
.global _start
_start:
    j   loop_b        # Jump to second jump instruction (loop_b)

loop_b:
    j   _start        # Second jump instruction; jumps back to the first
