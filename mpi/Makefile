# Compiler and Flags
CC = mpicc
CFLAGS = -O2 -Wall
OPENMPFLAGS = -fopenmp
OUTPUT_MPI = blur_mpi
OUTPUT_OPENMP = blur_openmp
SRCS_MPI = blur_mpi.c
SRCS_OPENMP = blur_openmp.c

# Target for MPI program
$(OUTPUT_MPI): $(SRCS_MPI)
	$(CC) $(CFLAGS) -o $(OUTPUT_MPI) $(SRCS_MPI)

# Target for OpenMP program
$(OUTPUT_OPENMP): $(SRCS_OPENMP)
	gcc $(OPENMPFLAGS) -o $(OUTPUT_OPENMP) $(SRCS_OPENMP)

# Clean up compiled files
clean:
	rm -f $(OUTPUT_MPI) $(OUTPUT_OPENMP)

# Default target (when you run "make" without arguments)
all: $(OUTPUT_MPI) $(OUTPUT_OPENMP)
