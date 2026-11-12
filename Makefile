# Compilers and Tools
FC      := gfortran
CC      := gcc
GLSLC   := glslc

# Target Executable and Directories
TARGET  := sim
BUILD   := build

# Compiler and Linker Flags
FFLAGS  := -O2 -Wall -Wextra -fcheck=all -fopenmp -g -J$(BUILD)
CFLAGS  := -O2 -Wall -Wextra -g -std=c11
LDFLAGS := -lvulkan -lglfw -lm -fopenmp

# Source Files
# Note: List Fortran files in dependency order (modules first, main.f90 last)
F_SRCS  := camera_mod.f90 world_mod.f90 main.f90
C_SRCS  := vk_backend.c

# Shader Files (Automatically finds any .vert, .frag, or .comp files)
SHADERS := $(wildcard *.vert *.frag *.comp)
SPV     := $(patsubst %, $(BUILD)/%.spv, $(SHADERS))

# Object Files
OBJS    := $(patsubst %.c, $(BUILD)/%.o, $(C_SRCS)) \
           $(patsubst %.f90, $(BUILD)/%.o, $(F_SRCS))

OMP_THREADS := 8

# Default Target
.PHONY: all run clean
all: $(SPV) $(TARGET)

# Link the executable (using gfortran so libgfortran is linked automatically)
$(TARGET): $(OBJS)
	$(FC) $(FFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile C sources
$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Fortran sources
$(BUILD)/%.o: %.f90 | $(BUILD)
	$(FC) $(FFLAGS) -c $< -o $@

# Compile GLSL shaders to SPIR-V (Targets Vulkan 1.3)
$(BUILD)/%.spv: % | $(BUILD)
	$(GLSLC) --target-env=vulkan1.3 $< -o $@

# Create build directory if it doesn't exist
$(BUILD):
	mkdir -p $(BUILD)

# Convenience target to build and run
run: all
	OMP_NUM_THREADS=$(OMP_THREADS) ./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD) $(TARGET) *.mod
