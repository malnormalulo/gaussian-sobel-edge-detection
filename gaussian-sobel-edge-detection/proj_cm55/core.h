#include "core_shared.h"

CY_SECTION(".cy_itcm")
NO_INLINE void convert_to_monochrome(
    size_t size, 
    const uint8_t* in_image, 
    uint8_t* out_image
) {
    for (size_t i = 0; i < size; i++)
        out_image[i] = (uint8_t)(
            (float)0.3  * in_image[i*3] +
            (float)0.59 * in_image[i*3 + 1] +
            (float)0.11 * in_image[i*3 + 2]);
}

CY_SECTION(".cy_itcm")
NO_INLINE void gaussian_blur(
    int height, 
    int width,
    uint8_t input[height * width], 
    uint8_t output[height * width]
) {
    const int radius = GBLUR_KERNEL_SIZE / 2;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
            float cell_sum = 0.f;
            float weight_sum = 0.f;
            for (int ki = -radius; ki < radius + 1; ki++)
                for (int kj = -radius; kj < radius + 1; kj++) {
                    if (i + ki >= 0 && i + ki < height && j + kj >= 0 && j + kj < width) {
                        float w = gaussian_kernel[ki + radius][kj + radius];
                        cell_sum   += (float)input[(i + ki) * width + (j + kj)] * w;
                        weight_sum += w;
                    }
                }
            output[i * width + j] = (uint8_t)(cell_sum / weight_sum);  // normalization
        }
}


static const int G_x [SED_KERNEL_SIZE][SED_KERNEL_SIZE] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};
static const int G_y [SED_KERNEL_SIZE][SED_KERNEL_SIZE] = {
    {-1, -2, -1},
    { 0,  0,  0},
    { 1,  2,  1}
};

CY_SECTION(".cy_itcm")
NO_INLINE void sobel_edge_detection(
    int height,
    int width,
    uint8_t input[height * width],
    uint8_t output[height * width]
) {
    const int radius = SED_KERNEL_SIZE / 2;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int sumx = 0;
            int sumy = 0;

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

                    const uint8_t px = input[ci * width + cj];
                    sumx += G_x[ki + radius][kj + radius] * px;
                    sumy += G_y[ki + radius][kj + radius] * px;
                }
            }

            int mag = abs(sumx) + abs(sumy);
            output[i * width + j] = (uint8_t)(
                mag > 255
                    ? 255
                    : mag < THRESHOLD
                        ? 0
                        : mag
                );
        }
    }
}
