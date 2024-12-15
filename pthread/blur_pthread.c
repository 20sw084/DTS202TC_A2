#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define MAX_LEN  255

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

static const float GAUSSIAN[3][3] = {
    {1.f / 16, 2.f / 16, 1.f / 16},
    {2.f / 16, 4.f / 16, 2.f / 16},
    {1.f / 16, 2.f / 16, 1.f / 16}
};

pthread_barrier_t barrier;
int NUM_FILTERS = 1;

struct image {
    uint8_t **image;
    uint8_t type, channels, maxval;
    int width;
    int height;
    int id;
    int num_tasks;
};

/**
 * Verifică dacă fișierul s-a deschis cu succes sau nu.
 *
 * @param file = descriptorul de fișier
 */
void check_file(FILE *file) {
    if (!file) {
        fprintf(stderr, "Eroare la deschiderea fișierelor!\n");
        exit(-1);
    }
}

/**
 * Verifică dacă s-a putut aloca memorie pentru structura de date sau nu.
 *
 * @param ptr  = pointerul către container
 * @param file = fișierul ce trebuie înschis înainte de închiderea programului
 */
void check_container(void *ptr, FILE *file) {
    if (!ptr) {
        fprintf(stderr, "Eroare la alocarea memoriei!\n");
        if (file) {
            fclose(file);
        }
        exit(-1);
    }
}

/**
 * Citirea imaginii din fișier.
 */
void read_image(char *image_in,
                uint8_t *type, uint8_t *channels,
                int *width, int *height,
                uint8_t *maxval, uint8_t ***image) {
    FILE *in;
    char comment_line[MAX_LEN];
    char c;
    int i;

    in = fopen(image_in, "rb");
    check_file(in);

    fscanf(in, "%c %hhu", &c, type);
    fgets(comment_line, MAX_LEN, in);
    fgets(comment_line, MAX_LEN, in);
    fscanf(in, "%d %d %hhu", width, height, maxval);
    fgets(comment_line, MAX_LEN, in);

    if ((*type) == 5) {
        *channels = 1;
    } else {
        *channels = 3;
    }

    *image = (uint8_t **) malloc(((*height) + 2) * sizeof(uint8_t *));
    check_container(*image, in);

    for (i = 0; i < (*height) + 2; ++i) {
        (*image)[i] =
            (uint8_t *) calloc((*channels) * ((*width) + 2), sizeof(uint8_t));
        check_container((*image)[i], in);
    }

    for (i = 1; i < (*height) + 1; ++i) {
        fread((*image)[i] + (*channels), sizeof(uint8_t),
              (*width) * (*channels), in);
    }

    fclose(in);
}

/**
 * Scrierea imaginii în fișier.
 */
void print_image(char *image_out,
                uint8_t *type, uint8_t *channels,
                int *width, int *height,
                uint8_t *maxval, uint8_t **image) {
    FILE *out;
    int i;

    out = fopen(image_out, "wb");
    check_file(out);

    fprintf(out, "P%u\n# Created by me\n", *type);
    fprintf(out, "%d %d\n", *width, *height);
    fprintf(out, "%d\n", *maxval);

    for (i = 1; i < (*height) + 1; ++i) {
        fwrite(image[i] + (*channels), sizeof(uint8_t),
               (*width) * (*channels), out);
    }

    fclose(out);
}

/**
 * Aplică filtrul pe o imagine / bucată de imagine.
 */
void apply_filter(int height, int width, uint8_t channels, uint8_t maxval,
                  uint8_t **image) {
    int i, j;
    float pixel;
    uint8_t **aux;

    aux = (uint8_t **) malloc((height + 2) * sizeof(uint8_t *));
    check_container(aux, NULL);

    for (i = 0; i < height + 2; ++i) {
        aux[i] = (uint8_t *) malloc(channels * (width + 2) * sizeof(uint8_t));
        check_container(aux[i], NULL);
        memcpy(aux[i], image[i], channels * (width + 2) * sizeof(uint8_t));
    }

    // Aplicarea filtrului propriu-zis (a convoluției)
    for (i = 1; i < height + 1; ++i) {
        for (j = channels; j < channels * (width + 1); ++j) {
            pixel = GAUSSIAN[2][2] * aux[i - 1][j - channels] +
                    GAUSSIAN[2][1] * aux[i - 1][j]            +
                    GAUSSIAN[2][0] * aux[i - 1][j + channels] +
                    GAUSSIAN[1][2] * aux[i][j - channels]     +
                    GAUSSIAN[1][1] * aux[i][j]                +
                    GAUSSIAN[1][0] * aux[i][j + channels]     +
                    GAUSSIAN[0][2] * aux[i + 1][j - channels] +
                    GAUSSIAN[0][1] * aux[i + 1][j]            +
                    GAUSSIAN[0][0] * aux[i + 1][j + channels];
            pixel = MIN(maxval, pixel);
            pixel = MAX(0, pixel);
            image[i][j] = pixel;
        }
    }

    for (i = 0; i < height + 2; ++i) {
        free(aux[i]);
    }
    free(aux);
}

/**
 * Funcția care va fi rulată de fiecare thread.
 */
void *thread_task(void *arg)
{
    struct image img = *(struct image*) arg;
    int start, end;

    start = img.id * ceil((double) img.height / img.num_tasks);
    end = MIN(img.height,
              (img.id + 1) * ceil((double) img.height / img.num_tasks));

    if (img.id > 0) {
        start--;
    }

    end++;

    if (img.id == img.num_tasks - 1) {
        end++;
    }

    img.height = end - start;

    for (int t = 0; t < NUM_FILTERS; t++) {
        // make sure that all threads had finished the job for the previous
        // filter before going to the next one
        pthread_barrier_wait(&barrier);
        apply_filter(img.height, img.width, img.channels, img.maxval,
                              img.image);
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

    if (argc < 5) {
        fprintf(stderr, "Eroare!\nMod utilizare: ./blur <in.format> "
                        "<out.format> <num_applied> <num_threads>\n");
        return 0;
    } else {
        image_in = argv[1];
        image_out = argv[2];
        NUM_FILTERS = atoi(argv[3]);
        num_threads = atoi(argv[4]);

        if (NUM_FILTERS <= 0) {
            fprintf(stderr, "Eroare!\n<num_applied> trebuie sa fie strict "
                            "pozitiv\n");
            return 0;
        }

        if (num_threads <= 0) {
            fprintf(stderr, "Eroare!\n<num_threads> trebuie sa fie strict "
                            "pozitiv\n");
            return 0;
        }
    }

    // Citirea imaginii de intrare din fișier.
    read_image(image_in, &type, &channels, &width, &height,
                &maxval, &image);

    pthread_t threads[num_threads];
    struct image img[num_threads];
    int r;
    int id;

    pthread_barrier_init(&barrier, NULL, num_threads);

    for (id = 0; id < num_threads; id++) {
        img[id].image = image;
        img[id].channels = channels;
        img[id].height = height;
        img[id].width = width;
        img[id].id = id;
        img[id].maxval = maxval;
        img[id].type = type;
        img[id].num_tasks = num_threads;

        r = pthread_create(&threads[id], NULL, thread_task, (void *) &img[id]);

        if (r) {
            fprintf(stderr, "Eroare la crearea thread-ului\n");
            return -1;
        }
    }

    // Așteptăm ca toate thread-urile să termine execuția.
    for (id = 0; id < num_threads; id++) {
        pthread_join(threads[id], NULL);
    }

    // Scrierea imaginii de ieșire în fișier.
    print_image(image_out, &type, &channels, &width, &height,
                &maxval, image);

    // Eliberarea memoriei alocate.
    for (id = 0; id < height + 2; id++) {
        free(image[id]);
    }
    free(image);

    pthread_barrier_destroy(&barrier);

    return 0;
}
