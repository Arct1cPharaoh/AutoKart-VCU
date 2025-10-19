# Compiler settings
CC = gcc
# TODO: Remove -g and -O0 flags for production
CFLAGS = -g -O0 -IInc -pthread

# Base source and test directories
SRC_DIR = Src
TEST_DIR = Tests

# Object files directory
OBJ_DIR = Obj

# Set empty SOURCES variable
SOURCES = ""

ifdef TEST_MODE
CFLAGS += -DTEST_MODE -IInc/fake_include
LDFLAGS = -pthread -lm -lrt
# Exclude main.c from Src and include Tests/main.c instead
SOURCES = $(shell find $(SRC_DIR) -type f -name '*.c' \
	      ! -name 'main.c' \
	      ! -name 'stm32*.c' \
	      ! -name '*hal*.c' \
	      ! -name '*HAL*.c' \
	      ! -name 'syscalls*.c' \
	      ! -name 'sysmem*.c' \
	      ! -name 'system_stm32*.c' \
	      ! -name 'startup_stm32*.c' \
	      ! -name 'freertos.c' \
	      ! -path '*/STM32*/*' \
	      ! -path '*/Drivers/*')
# Add all test files including Tests/main.c
SOURCES += $(shell find $(TEST_DIR) -type f -name '*.c')
else
CFLAGS += -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include/ -I../Drivers/CMSIS/Include/ -I../Drivers/STM32F7xx_HAL_Driver/Inc/ -DSTM32F767xx
LDFLAGS = -pthread -lm
SOURCES = $(shell find $(SRC_DIR) -type f -name '*.c')
endif

# Create corresponding object files paths
ifdef TEST_MODE
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/$(SRC_DIR)/%.o,$(filter $(SRC_DIR)/%,$(SOURCES))) \
	      $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/$(TEST_DIR)/%.o,$(filter $(TEST_DIR)/%,$(SOURCES)))
else
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
endif

# Default target
ifdef TEST_MODE
all: prepare test_runner.out
else
all: prepare main.out
endif

# Prepare directory structure
prepare:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/$(SRC_DIR)
	@mkdir -p $(OBJ_DIR)/$(TEST_DIR)
	@echo "Directory structure created."

# Test runner build
test_runner.out: $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "Built test runner: $@"

# Main program build
main.out: $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "Built main program: $@"

# Rule for compiling C source to object files in SRC_DIR
$(OBJ_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# Rule for compiling C source to object files in TEST_DIR
$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# Clean rule
clean:
	rm -rf $(OBJ_DIR) main.out test_runner.out
	@echo "Cleaned up build files."

# Include dependencies
-include $(OBJECTS:.o=.d)