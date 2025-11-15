CC = $(CROSS)gcc

CFLAGS = -Wall -O0 -nostdlib -march=rv32ima -mabi=ilp32 -ffreestanding -fomit-frame-pointer -g
LDFLAGS = -T ../linker.ld
