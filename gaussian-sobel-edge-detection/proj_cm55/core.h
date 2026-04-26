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


static const int16_t G_m1 [SED_KERNEL_SIZE] = {1, 2, 1};
static const int16_t G_m2 [SED_KERNEL_SIZE] = {-1, 0, 1};

// G_x = G_m1 * G_m2
// G_y = G_m2 * G_m1

CY_SECTION(".cy_itcm")
NO_INLINE int separable_sobel_kernel(
    int height,
    int width,
    uint8_t input[height * width],
    int16_t output[height * width],
    bool isG_x,
    int16_t buffer[height * width]
) {
    const int radius = SED_KERNEL_SIZE / 2;

    const int16_t *hor_m = isG_x ? G_m1 : G_m2;
    const int16_t *ver_m = isG_x ? G_m2 : G_m1;

    int sum = 0;

    // Horizontal
    for (int i = 0; i < height; i++) {
        // center
        for (int j = radius; j < width - radius; j++) {
            int16_t acc = 0;
            for (int kj = -radius; kj <= radius; kj++) {
                acc += input[i * width + j + kj] * hor_m[kj + radius];
            }
            buffer[i * width + j] = acc;
        }

        // left edge
        for (int j = 0; j < radius; j++) {
            int16_t acc = 0;
            for (int kj = -radius; kj <= radius; kj++) {
                const int cj = j + kj < 0
                        ? 0
                        : j + kj >= width
                            ? width  - 1
                            : j + kj;
                acc += input[i * width + cj] * hor_m[kj + radius];
            }
            buffer[i * width + j] = acc;
        }
        // right edge
        for (int j = (width - radius); j < width; j++) {
            int16_t acc = 0;
            for (int kj = -radius; kj <= radius; kj++) {
                const int cj = j + kj < 0
                        ? 0
                        : j + kj >= width
                            ? width  - 1
                            : j + kj;
                acc += input[i * width + cj] * hor_m[kj + radius];
            }
            buffer[i * width + j] = acc;
        }
    }

    // Vertical
    // center
    for (int i = radius; i < height - radius; i++) {
        for (int j = 0; j < width; j++) {
            int16_t acc = 0;
            for (int ki = -radius; ki <= radius; ki++) {
                acc += buffer[(i + ki) * width + j] * ver_m[ki + radius];
            }
            acc = abs(acc);
            output[i * width + j] = acc;
            sum += acc;
        }
    }

    // top edge
    for (int i = 0; i < radius; i++) {
        for (int j = 0; j < width; j++) {
            int16_t acc = 0;
            for (int ki = -radius; ki <= radius; ki++) {
                const int ci = i + ki < 0
                        ? 0
                        : i + ki >= height
                            ? height - 1
                            : i + ki;
                acc += buffer[ci * width + j] * ver_m[ki + radius];
            }
            acc = abs(acc);
            output[i * width + j] = acc;
            sum += acc;
        }
    }
    // bottom edge
    for (int i = height - radius; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int16_t acc = 0;
            for (int ki = -radius; ki <= radius; ki++) {
                const int ci = i + ki < 0
                        ? 0
                        : i + ki >= height
                            ? height - 1
                            : i + ki;
                acc += buffer[ci * width + j] * ver_m[ki + radius];
            }
            acc = abs(acc);
            output[i * width + j] = acc;
            sum += acc;
        }
    }

    return sum;
}

CY_SECTION(".cy_itcm")
NO_INLINE void sobel_edge_detection(
    int height,
    int width,
    uint8_t input[height * width],
    uint8_t output[height * width],
    int16_t G_x[height * width],
    int16_t G_y[height * width],
    int16_t buffer[height * width]
) {
    const int SUMX = separable_sobel_kernel(height, width, input, G_x, true, buffer);
    const int SUMY = separable_sobel_kernel(height, width, input, G_y, false, buffer);

    const int threshold = (SUMX + SUMY) / (2 * height * width);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int16_t sumx = G_x[i * width + j];
            int16_t sumy = G_y[i * width + j];

            int16_t mag = sumx + sumy;
            output[i * width + j] = (uint8_t)(
                mag > 255
                    ? 255
                    : mag < threshold
                        ? 0
                        : mag
                );
        }
    }
}
