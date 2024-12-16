CC = mpicc
CFLAGS = -O2 -Wall
OPENMPFLAGS = -fopenmp
OUTPUT_MPI = blur_mpi
OUTPUT_OPENMP = blur_openmp
SRCS_MPI = blur_mpi.c
SRCS_OPENMP = blur_openmp.c

$(OUTPUT_MPI): $(SRCS_MPI)
	$(CC) $(CFLAGS) -o $(OUTPUT_MPI) $(SRCS_MPI)

$(OUTPUT_OPENMP): $(SRCS_OPENMP)
	gcc $(OPENMPFLAGS) -o $(OUTPUT_OPENMP) $(SRCS_OPENMP)

clean:
	rm -f $(OUTPUT_MPI) $(OUTPUT_OPENMP)

all: $(OUTPUT_MPI) $(OUTPUT_OPENMP)
