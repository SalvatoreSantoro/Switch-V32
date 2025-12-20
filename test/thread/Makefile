SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))
SV32_DIR := ../../src
SV32_SRCS := $(SV32_DIR)/cthread.c

CFLAGS = -O3 -Wall -Wextra -g
LDFLAGS = -I$(SV32_DIR)

### BUILD
all: test

test: $(OBJS) cthread.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ cthread.o

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

cthread.o:
	$(CC) $(CFLAGS) $(LDFLAGS) -c $(SV32_DIR)/cthread.c -o cthread.o

clean:
	rm *.o test
