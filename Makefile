#####################################
##	  		 COMPILER 			   ##
#####################################

CC = gcc
CFLAGS = -std=c99 -O2 -fsanitize=undefined -fsanitize=address

#####################################
##	  		 WARNINGS 			   ##
#####################################

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
CFLAGS += -Wunused
CFLAGS += -Wunreachable-code
CFLAGS += -Wunused-but-set-parameter
CFLAGS += -Wwrite-strings
CFLAGS += -Wswitch-default 
CFLAGS += -Wconversion


#####################################
##	  	      PARAMS    	       ##
#####################################

#defaults
MODE := user
MODE_FLAGS := -DUSER
INCLUDE_MODE := -Isrc/user
BIN_NAME := sv32_user
DEBUG_OPT :=

#####################################
##	  	        DEMO    	       ##
#####################################

#default demo
NAME_DEMO ?= doom_riscv
DEMO_DIRS := $(wildcard demo/user/*) $(wildcard demo/supervisor/*)
DEMO_PROG_PATH := $(shell find ./demo -type d -name "$(NAME_DEMO)" 2>/dev/null)
# Determine whether it’s under 'user' or 'supervisor'
# (extract the second-level directory name based on DEMO_PROG_PATH)
MODE := $(shell echo "$(DEMO_PROG_PATH)" | awk -F/ '/demo\/(user|supervisor)\// {print $$3; exit}')

#supervisor params
ifeq ($(MODE),supervisor)
MODE := supervisor
MODE_FLAGS := -DSUPERVISOR
INCLUDE_MODE := -Isrc/supervisor
BIN_NAME := sv32_supervisor
endif

#enable debugging
ifdef DEBUG
DEBUG_OPT := -d
endif

#####################################
##	  		 LIBRARIES 			   ##
#####################################

LIBS_DIRS := $(wildcard include/*)

# Static libraries (assumes build/lib<dirname>.a exists in each include/*)
LIBS := $(addprefix $(LIBS_DIRS)/build/lib, $(notdir $(LIBS_DIRS)))
LIBS := $(addsuffix .a, $(LIBS))

# compiler include flags
INCLUDE_FLAGS := $(addprefix -I, $(LIBS_DIRS))
INCLUDE_FLAGS += -I/usr/include/SDL2 -Isrc
INCLUDE_FLAGS += $(INCLUDE_MODE)
# linker flag
LDFLAGS = -lSDL2  


#####################################
##        SOURCES and OBJECTS      ##
#####################################

#main sv32 directories
SRC_DIR := src
BUILD_DIR := build

#main sv32 srcs and objs
SRCS_BASE := $(wildcard $(SRC_DIR)/*.c)
SRCS_MODE := $(wildcard $(SRC_DIR)/$(MODE)/*.c)

OBJS_BASE := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.$(MODE).o, $(SRCS_BASE))
OBJS_MODE := $(patsubst $(SRC_DIR)/$(MODE)/%.c, $(BUILD_DIR)/%.$(MODE).o, $(SRCS_MODE))

SRCS := $(SRCS_BASE) $(SRCS_MODE)
OBJS := $(OBJS_BASE) $(OBJS_MODE)

#####################################
##             TARGETS             ##
#####################################

all: init $(BUILD_DIR) $(BUILD_DIR)/$(BIN_NAME)

.PHONY: init
init:
	-@git submodule update --init --recursive || true

# Rule: for each library, run make in its folder
$(LIBS): $(LIBS_DIRS) 
	@echo "Building library in $^"
	#CFLAGS=$(CFLAGS)
	$(MAKE) CC=$(CC) -C $^ 

$(BUILD_DIR)/$(BIN_NAME): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(MODE_FLAGS) $(LDFLAGS) $(INCLUDE_FLAGS) $(OBJS) $(LIBS) -o $@

$(BUILD_DIR)/%.$(MODE).o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(MODE_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@

$(BUILD_DIR)/%.$(MODE).o: $(SRC_DIR)/$(MODE)/%.c
	$(CC) $(CFLAGS) $(MODE_FLAGS) $(INCLUDE_FLAGS) -c $< -o $@

cppcheck:
	cppcheck --enable=all --check-level=exhaustive --force --inconclusive --quiet --suppress=missingIncludeSystem $(INCLUDE_FLAGS) src/

clang-tidy:
	clang-tidy src/* 

valgrind: user supervisor
	$(MAKE) -C $(DEMO_PROG_PATH)
	valgrind --suppressions=sdl.supp --tool=memcheck --leak-check=full --track-origins=yes -s ./build/$(BIN_NAME) -f "$(DEMO_PROG_PATH)/build/$(NAME_DEMO).elf" -m 4096 -u 4 -c 4 -i /dev/null -o /dev/null -e /dev/null $(DEBUG_OPT)

helgrind: user supervisor
	$(MAKE) -C $(DEMO_PROG_PATH)
	valgrind --tool=helgrind ./build/$(BIN_NAME) -f "$(DEMO_PROG_PATH)/build/$(NAME_DEMO).elf" -m 4096 -u 4 -i /dev/null -o /dev/null -e /dev/null $(DEBUG_OPT)

elf: $(MODE)
ifeq ($(MODE), user)
	$(MAKE) -C $(DEMO_PROG_PATH)
	./build/$(BIN_NAME) -f "$(DEMO_PROG_PATH)/build/$(NAME_DEMO).elf" -m 4096 -u 4 -i /dev/null -o /dev/null -e /dev/null $(DEBUG_OPT)
else
	$(MAKE) -C $(DEMO_PROG_PATH)
	./build/$(BIN_NAME) -f "$(DEMO_PROG_PATH)/build/$(NAME_DEMO).elf" -m 4096 -c 4 -u 4 -i /dev/null -o /dev/null -e /dev/null $(DEBUG_OPT)
endif


bin: supervisor
ifeq ($(MODE), user)
	@echo "BIN SUPPORTED ONLY FOR SUPERVISORS DEMO"
else
	$(MAKE) -C $(DEMO_PROG_PATH)
	./build/sv32_supervisor -f "$(DEMO_PROG_PATH)/build/$(NAME_DEMO).bin" -m 4096 -c 4 -b -i /dev/null -o /dev/null -e /dev/null $(DEBUG_OPT)
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
	# @for dir in $(DEMO_DIRS); do \
	# 	if [ -f $$dir/Makefile ]; then \
	# 		echo "Cleaning $$dir..."; \
	# 		$(MAKE) -C $$dir clean; \
	# 	fi \
	# done

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
	@echo "    make elf NAME_DEMO= (DEBUG)   -   run demo elf, NAME_DEMO must be the same of the program folder inside demo"
	@echo "    make clean NAME_DEMO= 		   -   run clean of the NAME_DEMO program"
	@echo "    make bin NAME_DEMO= (DEBUG)   -   run demo bin, NAME_DEMO must be the same of the program folder inside demo(ONLY SUPERVISOR)"
	@echo "    make valgrind NAME_DEMO= (DEBUG)   -   run demo elf running the vm with valgrind, NAME_DEMO must be the same of the program folder inside demo(ONLY SUPERVISOR)"
	@echo "    make helgrind NAME_DEMO= (DEBUG)   -   run demo elf running the vm with helgrind, NAME_DEMO must be the same of the program folder inside demo(ONLY SUPERVISOR)"
	@echo "    make cppcheck   -   run static analysis with cppcheck"
	@echo "    make clang-tidy   -   run static analysis with clang-tidy"
	@echo
