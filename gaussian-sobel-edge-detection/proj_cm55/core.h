#include "core_shared.h"


static uint8_t gaussian_kernel[GBLUR_KERNEL_SIZE];

void fill_gaussian_blur_kernel() {
    const int r = GBLUR_KERNEL_SIZE / 2;
    float sum = 0.f;
    float kernel_f[GBLUR_KERNEL_SIZE];

    for (int i = 0; i < GBLUR_KERNEL_SIZE; i++) {
        const int x = i - r;
        kernel_f[i] = expf(-(float)(x * x) / (2.0f * SIGMA * SIGMA));
        sum += kernel_f[i];
    }
    for (int i = 0; i < GBLUR_KERNEL_SIZE; i++)
        gaussian_kernel[i] = (uint8_t)roundf(kernel_f[i] / sum * 256.f);
}

void print_gaussian_kernel() {
    printf("[");
    for (int i = 0; i < GBLUR_KERNEL_SIZE; i++)
        printf("%d%s", gaussian_kernel[i], i < GBLUR_KERNEL_SIZE - 1 ? ", " : "");
    printf("]\n");
}

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
    size_t height, 
    size_t width,
    uint8_t input[height * width], 
    uint8_t output[height * width],
    uint8_t buffer[height * width]
) {
    const int radius = GBLUR_KERNEL_SIZE / 2;

    // Horizontal
    for (int i = 0; i < height; i++) {
        // center
        int j = radius;
        for (; j <= width - radius - 8; j += 8) {
            uint16x8_t acc = vdupq_n_u16(0);
            for (int kj = -radius; kj <= radius; kj++) {
                uint16x8_t v16 = vldrbq_u16(&input[i * width + j + kj]);
                acc = vmlaq_n_u16(acc, v16, (uint16_t)gaussian_kernel[kj + radius]);
            }
            vstrbq_u16(&buffer[i * width + j], vshrq_n_u16(acc, 8));
        }
        // center tail
        for (; j < (width - radius); j++) {
            uint16_t cell_sum = 0;
            for (int kj = -radius; kj <= radius; kj++)
                cell_sum += input[i * width + j + kj] * gaussian_kernel[kj + radius];
            buffer[i * width + j] = (uint8_t)(cell_sum >> 8);
        }

        // left edge
        for (j = 0; j < radius; j++) {
            uint16_t cell_sum = 0, weight_sum = 0;
            for (int kj = -radius; kj <= radius; kj++) {
                if (j + kj >= 0) {
                    uint8_t w = gaussian_kernel[kj + radius];
                    cell_sum  += input[i * width + j + kj] * w;
                    weight_sum += w;
                }
            }
            buffer[i * width + j] = (uint8_t)(cell_sum / weight_sum);
        }
        // right edge
        for (j = (width - radius); j < width; j++) {
            uint16_t cell_sum = 0, weight_sum = 0;
            for (int kj = -radius; kj <= radius; kj++) {
                if (j + kj < width) {
                    uint8_t w = gaussian_kernel[kj + radius];
                    cell_sum  += input[i * width + j + kj] * w;
                    weight_sum += w;
                }
            }
            buffer[i * width + j] = (uint8_t)(cell_sum / weight_sum);
        }
    }

    // Vertical
    // center
    for (int i = radius; i < height - radius; i++) {
        int j = 0;
        for (; j <= width - 8; j += 8) {
            uint16x8_t acc = vdupq_n_u16(0);
            for (int ki = -radius; ki <= radius; ki++) {
                uint16x8_t v16 = vldrbq_u16(&buffer[(i + ki) * width + j]);
                acc = vmlaq_n_u16(acc, v16, (uint16_t)gaussian_kernel[ki + radius]);
            }
            vstrbq_u16(&output[i * width + j], vshrq_n_u16(acc, 8));
        }
        // center tail
        for (; j < width; j++) {
            uint16_t cell_sum = 0;
            for (int ki = -radius; ki <= radius; ki++)
                cell_sum += buffer[(i + ki) * width + j] * gaussian_kernel[ki + radius];
            output[i * width + j] = (uint8_t)(cell_sum >> 8);
        }
    }

    // top edge
    for (int i = 0; i < radius; i++) {
        for (int j = 0; j < width; j++) {
            uint16_t cell_sum = 0, weight_sum = 0;
            for (int ki = -radius; ki <= radius; ki++) {
                if (i + ki >= 0) {
                    uint8_t w = gaussian_kernel[ki + radius];
                    cell_sum  += buffer[(i + ki) * width + j] * w;
                    weight_sum += w;
                }
            }
            output[i * width + j] = (uint8_t)(cell_sum / weight_sum);
        }
    }
    // bottom edge
    for (int i = height - radius; i < height; i++) {
        for (int j = 0; j < width; j++) {
            uint16_t cell_sum = 0, weight_sum = 0;
            for (int ki = -radius; ki <= radius; ki++) {
                if (i + ki < height) {
                    uint8_t w = gaussian_kernel[ki + radius];
                    cell_sum  += buffer[(i + ki) * width + j] * w;
                    weight_sum += w;
                }
            }
            output[i * width + j] = (uint8_t)(cell_sum / weight_sum);
        }
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
