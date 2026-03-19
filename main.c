#include <math.h>
#include <stdio.h>


// gaussian blur params
#define SIGMA 1.5
#define GBLUR_KERNEL_SIZE (int)(2 * round(SIGMA) + 1)


// image params & values
#define HEIGHT 10
#define WIDTH 10

char in_image[HEIGHT][WIDTH] = {
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

char out_GB_image[HEIGHT][WIDTH] = {};


// sobel edge detection params & values
#define SED_KERNEL_SIZE 3
#define THRESHOLD 11 // TODO: adjust value, think how to adjust on fly

int G_x [SED_KERNEL_SIZE][SED_KERNEL_SIZE] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};
int G_y [SED_KERNEL_SIZE][SED_KERNEL_SIZE] = {
    {-1, -2, -1},
    { 0,  0,  0},
    { 1,  2,  1}
};

float out_SED_image[HEIGHT][WIDTH] = {};

void print_2D_float_array(const int height, const int width, float array[height][width]) {
    for (int i=0; i<height; i++) {
        printf("[");
        for (int j=0; j<width; j++) {
            printf("%.3f%s", array[i][j], j < width - 1 ? ", " : "");
        }
        printf("]\n");
    }
}

void print_2D_char_array(const int height, const int width, char array[height][width]) {
    for (int i=0; i<height; i++) {
        printf("[");
        for (int j=0; j<width; j++) {
            printf("%c%s", array[i][j], j < width - 1 ? ", " : "");
        }
        printf("]\n");
    }
}

void fill_gaussian_blur_kernel(int size, float kernel[size][size]) {
    const int r = GBLUR_KERNEL_SIZE / 2;  // radius
    float sum = 0.f;

    // calculating G(x, y) for every cell
    for (int i = 0; i < GBLUR_KERNEL_SIZE; i++) {
        for (int j = 0; j < GBLUR_KERNEL_SIZE; j++) {
            const int x = i - r;  // offset fom center
            const int y = j - r;
            kernel[i][j] = exp(-(x*x + y*y) / (2.0 * SIGMA * SIGMA))
                           / (2.0 * M_PI * SIGMA * SIGMA);
            sum += kernel[i][j];
        }
    }

    // normalization
    for (int i = 0; i < GBLUR_KERNEL_SIZE; i++)
        for (int j = 0; j < GBLUR_KERNEL_SIZE; j++)
            kernel[i][j] /= sum;
}

void gaussian_blur_naive (const int height, const int width,
                            char input[height][width], char output[height][width],
                            const int kernel_size, float kernel[kernel_size][kernel_size]) {
    const int radius = kernel_size / 2;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
            float cell_sum = 0.f;
            for (int ki = -radius; ki < radius + 1; ki++)
                for (int kj = -radius; kj < radius + 1; kj++) {
                    if (i + ki >= 0 && i + ki < height && j + kj >= 0 && j + kj < width)
                        cell_sum += input[i + ki][j + kj] * kernel[ki + radius][kj + radius];
                }
            output[i][j] = (char)cell_sum;
        }
}

int main() {
    printf("Hello and welcome to Gaussian Blur & Sobel Edge Detection!\n");
    printf("Gaussian Blur KERNEL_SIZE = %d\n", GBLUR_KERNEL_SIZE);

    //TODO: load image, preprocess to make monochrome

    printf("Gaussian Blur input image:\n");
    print_2D_char_array(HEIGHT, WIDTH, in_image);

    float kernel[GBLUR_KERNEL_SIZE][GBLUR_KERNEL_SIZE] = {};
    fill_gaussian_blur_kernel(GBLUR_KERNEL_SIZE, kernel);
    printf("Gaussian Blur kernel:\n");
    print_2D_float_array(GBLUR_KERNEL_SIZE, GBLUR_KERNEL_SIZE, kernel);

    gaussian_blur_naive(HEIGHT, WIDTH, in_image, out_GB_image,
        GBLUR_KERNEL_SIZE, kernel);

    printf("\n\nGaussian Blur output / Sobel Edge Detection input image:\n");
    print_2D_char_array(HEIGHT, WIDTH, out_GB_image);

    return 0;
}