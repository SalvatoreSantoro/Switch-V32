CC = $(CROSS)gcc

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

ASMS := entry.S
ASMOBJS := $(ASMS:.S=.o)

SV32_DIR := ../../src

CFLAGS = -Wall -O0 -nostdlib -march=rv32ima -mabi=ilp32 -fomit-frame-pointer -g
LDFLAGS = -T linker.ld

### BUILD
all: atomics.elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

atomics.elf: $(ASMOBJS) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

objcopy: atomics.elf
	$(CROSS)objcopy -O binary atomics.elf atomics.bin

clean:
	rm -f *.o *.bin *.elf

