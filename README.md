# DTS202TC_A2 — Parallel Image Blur Implementations

This repository contains multiple C implementations of an image blur task using different parallel computing models:

- **Pthreads**
- **OpenMP**
- **MPI**
- **CUDA**

The project appears to be coursework focused on comparing approaches for parallelizing image filtering.

## Repository Structure

```text
DTS202TC_A2/
├── pthread/
│   ├── blur_pthread.c
│   └── Makefile
├── openmp/
│   ├── blur_openmp.c
│   └── Makefile
├── mpi/
│   ├── blur_mpi.c
│   └── Makefile
└── cuda/
    ├── blur_cuda.c
    └── Makefile
```

## Prerequisites

Install a Linux toolchain with:

- `gcc`
- `make`
- `pthread` support (usually included with glibc)
- `OpenMP` support in GCC (`-fopenmp`)
- `MPI` toolchain (`mpicc`, `mpirun`) for MPI build/run
- `CUDA Toolkit` (`nvcc`) and supported NVIDIA GPU for CUDA build/run

## Implementation Notes

## 1) Pthreads (`pthread/blur_pthread.c`)

- Reads and writes **binary PGM/PPM** style image data (`P5`/`P6` logic via header type field).
- Applies a **3x3 Gaussian kernel** repeatedly.
- CLI:
  ```bash
  ./blur <input_file> <output_file> <num_filters> <num_threads>
  ```

## 2) OpenMP (`openmp/blur_openmp.c`)

- Reads and writes **ASCII PGM (`P2`)**.
- Applies a **3x3 average blur** for a fixed `MAX_ITERATIONS` (currently `20`).
- CLI:
  ```bash
  ./blur_openmp <input.pgm> <output.pgm>
  ```

## 3) MPI (`mpi/blur_mpi.c`)

- Uses MPI to split rows across processes and apply iterative blur.
- Current source reads from a hardcoded input file `image.pgm` and writes to `blurred_image.pgm`.
- Launch example:
  ```bash
  mpirun -np 4 ./blur_mpi
  ```

## 4) CUDA (`cuda/blur_cuda.c`)

- Demonstrates a CUDA 2D kernel applying a box blur.
- Current source initializes synthetic random image data in memory (1024×1024) and prints first output pixel.
- CLI currently expects:
  ```bash
  ./blur_cuda <input_image> <output_image> <num_threads>
  ```
  but file arguments are not yet used by the program logic.

## Build Instructions

### Pthreads

```bash
cd /home/runner/work/DTS202TC_A2/DTS202TC_A2/pthread
make build
```

> Note: the current `pthread/Makefile` uses `SRC = blur_pthreads.c` while the file is `blur_pthread.c`.  
> If build fails, update `SRC` accordingly.

### OpenMP

```bash
cd /home/runner/work/DTS202TC_A2/DTS202TC_A2/openmp
gcc -fopenmp -o blur_openmp blur_openmp.c
```

> Note: `openmp/Makefile` currently references undefined MPI variables, so direct `gcc` compilation is the reliable path as-is.

### MPI

```bash
cd /home/runner/work/DTS202TC_A2/DTS202TC_A2/mpi
mpicc -O2 -Wall -o blur_mpi blur_mpi.c
```

### CUDA

```bash
cd /home/runner/work/DTS202TC_A2/DTS202TC_A2/cuda
make
```

## Run Examples

### Pthreads

```bash
cd /home/runner/work/DTS202TC_A2/DTS202TC_A2/pthread
./blur input.ppm output.ppm 5 8
```

### OpenMP

```bash
cd /home/runner/work/DTS202TC_A2/DTS202TC_A2/openmp
./blur_openmp input.pgm output.pgm
```

### MPI

```bash
cd /home/runner/work/DTS202TC_A2/DTS202TC_A2/mpi
mpirun -np 4 ./blur_mpi
```

### CUDA

```bash
cd /home/runner/work/DTS202TC_A2/DTS202TC_A2/cuda
./blur_cuda input.pgm output.pgm 256
```

## Suggested Next Improvements

- Normalize file I/O behavior across all implementations (common input/output interface).
- Fix Makefiles to match actual source filenames and targets.
- Add benchmark scripts for fair runtime comparison between methods.
- Add sample images and expected output artifacts.
- Add validation tests to compare output quality across implementations.

## License

No license file is currently present in the repository. Add one if you plan to distribute or reuse this code publicly.
