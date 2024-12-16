#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define MAX_ITERATIONS 20  // Maximum number of iterations for the blur effect

// Function prototypes
void readPGM(const char *filename, int **image, int *width, int *height, int *maxval);
void writePGM(const char *filename, int *image, int width, int height, int maxval);
void blur(int *image, int *local_image, int width, int height);

// Main function to perform image blurring
int main(int argc, char *argv[]) {
    // Check if the correct number of arguments are provided
    if (argc != 3) {
        printf("Usage: %s <input.pgm> <output.pgm>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];
    char *output_file = argv[2];

    int *image = NULL;
    int *local_image = NULL;
    int width, height, maxval;

    // Read the input image
    readPGM(input_file, &image, &width, &height, &maxval);

    // Allocate memory for the local image (temporary storage for blurring)
    local_image = (int *)malloc(width * height * sizeof(int));
    if (local_image == NULL) {
        printf("Error: Unable to allocate memory for the image.\n");
        return 1;
    }

    // Repeat the blur effect for a fixed number of iterations
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        blur(image, local_image, width, height);
    }

    // Write the blurred image to the output file
    writePGM(output_file, image, width, height, maxval);

    // Free dynamically allocated memory
    free(image);
    free(local_image);

    return 0;
}

// Function to apply the blur effect using a simple 3x3 kernel
void blur(int *image, int *local_image, int width, int height) {
    // Apply the blur effect in parallel using OpenMP
    #pragma omp parallel for collapse(2)
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum = 0;

            // Sum the values of the surrounding pixels (3x3 kernel)
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    sum += image[(y + ky) * width + (x + kx)];
                }
            }

            // Store the average (blurred) value in the local image
            local_image[y * width + x] = sum / 9;
        }
    }

    // After blurring, copy the result back to the original image
    #pragma omp parallel for
    for (int i = 0; i < height * width; i++) {
        image[i] = local_image[i];
    }
}

// Function to read a PGM (Portable Gray Map) image from file
void readPGM(const char *filename, int **image, int *width, int *height, int *maxval) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file %s\n", filename);
        exit(1);  // Exit if the file cannot be opened
    }

    // Read the PGM header information
    char format[3];
    fscanf(file, "%s", format);      // Read the PGM format (should be "P2")
    fscanf(file, "%d %d", width, height);  // Read the image dimensions
    fscanf(file, "%d", maxval);      // Read the maximum pixel value

    // Allocate memory for the image data
    *image = (int *)malloc((*width) * (*height) * sizeof(int));
    if (*image == NULL) {
        printf("Error: Unable to allocate memory for the image.\n");
        exit(1);  // Exit if memory allocation fails
    }

    // Read the pixel data
    for (int i = 0; i < (*width) * (*height); i++) {
        fscanf(file, "%d", &(*image)[i]);
    }

    fclose(file);  // Close the file after reading
}

// Function to write a PGM image to file
void writePGM(const char *filename, int *image, int width, int height, int maxval) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error opening file %s\n", filename);
        exit(1);  // Exit if the file cannot be opened
    }

    // Write the PGM header
    fprintf(file, "P2\n");
    fprintf(file, "%d %d\n", width, height);
    fprintf(file, "%d\n", maxval);

    // Write the pixel data
    for (int i = 0; i < width * height; i++) {
        fprintf(file, "%d ", image[i]);
    }

    fclose(file);  // Close the file after writing
}
