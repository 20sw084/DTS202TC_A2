#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>

#define TILE_WIDTH 16

// CUDA kernel to apply blur to an image
__global__ void blur_kernel(unsigned char* input, unsigned char* output, int width, int height) {
    int tx = threadIdx.x;
    int ty = threadIdx.y;
    int row = blockIdx.y * TILE_WIDTH + ty;
    int col = blockIdx.x * TILE_WIDTH + tx;
    
    int blur_radius = 1;  // For a simple 3x3 box blur

    if (row < height && col < width) {
        int pixel_value = 0;
        int count = 0;

        // Loop over a 3x3 neighborhood
        for (int i = -blur_radius; i <= blur_radius; i++) {
            for (int j = -blur_radius; j <= blur_radius; j++) {
                int r = row + i;
                int c = col + j;
                if (r >= 0 && r < height && c >= 0 && c < width) {
                    pixel_value += input[r * width + c];
                    count++;
                }
            }
        }
        
        // Store the average in the output
        output[row * width + col] = pixel_value / count;
    }
}

void check_cuda_error(cudaError_t err) {
    if (err != cudaSuccess) {
        printf("CUDA error: %s\n", cudaGetErrorString(err));
        exit(-1);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_image> <output_image> <num_threads>\n", argv[0]);
        return 1;
    }

    // For simplicity, we'll skip loading and saving actual image files
    // Let's assume we're dealing with a 2D grayscale image as a 1D array

    // Example image dimensions (replace with actual image loading in a real implementation)
    int width = 1024;  // Image width
    int height = 1024; // Image height

    // Allocate memory for the image data
    unsigned char *input, *output;
    unsigned char *d_input, *d_output;

    input = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    output = (unsigned char*)malloc(width * height * sizeof(unsigned char));

    // Initialize the input image with random data (in a real application, you'd load an image)
    for (int i = 0; i < width * height; i++) {
        input[i] = rand() % 256;
    }

    // Allocate GPU memory
    check_cuda_error(cudaMalloc((void**)&d_input, width * height * sizeof(unsigned char)));
    check_cuda_error(cudaMalloc((void**)&d_output, width * height * sizeof(unsigned char)));

    // Copy input data to device memory
    check_cuda_error(cudaMemcpy(d_input, input, width * height * sizeof(unsigned char), cudaMemcpyHostToDevice));

    // Define grid and block dimensions
    dim3 dimBlock(TILE_WIDTH, TILE_WIDTH);
    dim3 dimGrid((width + TILE_WIDTH - 1) / TILE_WIDTH, (height + TILE_WIDTH - 1) / TILE_WIDTH);

    // Launch the kernel
    blur_kernel<<<dimGrid, dimBlock>>>(d_input, d_output, width, height);

    // Check for errors during kernel execution
    check_cuda_error(cudaGetLastError());

    // Copy the result back to host memory
    check_cuda_error(cudaMemcpy(output, d_output, width * height * sizeof(unsigned char), cudaMemcpyDeviceToHost));

    // Free device memory
    cudaFree(d_input);
    cudaFree(d_output);

    // Here, you would save the output image
    // For now, let's just print the first pixel of the output image
    printf("First pixel value after blur: %d\n", output[0]);

    // Free host memory
    free(input);
    free(output);

    return 0;
}
