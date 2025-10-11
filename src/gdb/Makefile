# Project name
LIB_NAME := stubb_a_dub
LIB_DIR := src
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_OUTPUT := $(BUILD_DIR)/lib$(LIB_NAME).a

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -fPIC -O2 -DSAD_DEBUG
INCLUDES := -I$(LIB_DIR)

# Source and object files
SRC := $(wildcard $(LIB_DIR)/*.c)
OBJ := $(patsubst $(LIB_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# Default target
all: $(LIB_OUTPUT)

# Create static library
$(LIB_OUTPUT): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	ar rcs $@ $^

# Compile .c files to .o
$(OBJ_DIR)/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Phony targets
.PHONY: all clean
