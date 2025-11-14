SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))
SV32_DIR := ../../src
SAD_DIR := ../../include/stubb_a_dub
SV32_SRCS := $(SV32_DIR)/threads_mgr.c

CFLAGS = -O0 -Wall -Wextra -g -DDEBUG_THREAD_MGR
LDFLAGS = -I$(SV32_DIR) -I$(SAD_DIR)

### BUILD
all: test

test: $(OBJS) threads_mgr.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ threads_mgr.o

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

threads_mgr.o:
	$(CC) $(CFLAGS) $(LDFLAGS) -c $(SV32_DIR)/threads_mgr.c -o threads_mgr.o

clean:
	rm *.o test
