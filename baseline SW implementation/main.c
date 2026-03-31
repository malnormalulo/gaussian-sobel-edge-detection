#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


// gaussian blur params
#define SIGMA 1
#define GBLUR_KERNEL_SIZE (int)(2 * round(SIGMA) + 1)


// sobel edge detection params & values
#define SED_KERNEL_SIZE 3
#define THRESHOLD 25.f // TODO: adjust value, think how to adjust on fly

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

typedef struct {
    uint8_t *data;   // RGB, size = width * height * 3
    int width;
    int height;
} ImageRGB;

ImageRGB load_rgb_image(const char *path) {
    ImageRGB img = {0};

    int channels = 0;
    uint8_t *pixels = stbi_load(path, &img.width, &img.height, &channels, 3);
    if (!pixels) {
        fprintf(stderr, "Failed to load image '%s': %s\n", path, stbi_failure_reason());
        return img;
    }

    img.data = pixels;
    return img;
}

void free_rgb_image(ImageRGB *img) {
    if (img->data) {
        stbi_image_free(img->data);
        img->data = NULL;
    }
}

uint8_t *alloc_gray_buffer(const ImageRGB *img) {
    return malloc(img->width * img->height * sizeof(uint8_t));
}

uint8_t *gray_to_rgb(const uint8_t *gray, int size) {
    uint8_t *rgb = malloc(size * 3 * sizeof(uint8_t));
    for (int i = 0; i < size; i++) {
        rgb[i * 3 + 0] = gray[i];
        rgb[i * 3 + 1] = gray[i];
        rgb[i * 3 + 2] = gray[i];
    }
    return rgb;
}

void save_gray_as_bmp(const char *path, const uint8_t *gray,
                      int width, int height) {
    const int size = width * height;
    uint8_t *rgb = gray_to_rgb(gray, size);
    stbi_write_bmp(path, width, height, 3, rgb);
    free(rgb);
}


// Core logic
void convert_to_monochrome_naive(const size_t size, uint8_t* in_image, uint8_t* out_image) {
    for (size_t i = 0; i < size; i++)
        out_image[i] = (uint8_t)(
            (float)0.3  * in_image[i*3] +
            (float)0.59 * in_image[i*3 + 1] +
            (float)0.11 * in_image[i*3 + 2]);
}

void fill_gaussian_blur_kernel_naive(int size, float kernel[size][size]) {
    const int r = size / 2;  // radius
    float sum = 0.f;

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
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

void sobel_edge_detection_naive(const int height, const int width,
                                  const uint8_t input[height * width],
                                  uint8_t output[height * width]) {
    const int radius = SED_KERNEL_SIZE / 2;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            float sumx = 0.f;
            float sumy = 0.f;

            for (int ki = -radius; ki <= radius; ki++) {
                for (int kj = -radius; kj <= radius; kj++) {
                    const int ci = i + ki < 0
                        ? 0
                        : i + ki >= height
                            ? height - 1
                            : i + ki;
                    const int cj = j + kj < 0
                        ? 0
                        : j + kj >= width
                            ? width  - 1
                            : j + kj;

                    const float px = input[ci * width + cj];
                    sumx += px * G_x[ki + radius][kj + radius];
                    sumy += px * G_y[ki + radius][kj + radius];
                }
            }

            float mag = sqrtf(sumx * sumx + sumy * sumy);
            output[i * width + j] = (uint8_t)(
                mag > 255.f
                    ? 255.f
                    : mag < THRESHOLD
                        ? 0.f
                        : mag
                );
        }
    }
}

int main() {
    printf("Welcome to Gaussian Blur & Sobel Edge Detection!\n");

    printf("\nLoading image...\n");
    ImageRGB src = load_rgb_image("input2.bmp");
    if (!src.data) {
        return -1;
    }
    printf("width: %d, height: %d\n", src.width, src.height);
    printf("Loading completed\n");

    const int size = src.width * src.height;

    printf("\nMonochrome image...\n");
    uint8_t *monochrome_image = alloc_gray_buffer(&src);
    convert_to_monochrome_naive(size, src.data, monochrome_image);
    free_rgb_image(&src);

    save_gray_as_bmp("out_monochrome.bmp", monochrome_image, src.width, src.height);
    printf("Monochrome completed\n");

    printf("\nGaussian Blur...\n");
    printf("Gaussian Blur KERNEL_SIZE = %d\n", GBLUR_KERNEL_SIZE);

    float kernel[GBLUR_KERNEL_SIZE][GBLUR_KERNEL_SIZE];
    fill_gaussian_blur_kernel_naive(GBLUR_KERNEL_SIZE, kernel);
    print_2D_float_array(GBLUR_KERNEL_SIZE, GBLUR_KERNEL_SIZE, kernel);

    uint8_t *gaussian_blur_image = malloc(size * sizeof(uint8_t));
    gaussian_blur_naive(src.height, src.width,
                        monochrome_image, gaussian_blur_image,
                        GBLUR_KERNEL_SIZE, kernel);
    free(monochrome_image);

    save_gray_as_bmp("out_gaussian_blur.bmp", gaussian_blur_image, src.width, src.height);
    printf("Gaussian Blur completed\n");

    printf("\nSobel Edge Detection\n");
    uint8_t *sobel_image = malloc(size * sizeof(uint8_t));
    sobel_edge_detection_naive(src.height, src.width,
                               gaussian_blur_image, sobel_image);
    free(gaussian_blur_image);

    save_gray_as_bmp("out_sobel_edge.bmp", sobel_image, src.width, src.height);
    free(sobel_image);
    printf("Sobel Edge Detection completed\n");

    return 0;
}

// https://abhijitnathwani.github.io/blog/2018/01/08/rgbtogray-Image-using-C
// https://github.com/abhijitnathwani/image-processing/blob/master/image_rgbtogray.c
// https://medium.com/@twinnroshan/understanding-and-implementing-edge-detection-in-c-with-sobel-operator-31159f26587c
// https://github.com/fzehracetin/sobel-edge-detection-in-c/tree/main
// https://homepages.inf.ed.ac.uk/rbf/HIPR2/sobel.htm
// https://www.youtube.com/watch?v=uihBwtPIBxM&t=6s
// https://www.youtube.com/watch?v=C_zFhWdM4ic&t=136s
// https://stackoverflow.com/questions/42618100/bmp-processing-in-c