### VARIABLES & FLAGS
NAME = SV32

DOOM_DIR = doom_riscv/src/riscv
DOOM = doom-riscv.elf
CC = gcc -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
CFLAGS = -O2 -Wall -Wextra  -lSDL2 -I/usr/include/SDL2 
### -Werror
CROSS ?=

### DIRECTORIES
SRC_DIR := src
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

### GDB LIBRARY INTEGRATION
GDB_DIR := $(SRC_DIR)/gdb
GDB_LIB := $(GDB_DIR)/build/libstubb_a_dub.a
GDB_INCLUDE := -I$(GDB_DIR)/src

### BUILD
all: $(BUILD_DIR)/$(NAME)

$(BUILD_DIR)/$(NAME): $(OBJS) $(GDB_LIB)
	$(CC) $(CFLAGS) $(GDB_INCLUDE) $(OBJS) $(GDB_LIB) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GDB_INCLUDE) -c $< -o $@

$(GDB_LIB):
	$(MAKE) -C $(GDB_DIR) EXTRA_CFLAGS="$(CFLAGS)"

doom: $(BUILD_DIR)/$(NAME)
	git submodule update --init --recursive
	$(MAKE) -C $(DOOM_DIR) CROSS="$(CROSS)"
	$(BUILD_DIR)/$(NAME) -f "$(DOOM_DIR)/$(DOOM)" -u 4 -o /dev/null -e /dev/null -d

valgrind: $(BUILD_DIR)/$(NAME)
	git submodule update --init --recursive
	$(MAKE) -C $(DOOM_DIR) CROSS="$(CROSS)"
	valgrind --tool=memcheck --leak-check=full --track-origins=yes -s $(BUILD_DIR)/$(NAME) -f "$(DOOM_DIR)/$(DOOM)" -u 4 -o /dev/null -e /dev/null -d



### CREATE BUILD DIRECTORY IF NOT EXISTS
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

### CLEAN BUILD FILES
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	#$(MAKE) clean -C $(DOOM_DIR)
	$(MAKE) clean -C $(GDB_DIR)

