#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>

#define TILE_WIDTH 16  // Define the width of the tile (block size) for the CUDA kernel

// CUDA kernel to apply blur to an image using a 3x3 box blur
__global__ void blur_kernel(unsigned char* input, unsigned char* output, int width, int height) {
    int tx = threadIdx.x;  // Thread's x-coordinate within the block
    int ty = threadIdx.y;  // Thread's y-coordinate within the block
    int row = blockIdx.y * TILE_WIDTH + ty;  // Row index in the image
    int col = blockIdx.x * TILE_WIDTH + tx;  // Column index in the image
    
    int blur_radius = 1;  // Radius for the 3x3 box blur

    // Ensure the thread is within the bounds of the image
    if (row < height && col < width) {
        int pixel_value = 0;
        int count = 0;

        // Loop over a 3x3 neighborhood around the current pixel
        for (int i = -blur_radius; i <= blur_radius; i++) {
            for (int j = -blur_radius; j <= blur_radius; j++) {
                int r = row + i;
                int c = col + j;
                if (r >= 0 && r < height && c >= 0 && c < width) {
                    pixel_value += input[r * width + c];  // Accumulate the pixel values
                    count++;  // Count the valid pixels in the neighborhood
                }
            }
        }
        
        // Store the average value (blurred pixel) in the output image
        output[row * width + col] = pixel_value / count;
    }
}

// Function to check for CUDA errors and handle them
void check_cuda_error(cudaError_t err) {
    if (err != cudaSuccess) {
        printf("CUDA error: %s\n", cudaGetErrorString(err));
        exit(-1);  // Exit if an error occurs
    }
}

int main(int argc, char* argv[]) {
    // Ensure correct usage of the program
    if (argc != 4) {
        printf("Usage: %s <input_image> <output_image> <num_threads>\n", argv[0]);
        return 1;
    }

    // Image dimensions (for demonstration purposes, real code should load actual images)
    int width = 1024;  // Image width
    int height = 1024; // Image height

    // Allocate memory for the image data
    unsigned char *input, *output;
    unsigned char *d_input, *d_output;

    // Allocate host memory for the input and output images
    input = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    output = (unsigned char*)malloc(width * height * sizeof(unsigned char));

    // Initialize the input image with random data (in practice, load an actual image)
    for (int i = 0; i < width * height; i++) {
        input[i] = rand() % 256;  // Assign random pixel values between 0 and 255
    }

    // Allocate GPU memory for the input and output images
    check_cuda_error(cudaMalloc((void**)&d_input, width * height * sizeof(unsigned char)));
    check_cuda_error(cudaMalloc((void**)&d_output, width * height * sizeof(unsigned char)));

    // Copy the input data from the host to the device (GPU)
    check_cuda_error(cudaMemcpy(d_input, input, width * height * sizeof(unsigned char), cudaMemcpyHostToDevice));

    // Define grid and block dimensions for CUDA kernel launch
    dim3 dimBlock(TILE_WIDTH, TILE_WIDTH);  // Each block is TILE_WIDTH x TILE_WIDTH threads
    dim3 dimGrid((width + TILE_WIDTH - 1) / TILE_WIDTH, (height + TILE_WIDTH - 1) / TILE_WIDTH);  // Calculate grid size

    // Launch the CUDA kernel to perform the blur operation
    blur_kernel<<<dimGrid, dimBlock>>>(d_input, d_output, width, height);

    // Check if any error occurred during kernel execution
    check_cuda_error(cudaGetLastError());

    // Copy the output data back from device to host memory
    check_cuda_error(cudaMemcpy(output, d_output, width * height * sizeof(unsigned char), cudaMemcpyDeviceToHost));

    // Free GPU memory
    cudaFree(d_input);
    cudaFree(d_output);

    // In a real application, you'd save the output image here
    // For demonstration, print the first pixel value after blurring
    printf("First pixel value after blur: %d\n", output[0]);

    // Free host memory
    free(input);
    free(output);

    return 0;
}
