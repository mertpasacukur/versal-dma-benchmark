# Makefile for DMA Benchmark Application
# Vitis 2023.2 - Versal VPK120

# Toolchain
TOOLCHAIN = C:/Xilinx_2023_2/Vitis/2023.2/gnu/aarch64/nt/aarch64-none/bin
CC = $(TOOLCHAIN)/aarch64-none-elf-gcc
LD = $(TOOLCHAIN)/aarch64-none-elf-gcc
OBJCOPY = $(TOOLCHAIN)/aarch64-none-elf-objcopy
SIZE = $(TOOLCHAIN)/aarch64-none-elf-size

# Directories
WORKSPACE = C:/Users/mpcukur/claude_code_ws/versal_dma/vitis/workspace_all_dma
PLATFORM = $(WORKSPACE)/dma_all_platform
BSP_DIR = $(PLATFORM)/psv_cortexa72_0/standalone_domain/bsp/psv_cortexa72_0
SRC_DIR = src
BUILD_DIR = Debug

# Include paths
INCLUDES = -I$(SRC_DIR) \
           -I$(SRC_DIR)/drivers \
           -I$(SRC_DIR)/utils \
           -I$(SRC_DIR)/tests \
           -I$(SRC_DIR)/scenarios \
           -I$(BSP_DIR)/include

# Library paths
LIB_DIRS = -L$(BSP_DIR)/lib

# Libraries
LIBS = -lxil -lgcc -lc

# Compiler flags
CFLAGS = -Wall -O2 -c -fmessage-length=0 \
         -march=armv8-a -DARMA72_EL3 -Dversal \
         -fno-tree-loop-distribute-patterns \
         $(INCLUDES)

# Linker flags
LDFLAGS = -march=armv8-a \
          -Wl,-T,$(SRC_DIR)/lscript.ld \
          -Wl,--start-group $(LIBS) -Wl,--end-group \
          $(LIB_DIRS)

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c) \
       $(wildcard $(SRC_DIR)/drivers/*.c) \
       $(wildcard $(SRC_DIR)/utils/*.c) \
       $(wildcard $(SRC_DIR)/tests/*.c) \
       $(wildcard $(SRC_DIR)/scenarios/*.c)

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Target
TARGET = $(BUILD_DIR)/dma_benchmark_app.elf

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)
	@echo "Build complete!"
	@$(SIZE) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/drivers
	mkdir -p $(BUILD_DIR)/utils
	mkdir -p $(BUILD_DIR)/tests
	mkdir -p $(BUILD_DIR)/scenarios

$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(LD) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

print-srcs:
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"
