#include <math.h>
#include <stdio.h>

#define SIGMA 1.5
#define KERNEL_SIZE (int)(2 * round(SIGMA) + 1)

#define HEIGHT 10
#define WIDTH 10

float image[HEIGHT][WIDTH] = {
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100},
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100},
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100},
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100},
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100},
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100},
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100},
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100},
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100},
    {50, 50, 50, 50, 50, 100, 100, 100, 100, 100}
};

float output_image[HEIGHT][WIDTH] = {};

void print_2D_array(int height, int width, float array[height][width]) {
    for (int i=0; i<height; i++) {
        printf("[");
        for (int j=0; j<width; j++) {
            printf("%.3f%s", array[i][j], j < width - 1 ? ", " : "");
        }
        printf("]\n");
    }
}

void fill_kernel(int size, float kernel[size][size]) {
    const int r = KERNEL_SIZE / 2;  // radius
    float sum = 0.f;

    // calculating G(x, y) for every cell
    for (int i = 0; i < KERNEL_SIZE; i++) {
        for (int j = 0; j < KERNEL_SIZE; j++) {
            const int x = i - r;  // offset fom center
            const int y = j - r;
            kernel[i][j] = exp(-(x*x + y*y) / (2.0 * SIGMA * SIGMA))
                           / (2.0 * M_PI * SIGMA * SIGMA);
            sum += kernel[i][j];
        }
    }

    // normalization
    for (int i = 0; i < KERNEL_SIZE; i++)
        for (int j = 0; j < KERNEL_SIZE; j++)
            kernel[i][j] /= sum;
}

void gaussian_blur_naive (int height, int width,
    float input[height][width], float output[height][width],
    int kernel_size, float kernel[kernel_size][kernel_size]) {
    const int radius = kernel_size / 2;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
            float cell_sum = 0.f;
            for (int ki = -radius; ki < radius + 1; ki++)
                for (int kj = -radius; kj < radius + 1; kj++) {
                    if (i + ki >= 0 && i + ki < height && j + kj >= 0 && j + kj < width)
                        cell_sum += input[i + ki][j + kj] * kernel[ki + radius][kj + radius];
                }
            output[i][j] = cell_sum;
        }
}

int main() {
    printf("Hello and welcome to Gaussian Blur & Sobel Edge Detection!\n");
    printf("Gaussian Blur KERNEL_SIZE = %d\n", KERNEL_SIZE);

    //TODO: load image, preprocess to make monochrome

    printf("Gaussian Blur input image:\n");
    print_2D_array(HEIGHT, WIDTH, image);

    float kernel[KERNEL_SIZE][KERNEL_SIZE] = {};
    fill_kernel(KERNEL_SIZE, kernel);
    printf("Gaussian Blur kernel:\n");
    print_2D_array(KERNEL_SIZE, KERNEL_SIZE, kernel);

    gaussian_blur_naive(HEIGHT, WIDTH, image, output_image,
        KERNEL_SIZE, kernel);

    printf("\n\nGaussian Blur output / Sobel Edge Detection input image:\n");
    print_2D_array(HEIGHT, WIDTH, output_image);

    return 0;
}