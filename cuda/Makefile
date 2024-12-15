# Makefile for CUDA image blur program

# Compiler
NVCC = nvcc

# Compiler flags
CFLAGS = -O2 -arch=sm_52

# Source file
SRC = blur_cuda.c  # Use .c extension

# Executable name
EXE = blur_cuda

# Default target
all: $(EXE)

# Build target
$(EXE): $(SRC)
	$(NVCC) $(CFLAGS) $(SRC) -o $(EXE)

# Clean target
clean:
	rm -f $(EXE)
