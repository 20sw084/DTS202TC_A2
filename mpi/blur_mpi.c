#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_ITERATIONS 20  // Define maximum iterations for blur effect

// Function to read a PGM file (Portable Gray Map) and store the image data
int readPGM(const char *filename, int **image, int *width, int *height, int *maxval) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return -1;  // Return error code if file could not be opened
    }
    
    char format[3];
    // Check if the PGM format is correct
    if (fscanf(file, "%s", format) != 1 || format[0] != 'P' || format[1] != '2') {
        fprintf(stderr, "Invalid PGM format\n");
        fclose(file);
        return -1;
    }

    // Read image dimensions and max pixel value
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

    // Allocate memory for the image data
    *image = (int *)malloc((*width) * (*height) * sizeof(int));
    if (*image == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    // Read the pixel data into the image array
    for (int i = 0; i < (*width) * (*height); i++) {
        if (fscanf(file, "%d", &(*image)[i]) != 1) {
            fprintf(stderr, "Error reading pixel data\n");
            fclose(file);
            free(*image);
            return -1;
        }
    }

    fclose(file);
    return 0;  // Return 0 on successful reading
}

// Function to write the PGM file
int writePGM(const char *filename, int *image, int width, int height, int maxval) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return -1;  // Return error code if file could not be opened
    }

    // Write the PGM header
    fprintf(file, "P2\n");
    fprintf(file, "%d %d\n", width, height);
    fprintf(file, "%d\n", maxval);

    // Write pixel data
    for (int i = 0; i < width * height; i++) {
        fprintf(file, "%d ", image[i]);
        // Add newline after each row of pixels
        if ((i + 1) % width == 0) {
            fprintf(file, "\n");
        }
    }

    fclose(file);
    return 0;  // Return 0 on successful writing
}

// Function to apply blur effect to the image
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

// Main function implementing the parallel blur using MPI
int main(int argc, char *argv[]) {
    int rank, size;
    int *image = NULL, *local_image = NULL;
    int width, height, maxval;
    
    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // Get the rank of the current process
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // Get the total number of processes
    
    // Root process reads the image from file
    if (rank == 0) {
        if (readPGM("image.pgm", &image, &width, &height, &maxval) != 0) {
            MPI_Finalize();
            return -1;
        }
    }
    
    // Broadcast the image dimensions and max pixel value to all processes
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&maxval, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocate memory for local image portion
    int local_height = height / size;  // Divide image rows among processes
    local_image = (int *)malloc(width * local_height * sizeof(int));

    // Scatter image data across all processes
    MPI_Scatter(image, width * local_height, MPI_INT, local_image, width * local_height, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocate temporary image for storing blur result
    int *temp_image = (int *)malloc(width * local_height * sizeof(int));

    // Apply blur effect for multiple iterations
    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        applyBlur(image, local_image, temp_image, width, local_height);

        // Boundary row exchange between neighboring processes can be done here if needed.

        // Copy the blurred image data from temp_image to local_image
        for (int i = 0; i < local_height; i++) {
            for (int j = 0; j < width; j++) {
                local_image[i * width + j] = temp_image[i * width + j];
            }
        }
    }

    // Gather the final image data from all processes back into the root process
    MPI_Gather(local_image, width * local_height, MPI_INT, image, width * local_height, MPI_INT, 0, MPI_COMM_WORLD);

    // Root process writes the final image to a file
    if (rank == 0) {
        if (writePGM("blurred_image.pgm", image, width, height, maxval) != 0) {
            free(image);
            MPI_Finalize();
            return -1;
        }

        free(image);  // Free allocated memory for the image
    }

    // Free memory allocated for local and temporary images
    free(local_image);
    free(temp_image);

    // Finalize MPI and exit
    MPI_Finalize();
    return 0;
}
