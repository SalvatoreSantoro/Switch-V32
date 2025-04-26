.data
arr: .word 5, 2, 9, 1, 5, 6
n: .word 6
.text
.globl _start
_start:
la t0,arr
lw t1,n
addi t2,t1,-1
addi t3,x0,0
outer_loop:
    addi t4, t3, 1
    addi t5, x0, 0 
inner_loop:
slli t6,t3,2
add t7,t0,t6
    lw   t8, 0(t7)
    addi t6, t6, 4
    add  t7, t0, t6
    lw   t9, 0(t7)
    blt  t8, t9, no_swap
    sw   t9, 0(t7)
    sw   t8, 0(t7)
    addi t5, t5, 1
no_swap:
    addi t4, t4, 1
    blt  t4, t2, inner_loop
    addi t3, t3, 1
    blt  t3, t2, outer_loop
    j _start
