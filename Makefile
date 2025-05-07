
### VARIABLES & FLAGS

# useful: -Wpadded

NAME:=SRV32

CC:=gcc
CFLAGS:= -O2 -Wall -Wextra -lSDL2 #-Werror

### DIRECTORIES
SRC_DIR:=srv32
BUILD_DIR:=build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))


### BUILD
all: $(BUILD_DIR)/$(NAME)

$(BUILD_DIR)/$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BUILD_DIR)/$(NAME)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(BUILD_DIR)/$(NAME) 
	$(BUILD_DIR)/$(NAME)


### CREATE BUILD DIRECTORY IF NOT EXISTS
$(BUILD_DIR):
	@"mkdir" $(BUILD_DIR)


### SANITIZERS
.PHONY: valgrind
valgrind: test
	valgrind --tool=memcheck --leak-check=full --track-origins=yes -s $(BUILD_DIR)/test_binary

.PHONY: asan
asan: $(TEST_OBJS)
	$(CC) $(CFLAGS) -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer $(OBJS) -o $(BUILD_DIR)/$(NAME)
	@echo "Running tests..."
	$(BUILD_DIR)/$(NAME)

### CLEAN BUILD FILES
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
