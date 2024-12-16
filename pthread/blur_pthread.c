#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

// Maximum length of a comment line or buffer size
#define MAX_LEN 255

// Macro for finding the minimum of two values
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

// Macro for finding the maximum of two values
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

// Gaussian filter used for blurring
static const float GAUSSIAN[3][3] = {
    {1.f / 16, 2.f / 16, 1.f / 16},
    {2.f / 16, 4.f / 16, 2.f / 16},
    {1.f / 16, 2.f / 16, 1.f / 16}
};

// Synchronization barrier for threads
pthread_barrier_t barrier;

// Global variable for the number of filters to apply
int NUM_FILTERS = 1;

// Struct representing an image
struct image {
    uint8_t **image;  // 2D array of image pixels
    uint8_t type, channels, maxval; // Image type, channels, and max pixel value
    int width;  // Width of the image
    int height; // Height of the image
    int id;     // Thread ID (used for dividing work between threads)
    int num_tasks; // Number of tasks (threads) for parallel processing
};

/**
 * Check if the file has been opened successfully.
 *
 * @param file = file descriptor
 */
void check_file(FILE *file) {
    if (!file) {
        fprintf(stderr, "Error opening file!\n");
        exit(-1);
    }
}

/**
 * Check if memory allocation was successful for a given container.
 *
 * @param ptr  = pointer to the container
 * @param file = file to close before program termination (if any)
 */
void check_container(void *ptr, FILE *file) {
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed!\n");
        if (file) {
            fclose(file);
        }
        exit(-1);
    }
}

/**
 * Reads an image from a file.
 */
void read_image(char *image_in, uint8_t *type, uint8_t *channels,
                int *width, int *height, uint8_t *maxval, uint8_t ***image) {
    FILE *in;
    char comment_line[MAX_LEN];
    char c;
    int i;

    // Open image file
    in = fopen(image_in, "rb");
    check_file(in);

    // Read image type and comments
    fscanf(in, "%c %hhu", &c, type);
    fgets(comment_line, MAX_LEN, in);
    fgets(comment_line, MAX_LEN, in);

    // Read image dimensions and max pixel value
    fscanf(in, "%d %d %hhu", width, height, maxval);
    fgets(comment_line, MAX_LEN, in);

    // Set channels based on the image type (1 for grayscale, 3 for RGB)
    *channels = (*type == 5) ? 1 : 3;

    // Allocate memory for image buffer
    *image = (uint8_t **) malloc(((*height) + 2) * sizeof(uint8_t *));
    check_container(*image, in);

    // Allocate memory for each row of the image
    for (i = 0; i < (*height) + 2; ++i) {
        (*image)[i] = (uint8_t *) calloc((*channels) * ((*width) + 2), sizeof(uint8_t));
        check_container((*image)[i], in);
    }

    // Read pixel data from file
    for (i = 1; i < (*height) + 1; ++i) {
        fread((*image)[i] + (*channels), sizeof(uint8_t),
              (*width) * (*channels), in);
    }

    fclose(in);
}

/**
 * Writes an image to a file.
 */
void print_image(char *image_out, uint8_t *type, uint8_t *channels,
                 int *width, int *height, uint8_t *maxval, uint8_t **image) {
    FILE *out;
    int i;

    // Open output file
    out = fopen(image_out, "wb");
    check_file(out);

    // Write image header
    fprintf(out, "P%u\n# Created by me\n", *type);
    fprintf(out, "%d %d\n", *width, *height);
    fprintf(out, "%d\n", *maxval);

    // Write pixel data to file
    for (i = 1; i < (*height) + 1; ++i) {
        fwrite(image[i] + (*channels), sizeof(uint8_t),
               (*width) * (*channels), out);
    }

    fclose(out);
}

/**
 * Applies a filter (convolution) to the image.
 */
void apply_filter(int height, int width, uint8_t channels, uint8_t maxval,
                  uint8_t **image) {
    int i, j;
    float pixel;
    uint8_t **aux;

    // Allocate auxiliary memory for filtered image
    aux = (uint8_t **) malloc((height + 2) * sizeof(uint8_t *));
    check_container(aux, NULL);

    // Copy original image to auxiliary buffer (with padding)
    for (i = 0; i < height + 2; ++i) {
        aux[i] = (uint8_t *) malloc(channels * (width + 2) * sizeof(uint8_t));
        check_container(aux[i], NULL);
        memcpy(aux[i], image[i], channels * (width + 2) * sizeof(uint8_t));
    }

    // Apply Gaussian filter (convolution operation)
    for (i = 1; i < height + 1; ++i) {
        for (j = channels; j < channels * (width + 1); ++j) {
            pixel = GAUSSIAN[2][2] * aux[i - 1][j - channels] +
                    GAUSSIAN[2][1] * aux[i - 1][j] +
                    GAUSSIAN[2][0] * aux[i - 1][j + channels] +
                    GAUSSIAN[1][2] * aux[i][j - channels] +
                    GAUSSIAN[1][1] * aux[i][j] +
                    GAUSSIAN[1][0] * aux[i][j + channels] +
                    GAUSSIAN[0][2] * aux[i + 1][j - channels] +
                    GAUSSIAN[0][1] * aux[i + 1][j] +
                    GAUSSIAN[0][0] * aux[i + 1][j + channels];

            // Clamp pixel values to valid range
            pixel = MIN(maxval, pixel);
            pixel = MAX(0, pixel);

            // Update pixel in the image
            image[i][j] = pixel;
        }
    }

    // Free auxiliary memory
    for (i = 0; i < height + 2; ++i) {
        free(aux[i]);
    }
    free(aux);
}

/**
 * The function executed by each thread.
 */
void *thread_task(void *arg) {
    struct image img = *(struct image*) arg;
    int start, end;

    // Divide work between threads
    start = img.id * ceil((double) img.height / img.num_tasks);
    end = MIN(img.height, (img.id + 1) * ceil((double) img.height / img.num_tasks));

    if (img.id > 0) {
        start--;
    }

    end++;

    if (img.id == img.num_tasks - 1) {
        end++;
    }

    img.height = end - start;

    // Apply the filter NUM_FILTERS times
    for (int t = 0; t < NUM_FILTERS; t++) {
        // Synchronize threads at the barrier before moving to the next filter
        pthread_barrier_wait(&barrier);
        apply_filter(img.height, img.width, img.channels, img.maxval, img.image);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int width, height;
    uint8_t type, channels, maxval;
    uint8_t **image;
    char *image_in;
    char *image_out;
    int num_threads;

    // Check if arguments are provided correctly
    if (argc < 5) {
        fprintf(stderr, "Error!\nUsage: ./blur <input_file> <output_file> <num_filters> <num_threads>\n");
        return 0;
    } else {
        image_in = argv[1];
        image_out = argv[2];
        NUM_FILTERS = atoi(argv[3]);
        num_threads = atoi(argv[4]);

        // Validate filters and threads count
        if (NUM_FILTERS <= 0) {
            fprintf(stderr, "Error!\n<num_filters> must be a positive integer\n");
            return 0;
        }

        if (num_threads <= 0) {
            fprintf(stderr, "Error!\n<num_threads> must be a positive integer\n");
            return 0;
        }
    }

    // Read the input image
    read_image(image_in, &type, &channels, &width, &height, &maxval, &image);

    // Initialize thread barrier
    pthread_barrier_init(&barrier, NULL, num_threads);

    // Create threads to apply filters in parallel
    pthread_t threads[num_threads];
    struct image img_params = { image, type, channels, maxval, width, height, 0, num_threads };

    for (int i = 0; i < num_threads; i++) {
        img_params.id = i;
        pthread_create(&threads[i], NULL, thread_task, (void *) &img_params);
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Write the filtered image to output
    print_image(image_out, &type, &channels, &width, &height, &maxval, image);

    // Clean up and free resources
    for (int i = 0; i < height + 2; ++i) {
        free(image[i]);
    }
    free(image);
    pthread_barrier_destroy(&barrier);

    return 0;
}
