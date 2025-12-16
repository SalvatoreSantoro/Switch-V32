SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))
SV32_DIR := ../../src
SV32_SRCS := $(SV32_DIR)/thread.c

CFLAGS = -O0 -Wall -Wextra -g
LDFLAGS = -I$(SV32_DIR)

### BUILD
all: test

test: $(OBJS) thread.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ thread.o

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

thread.o:
	$(CC) $(CFLAGS) $(LDFLAGS) -c $(SV32_DIR)/thread.c -o thread.o

clean:
	rm *.o test
