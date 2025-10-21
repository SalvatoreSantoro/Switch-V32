CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE = $(CROSS)size

CFLAGS = -Wall -O2 -march=rv32im -mabi=ilp32 -flto -fomit-frame-pointer -g 
