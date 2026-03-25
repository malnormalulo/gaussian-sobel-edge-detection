#include "core_shared.h"

NO_INLINE void convert_to_monochrome(
    size_t size, 
    uint8_t* in_image, 
    uint8_t* out_image
) {
    for (size_t i = 0; i < size; i++)
        out_image[i] = (uint8_t)(
            // 0.3f  * in_image[i*3] +
            // 0.59f * in_image[i*3 + 1] +
            // 0.11f * in_image[i*3 + 2]);
            (77 * in_image[i*3] +
            151 * in_image[i*3 + 1] +
            28  * in_image[i*3 + 2]) >> 8);
}

void gaussian_blur(
    int height, 
    int width,
    uint8_t input[height * width], 
    uint8_t output[height * width],
    int kernel_size, 
    float kernel[kernel_size][kernel_size]
) {
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

void sobel_edge_detection(
    int height,
    int width,
    uint8_t input[height * width],
    uint8_t output[height * width]
) {
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
