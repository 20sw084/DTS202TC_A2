#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_ITERATIONS 20

// Function to read the PGM file
int readPGM(const char *filename, int **image, int *width, int *height, int *maxval) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return -1;
    }
    
    char format[3];
    if (fscanf(file, "%s", format) != 1 || format[0] != 'P' || format[1] != '2') {
        fprintf(stderr, "Invalid PGM format\n");
        fclose(file);
        return -1;
    }

    if (fscanf(file, "%d %d", width, height) != 2) {
        fprintf(stderr, "Error reading image dimensions\n");
        fclose(file);
        return -1;
    }

    if (fscanf(file, "%d", maxval) != 1) {
        fprintf(stderr, "Error reading max value\n");
        fclose(file);
        return -1;
    }

    *image = (int *)malloc((*width) * (*height) * sizeof(int));
    if (*image == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    for (int i = 0; i < (*width) * (*height); i++) {
        if (fscanf(file, "%d", &(*image)[i]) != 1) {
            fprintf(stderr, "Error reading pixel data\n");
            fclose(file);
            free(*image);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

// Function to write the PGM file
int writePGM(const char *filename, int *image, int width, int height, int maxval) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return -1;
    }

    fprintf(file, "P2\n");
    fprintf(file, "%d %d\n", width, height);
    fprintf(file, "%d\n", maxval);

    for (int i = 0; i < width * height; i++) {
        fprintf(file, "%d ", image[i]);
        if ((i + 1) % width == 0) {
            fprintf(file, "\n");
        }
    }

    fclose(file);
    return 0;
}

// Function to apply blur effect
void applyBlur(int *image, int *local_image, int *temp_image, int width, int local_height) {
    for (int i = 0; i < local_height; i++) {
        for (int j = 0; j < width; j++) {
            int sum = 0;
            int count = 0;

            // Apply the average blur over neighbors (including boundary checks)
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    int ni = i + di;
                    int nj = j + dj;
                    if (ni >= 0 && ni < local_height && nj >= 0 && nj < width) {
                        sum += local_image[ni * width + nj];
                        count++;
                    }
                }
            }

            // Store the blurred pixel value in temp_image
            temp_image[i * width + j] = sum / count;
        }
    }
}

// Main function implementing the blur
int main(int argc, char *argv[]) {
    int rank, size;
    int *image = NULL, *local_image = NULL;
    int width, height, maxval;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Root process reads the image
    if (rank == 0) {
        if (readPGM("image.pgm", &image, &width, &height, &maxval) != 0) {
            MPI_Finalize();
            return -1;
        }
    }
    
    // Broadcast the image dimensions and maxval to all processes
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&maxval, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocate space for local image portion
    int local_height = height / size;  // Each process handles a portion of the rows
    local_image = (int *)malloc(width * local_height * sizeof(int));

    // Scatter image data across all processes
    MPI_Scatter(image, width * local_height, MPI_INT, local_image, width * local_height, MPI_INT, 0, MPI_COMM_WORLD);

    // Perform the blur effect in parallel for each process
    int *temp_image = (int *)malloc(width * local_height * sizeof(int));

    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        applyBlur(image, local_image, temp_image, width, local_height);

        // Boundary row exchange between neighboring processes can be done here if needed.

        // Copy the temp_image to local_image after processing
        for (int i = 0; i < local_height; i++) {
            for (int j = 0; j < width; j++) {
                local_image[i * width + j] = temp_image[i * width + j];
            }
        }
    }

    // Gather the results back into the root process
    MPI_Gather(local_image, width * local_height, MPI_INT, image, width * local_height, MPI_INT, 0, MPI_COMM_WORLD);

    // Root process writes the final image
    if (rank == 0) {
        if (writePGM("blurred_image.pgm", image, width, height, maxval) != 0) {
            free(image);
            MPI_Finalize();
            return -1;
        }

        free(image);
    }

    free(local_image);
    free(temp_image);

    MPI_Finalize();
    return 0;
}
