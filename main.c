#include <math.h>
#include <stdio.h>

#define SIGMA 1.5
#define KERNEL_SIZE (int)(2 * round(SIGMA) + 1)

#define HEIGHT 10
#define WIDTH 10

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

int main() {
    printf("Hello and welcome to Gaussian Blur!\n");
    printf("KERNEL_SIZE = %d\n", KERNEL_SIZE);

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

    printf("Input image:\n");
    print_2D_array(HEIGHT, WIDTH, image);

    float kernel[KERNEL_SIZE][KERNEL_SIZE] = {};
    fill_kernel(KERNEL_SIZE, kernel);
    printf("Kernel:\n");
    print_2D_array(KERNEL_SIZE, KERNEL_SIZE, kernel);

    const int radius = KERNEL_SIZE / 2;
    for (int i = 0; i < HEIGHT; i++)
        for (int j = 0; j < WIDTH; j++) {
            float cell_sum = 0.f;
            for (int ki = -radius; ki < radius + 1; ki++)
                for (int kj = -radius; kj < radius + 1; kj++) {
                    if (i + ki >= 0 && i + ki < HEIGHT && j + kj >= 0 && j + kj < WIDTH)
                        cell_sum += image[i + ki][j + kj] * kernel[ki + radius][kj + radius];
                }
            output_image[i][j] = cell_sum;
        }

    printf("\n\nOutput image:\n");
    print_2D_array(HEIGHT, WIDTH, output_image);

    return 0;
}