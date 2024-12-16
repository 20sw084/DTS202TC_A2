.PHONY: build run pack clean

CC = gcc

CFLAGS = -Wall -Wextra -std=c99

LIBS=-lm -lpthread

SRC = blur_pthreads.c

EXE = blur

build: $(SRC)
	$(CC) $(CFLAGS) $^ -o $(EXE) $(LIBS)

run: build
	@echo "Running with parameters:"
	@echo "Input Image: $(IN)"
	@echo "Output Image: $(OUT)"
	@echo "Filters: $(FILTERS)"
	@echo "Number of Threads: $(NUM_THREADS)"
	./$(EXE) $(IN) $(OUT) $(FILTERS) $(NUM_THREADS)

clean:
	rm -f *.o $(EXE)
