include ../build.mk

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

ASMS := ../entry.S
ASMOBJS := $(ASMS:.S=.o)

### BUILD
all: build_folder atomics.elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

atomics.elf: $(ASMOBJS) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o build/$@

build_folder:
	@mkdir -p build

objcopy: atomics.elf
	$(CROSS)objcopy -O binary build/atomics.elf build/atomics.bin

clean:
	rm -f *.o *.bin *.elf

