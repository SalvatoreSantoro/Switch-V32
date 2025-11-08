# SwitchV32

SwitchV32 is a one man band project aimed to implement a simple virtual machine capable
of running programs.

This isn't production grade software, it's more of a toy project developed for learning
purposes.

The architecture of reference is RISC-V and the virtual machine is aggressively implemented
with the use of "switch-cases" in order to implement all the CPU logic (hence the name "SwitchV32").

The virtual machine can be compiled in 2 modes:
-USER
-SUPERVISOR

./configure --prefix=/opt/riscv32sup/ --with-multilib-generator="rv32im-ilp32--a_zifencei_zicsr*c" 
