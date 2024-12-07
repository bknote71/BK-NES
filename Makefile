# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Iincludes

# Assembler and linker for 6502
ASM = ca65
LINKER = ld65

# Directories
SRC_DIR = src
INCLUDE_DIR = includes
TEST_DIR = test
BUILD_DIR = build

# Output files
EXECUTABLE = $(BUILD_DIR)/test_cpu
BINARY = $(BUILD_DIR)/summation.bin

# Files
SRC_FILES = $(SRC_DIR)/CPU.cpp
TEST_FILES = $(TEST_DIR)/test.cpp
ASM_FILE = $(TEST_DIR)/summation.asm
CFG_FILE = nes.cfg

# Default rule
all: $(EXECUTABLE) $(BINARY)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile CPU test executable
$(EXECUTABLE): $(BUILD_DIR) $(SRC_FILES) $(TEST_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC_FILES) $(TEST_FILES)

# Assemble factorial program
$(BINARY): $(BUILD_DIR) $(ASM_FILE)
	$(ASM) $(ASM_FILE) -o $(BUILD_DIR)/summation.o
	$(LINKER) $(BUILD_DIR)/summation.o -o $(BINARY) -C $(CFG_FILE)

# Clean build files
clean:
	rm -rf $(BUILD_DIR)
