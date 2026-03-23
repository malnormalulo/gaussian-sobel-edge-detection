#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


// gaussian blur params
#define SIGMA 1.5
#define GBLUR_KERNEL_SIZE (int)(2 * round(SIGMA) + 1)


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

// Utills
void print_2D_float_array(const int height, const int width, float array[height][width]) {
    for (int i=0; i<height; i++) {
        printf("[");
        for (int j=0; j<width; j++) {
            printf("%.3f%s", array[i][j], j < width - 1 ? ", " : "");
        }
        printf("]\n");
    }
}

void print_uint8_t_array(const int height, const int width, const uint8_t * array) {
    for (int i = 0; i < height; i++) {
        printf("[");
        for (int j = 0; j < width; j++) {
            printf("%d%s", array[i * width + j], j < width - 1 ? ", " : "");
        }
        printf("]\n");
    }
}


// Core logic
void convert_to_monochrome_naive(const int size, uint8_t in_image[size][3], uint8_t out_image[size]) {
    for (int i = 0; i < size; i++)
        out_image[i] = (uint8_t)(0.3f * in_image[i][0] + 0.59f * in_image[i][1] + 0.11f * in_image[i][2]);
}

void fill_gaussian_blur_kernel_naive(int size, float kernel[size][size]) {
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

void gaussian_blur_naive(const int height, const int width,
                          const uint8_t input[height * width], uint8_t output[height * width],
                          const int kernel_size, float kernel[kernel_size][kernel_size]) {
    const int radius = kernel_size / 2;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
            float cell_sum = 0.f;
            float weight_sum = 0.f;
            for (int ki = -radius; ki < radius + 1; ki++)
                for (int kj = -radius; kj < radius + 1; kj++) {
                    if (i + ki >= 0 && i + ki < height && j + kj >= 0 && j + kj < width) {
                        float w = kernel[ki + radius][kj + radius];
                        cell_sum   += (float)input[(i + ki) * width + (j + kj)] * w;
                        weight_sum += w;
                    }
                }
            output[i * width + j] = (uint8_t)(cell_sum / weight_sum);  // normalization
        }
}



int main() {
    printf("Welcome to Gaussian Blur & Sobel Edge Detection!\n");
    printf("\nLoading image...\n");

    FILE *fIn = fopen("input.bmp","rb"); // Input file
    FILE *fOut = fopen("output.bmp","w+b"); // Output file

    if(fIn == NULL)
    {
        printf("File does not exist.\n");
        return -1;
    }

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, fIn);
    fwrite(header, sizeof(unsigned char), 54, fOut);

    // extract image height, width from imageHeader
    const int width = *(int*)&header[18];
    const int height = *(int*)&header[22];

    const int size = height * width;
    uint8_t (*original_image)[3] = malloc(size * 3 * sizeof(uint8_t));

    fread(original_image, sizeof(uint8_t), size * 3, fIn);

    fclose(fIn);
    printf("Loading completed\n");

    printf("width: %d\n", width);
    printf("height: %d\n", height);

    printf("\nMonochrome image...\n");

    uint8_t *monochrome_image = malloc(size * sizeof(uint8_t));
    convert_to_monochrome_naive(size, original_image, monochrome_image);

    free(original_image);

    // for (int i = 0; i < size; i++) {
    //     fputc(monochrome_image[i], fOut); // B
    //     fputc(monochrome_image[i], fOut); // G
    //     fputc(monochrome_image[i], fOut); // R
    // }
    // fclose(fOut);

    printf("Monochrome completed\n");
    // print_uint8_t_array(height, width, monochrome_image);


    printf("\nGaussian Blur...\n");
    printf("Gaussian Blur KERNEL_SIZE = %d\n", GBLUR_KERNEL_SIZE);

    float kernel[GBLUR_KERNEL_SIZE][GBLUR_KERNEL_SIZE] = {};
    fill_gaussian_blur_kernel_naive(GBLUR_KERNEL_SIZE, kernel);
    printf("Gaussian Blur kernel:\n");
    print_2D_float_array(GBLUR_KERNEL_SIZE, GBLUR_KERNEL_SIZE, kernel);

    uint8_t *gaussian_blur_image = malloc(size * sizeof(uint8_t));
    gaussian_blur_naive(height, width, monochrome_image, gaussian_blur_image,
        GBLUR_KERNEL_SIZE, kernel);
    free(monochrome_image);

    for (int i = 0; i < size; i++) {
        fputc(gaussian_blur_image[i], fOut); // B
        fputc(gaussian_blur_image[i], fOut); // G
        fputc(gaussian_blur_image[i], fOut); // R
    }

    fclose(fOut);

    printf("Gaussian Blur completed\n");


    printf("\nSobel Edge Detection\n");



    free(gaussian_blur_image); ////!!!!!!!!!!

    return 0;
}

// https://abhijitnathwani.github.io/blog/2018/01/08/rgbtogray-Image-using-C
// https://github.com/abhijitnathwani/image-processing/blob/master/image_rgbtogray.c