NVCC = nvcc

CFLAGS = -O2 -arch=sm_52

SRC = blur_cuda.c  # Use .c extension

EXE = blur_cuda

all: $(EXE)

$(EXE): $(SRC)
	$(NVCC) $(CFLAGS) $(SRC) -o $(EXE)

clean:
	rm -f $(EXE)
