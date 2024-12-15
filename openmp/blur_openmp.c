#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define MAX_ITERATIONS 20  // Maximum number of iterations

void readPGM(const char *filename, int **image, int *width, int *height, int *maxval);
void writePGM(const char *filename, int *image, int width, int height, int maxval);

void blur(int *image, int *local_image, int width, int height);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input.pgm> <output.pgm>\n", argv[0]);
        return 1;
    }

    char *input_file = argv[1];
    char *output_file = argv[2];

    int *image = NULL;
    int *local_image = NULL;
    int width, height, maxval;

    // Read the image
    readPGM(input_file, &image, &width, &height, &maxval);

    // Allocate memory for the local image (for temporary storage during blurring)
    local_image = (int *)malloc(width * height * sizeof(int));
    if (local_image == NULL) {
        printf("Error: Unable to allocate memory for the image.\n");
        return 1;
    }

    // Repeat the blur effect MAX_ITERATIONS times
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        blur(image, local_image, width, height);
    }

    // Write the output image
    writePGM(output_file, image, width, height, maxval);

    // Free allocated memory
    free(image);
    free(local_image);

    return 0;
}

void blur(int *image, int *local_image, int width, int height) {
    // Apply the blur effect using a simple 3x3 kernel
    #pragma omp parallel for
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    sum += image[(y + ky) * width + (x + kx)];
                }
            }
            local_image[y * width + x] = sum / 9; // Apply blur (average of neighbors)
        }
    }

    // After applying the blur, copy the result back to the original image
    #pragma omp parallel for
    for (int i = 0; i < height * width; i++) {
        image[i] = local_image[i];
    }
}

void readPGM(const char *filename, int **image, int *width, int *height, int *maxval) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file %s\n", filename);
        exit(1);
    }

    char format[3];
    fscanf(file, "%s", format);
    fscanf(file, "%d %d", width, height);
    fscanf(file, "%d", maxval);

    *image = (int *)malloc((*width) * (*height) * sizeof(int));
    for (int i = 0; i < (*width) * (*height); i++) {
        fscanf(file, "%d", &(*image)[i]);
    }

    fclose(file);
}

void writePGM(const char *filename, int *image, int width, int height, int maxval) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error opening file %s\n", filename);
        exit(1);
    }

    fprintf(file, "P2\n");
    fprintf(file, "%d %d\n", width, height);
    fprintf(file, "%d\n", maxval);

    for (int i = 0; i < width * height; i++) {
        fprintf(file, "%d ", image[i]);
    }

    fclose(file);
}
