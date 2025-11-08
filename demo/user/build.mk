CC = $(CROSS)gcc

CFLAGS += -Wall -std=gnu99 -O2 -march=rv32im -mabi=ilp32 -flto -fomit-frame-pointer -g 
