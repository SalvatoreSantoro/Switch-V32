### VARIABLES & FLAGS
NAME = SV32

DOOM_DIR = doom_riscv/src/riscv
DOOM = doom-riscv.elf
CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -lSDL2 -I/usr/include/SDL2 
CROSS ?=

### DIRECTORIES
SRC_DIR:=src
BUILD_DIR:=build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))


### BUILD
all: $(BUILD_DIR)/$(NAME)

$(BUILD_DIR)/$(NAME): $(OBJS)
	$(CC) $(CFLAGS)  $(OBJS) -o $(BUILD_DIR)/$(NAME)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


doom: $(BUILD_DIR)/$(NAME)
	git submodule update --init --recursive
	$(MAKE) -C $(DOOM_DIR) CROSS="$(CROSS)"
	$(BUILD_DIR)/$(NAME) -f "$(DOOM_DIR)/$(DOOM)" -u 4 -o /dev/null -e /dev/null

### CREATE BUILD DIRECTORY IF NOT EXISTS
$(BUILD_DIR):
	@"mkdir" $(BUILD_DIR)

### CLEAN BUILD FILES
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) clean -C $(DOOM_DIR)
