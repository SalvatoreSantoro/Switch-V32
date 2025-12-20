# Project name
LIB_NAME := stubb_a_dub
LIB_DIR := src
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_OUTPUT := $(BUILD_DIR)/lib$(LIB_NAME).a

# Compiler and flags
CC := gcc
CFLAGS := -std=c99 -fPIC -O2 
# -DSAD_DEBUG
INCLUDES := -I$(LIB_DIR) -I.

CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpedantic -pedantic-errors
CFLAGS += -Wno-unused-parameter
CFLAGS += -Waggregate-return
CFLAGS += -Wbad-function-cast
CFLAGS += -Wcast-align
CFLAGS += -Wcast-qual
CFLAGS += -Wfloat-equal
CFLAGS += -Wformat=2
CFLAGS += -Wlogical-op
CFLAGS += -Wmissing-declarations
CFLAGS += -Wmissing-include-dirs
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wnested-externs
CFLAGS += -Wpointer-arith
CFLAGS += -Wredundant-decls
CFLAGS += -Wsequence-point
CFLAGS += -Wshadow
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wswitch
CFLAGS += -Wundef
CFLAGS += -Wunreachable-code
CFLAGS += -Wunused-but-set-parameter
CFLAGS += -Wwrite-strings
CFLAGS += -Wswitch-default 
CFLAGS += -Wconversion


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

cppcheck:
	cppcheck --enable=all --check-level=exhaustive --force --inconclusive --quiet --suppress=missingIncludeSystem $(INCLUDE_FLAGS) src/

# Phony targets
.PHONY: all clean
