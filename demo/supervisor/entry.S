.global _entry
.extern _stack_base
.extern __global_pointer$
.extern main

.section .text
_entry:
#a0 contains the hart id, used to create different cores stacks
	li t0, 4096
	la t1, _stack_base
	mul a0, a0, t0
	add a0, a0, t1
	mv sp, a0
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    call main

loop: j loop
