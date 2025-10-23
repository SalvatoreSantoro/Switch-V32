NAME ?= doom_riscv
DEBUG_OPT :=

DEMO_PROG_PATH := $(shell find ./demo -type d -name "$(NAME)" 2>/dev/null)

# Determine whether it’s under 'user' or 'supervisor'
# (extract the second-level directory name)
ROLE := $(shell echo "$(DEMO_PROG_PATH)" | awk -F/ '/demo\/(user|supervisor)\// {print $$3; exit}')

ifdef DEBUG
DEBUG_OPT := -d
endif

# All directories inside include/
LIBS_DIRS := $(wildcard include/*)
DEMO_DIRS := $(wildcard demo/user/*) $(wildcard demo/supervisor/*)

# Static libraries (assumes build/lib<dirname>.a exists in each include/*)
LIBS := $(addprefix $(LIBS_DIRS)/build/lib, $(notdir $(LIBS_DIRS)))
LIBS := $(addsuffix .a, $(LIBS))

# Compiler include flags
INCLUDE_FLAGS := $(addprefix -I, $(LIBS_DIRS))

### VARIABLES & FLAGS
CC = gcc 
CFLAGS = -O2

### WARNINGS
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

LDFLAGS = -lSDL2 -I/usr/include/SDL2 $(INCLUDE_FLAGS)


MODE_DEFAULT := user
MODE_FLAGS := -DUSER
BIN_NAME := sv32_user

# Define macros per mode
ifeq ($(MODE),supervisor)
MODE_DEFAULT := supervisor
MODE_FLAGS := -DSUPERVISOR
BIN_NAME := sv32_supervisor
else
MODE_FLAGS := -DUSER
BIN_NAME := sv32_user
endif

CFLAGS += $(MODE_FLAGS)

### DIRECTORIES
SRC_DIR := src
BUILD_DIR := build

### SRCS and OBJS
SRCS_BASE := $(wildcard $(SRC_DIR)/*.c)
SRCS_MODE := $(wildcard $(SRC_DIR)/$(MODE_DEFAULT)/*.c)

OBJS_BASE := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.$(MODE_DEFAULT).o, $(SRCS_BASE))
OBJS_MODE := $(patsubst $(SRC_DIR)/$(MODE_DEFAULT)/%.c, $(BUILD_DIR)/%.$(MODE_DEFAULT).o, $(SRCS_MODE))

SRCS := $(SRCS_BASE) $(SRCS_MODE)
OBJS := $(OBJS_BASE) $(OBJS_MODE)

all: init $(BUILD_DIR) $(BUILD_DIR)/$(BIN_NAME)

.PHONY: init
init:
	-@git submodule update --init --recursive || true

# Rule: for each library, run make in its folder
$(LIBS): $(LIBS_DIRS) 
	@echo "Building library in $^"
	$(MAKE) -C $^

$(BUILD_DIR)/$(BIN_NAME): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@


$(BUILD_DIR)/%.$(MODE_DEFAULT).o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@


#valgrind: $(BUILD_DIR)/$(NAME)
	#valgrind --tool=memcheck --leak-check=full --track-origins=yes -s $(BUILD_DIR)/$(NAME) -f "$(DOOM_DIR)/$(DOOM)" -u 4 -o /dev/null -e /dev/null -d

elf: user supervisor
	$(MAKE) -C $(DEMO_PROG_PATH)
ifeq ($(ROLE), user)
	./build/sv32_user -f "$(DEMO_PROG_PATH)/build/$(NAME).elf" -u 4 -i /dev/null -o /dev/null -e /dev/null $(DEBUG_OPT)
else
	./build/sv32_supervisor -f "$(DEMO_PROG_PATH)/build/$(NAME).elf" -u 4 -i /dev/null -o /dev/null -e /dev/null $(DEBUG_OPT)
endif

bin: supervisor
ifeq ($(ROLE), user)
	@echo "BIN SUPPORTED ONLY FOR SUPERVISORS DEMO"
else
	$(MAKE) -C $(DEMO_PROG_PATH)
	./build/sv32_supervisor -f "$(DEMO_PROG_PATH)/build/$(NAME).bin" -b -i /dev/null -o /dev/null -e /dev/null $(DEBUG_OPT)
endif

# Separate targets for modes
# recursively run setting MODE
user:
	$(MAKE) MODE=user

supervisor:
	$(MAKE) MODE=supervisor


### CREATE BUILD DIRECTORY IF NOT EXISTS
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

### CLEAN BUILD FILES
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@for dir in $(LIBS_DIRS); do \
		if [ -f $$dir/Makefile ]; then \
			echo "Cleaning $$dir..."; \
			$(MAKE) -C $$dir clean; \
		fi \
	done
	@for dir in $(DEMO_DIRS); do \
		if [ -f $$dir/Makefile ]; then \
			echo "Cleaning $$dir..."; \
			$(MAKE) -C $$dir clean; \
		fi \
	done

.PHONY: help
help:
	@echo '          _____          _ _       _     _   _  _____  _____  '
	@echo '         /  ___|        (_) |     | |   | | | ||____ |/ __  \ '
	@echo '         \ `--.__      ___| |_ ___| |__ | | | |    / /`` / /` '
	@echo '          `--. \ \ /\ / / | __/ __| `_ \| | | |    \ \  / /   '
	@echo '         /\__/ /\ V  V /| | || (__| | | \ \_/ /.___/ /./ /___ '
	@echo '         \____/  \_/\_/ |_|\__\___|_| |_|\___/ \____/ \_____/ '
	@echo
	@echo "    ### ALWAYS RUN source settings.sh CROSS=/path/to/cross_compiler ###"
	@echo "    ### You can use the DEBUG parameter when running a demo program ###"
	@echo "    ###   to debug the application using GDB      (PORT 1234)       ###"
	@echo "    ###   version of VM used (USER/SUPERVISOR) changes according    ###"
	@echo "    ###   to the folder in which the demo program is in             ###"
	@echo
	@echo "    make init                -   initialize submodules"
	@echo "    make supervisor          -   compile with supervisor mode enabled"
	@echo "    make user                -   compile with only user mode enabled"
	@echo "    make                     -   compile with only user mode enabled"
	@echo "    make clean               -   clean the build directory (even of submodules)"
	@echo "    make elf NAME= (DEBUG)   -   run demo elf, NAME must be the same of the program folder inside demo"
	@echo "    make bin NAME= (DEBUG)   -   run demo bin, NAME must be the same of the program folder inside demo(ONLY SUPERVISOR)"
	@echo
