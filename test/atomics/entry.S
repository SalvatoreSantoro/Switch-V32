.global _entry
.extern _stack_base
.extern __global_pointer$
.extern main

.section .text
_entry:
    la sp, _stack_base
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    call main

loop: j loop
