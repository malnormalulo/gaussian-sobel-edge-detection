#include "core_shared.h"


CY_SECTION(".cy_dtcm")
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
NO_INLINE void convert_rgb565_to_mono_rgb888(
    const size_t size,
    const uint8_t *restrict in_image,
    uint8_t *out_image
) {
    for(int i = 0; i < size; i++) {
        uint16_t pixel = ((uint16_t)in_image[i*2+1] << 8) | in_image[i*2];
        uint16_t r, g, b;
        
        r = ((pixel & 0b1111100000000000) >> 11) * 255/31 * 77;
        g = ((pixel & 0b0000011111100000) >> 5) * 255/63 * 151;
        b = ( pixel & 0b0000000000011111) * 255/31 * 28;

        out_image[i] = (uint8_t)((r + g + b) >> 8);
    }
}

CY_SECTION(".cy_dtcm")
static uint8_t gauss_buffer[SIZE];

CY_SECTION(".cy_itcm")
NO_INLINE void gaussian_blur(
    const size_t height,
    const size_t width,
    const uint8_t * restrict input,
    uint8_t *output
) {
    const int radius = GBLUR_KERNEL_SIZE / 2;

    // Horizontal
    for (int i = 0; i < height; i++) {
        // center
        int j = radius;
        for (; j <= width - radius - 8; j += 8) {
            uint16x8_t acc = vdupq_n_u16(0);
            for (int kj = -radius; kj <= radius; kj++) {
                const uint16x8_t v16 = vldrbq_u16(&input[i * width + j + kj]);
                acc = vmlaq_n_u16(acc, v16, (uint16_t)gaussian_kernel[kj + radius]);
            }
            vstrbq_u16(&gauss_buffer[i * width + j], vshrq_n_u16(acc, 8));
        }
        // center tail
        for (; j < (width - radius); j++) {
            uint16_t cell_sum = 0;
            for (int kj = -radius; kj <= radius; kj++)
                cell_sum += input[i * width + j + kj] * gaussian_kernel[kj + radius];
            gauss_buffer[i * width + j] = (uint8_t)(cell_sum >> 8);
        }

        // left edge
        for (j = 0; j < radius; j++) {
            uint16_t cell_sum = 0, weight_sum = 0;
            for (int kj = -radius; kj <= radius; kj++) {
                if (j + kj >= 0) {
                    const uint8_t w = gaussian_kernel[kj + radius];
                    cell_sum  += input[i * width + j + kj] * w;
                    weight_sum += w;
                }
            }
            gauss_buffer[i * width + j] = (uint8_t)(cell_sum / weight_sum);
        }
        // right edge
        for (j = (width - radius); j < width; j++) {
            uint16_t cell_sum = 0, weight_sum = 0;
            for (int kj = -radius; kj <= radius; kj++) {
                if (j + kj < width) {
                    const uint8_t w = gaussian_kernel[kj + radius];
                    cell_sum  += input[i * width + j + kj] * w;
                    weight_sum += w;
                }
            }
            gauss_buffer[i * width + j] = (uint8_t)(cell_sum / weight_sum);
        }
    }

    // Vertical
    // center
    for (int i = radius; i < height - radius; i++) {
        int j = 0;
        for (; j <= width - 8; j += 8) {
            uint16x8_t acc = vdupq_n_u16(0);
            for (int ki = -radius; ki <= radius; ki++) {
                const uint16x8_t v16 = vldrbq_u16(&gauss_buffer[(i + ki) * width + j]);
                acc = vmlaq_n_u16(acc, v16, (uint16_t)gaussian_kernel[ki + radius]);
            }
            vstrbq_u16(&output[i * width + j], vshrq_n_u16(acc, 8));
        }
        // center tail
        for (; j < width; j++) {
            uint16_t cell_sum = 0;
            for (int ki = -radius; ki <= radius; ki++)
                cell_sum += gauss_buffer[(i + ki) * width + j] * gaussian_kernel[ki + radius];
            output[i * width + j] = (uint8_t)(cell_sum >> 8);
        }
    }

    // top edge
    for (int i = 0; i < radius; i++) {
        for (int j = 0; j < width; j++) {
            uint16_t cell_sum = 0, weight_sum = 0;
            for (int ki = -radius; ki <= radius; ki++) {
                if (i + ki >= 0) {
                    const uint8_t w = gaussian_kernel[ki + radius];
                    cell_sum  += gauss_buffer[(i + ki) * width + j] * w;
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
                    const uint8_t w = gaussian_kernel[ki + radius];
                    cell_sum  += gauss_buffer[(i + ki) * width + j] * w;
                    weight_sum += w;
                }
            }
            output[i * width + j] = (uint8_t)(cell_sum / weight_sum);
        }
    }
}



CY_SECTION(".cy_socmem_data")
static int16_t G_x[SIZE];
CY_SECTION(".cy_socmem_data")
static int16_t G_y[SIZE];

CY_SECTION(".cy_dtcm")
static int16_t sobel_buffer[SIZE];

static const int16_t G_m1 [SED_KERNEL_SIZE] = {1, 2, 1};
static const int16_t G_m2 [SED_KERNEL_SIZE] = {-1, 0, 1};

// G_x = G_m1 * G_m2
// G_y = G_m2 * G_m1

CY_SECTION(".cy_itcm")
NO_INLINE int separable_sobel_kernel(
    const int height,
    const int width,
    const uint8_t * restrict input,
    int16_t *output,
    bool isG_x
) {
    const int radius = SED_KERNEL_SIZE / 2;

    const int16_t *hor_m = isG_x ? G_m1 : G_m2;
    const int16_t *ver_m = isG_x ? G_m2 : G_m1;

    int sum = 0;

    // Horizontal
    for (int i = 0; i < height; i++) {
        // center
        int j = radius;
        for (; j <= width - radius - 8; j += 8) {
            int16x8_t acc = vdupq_n_s16(0);
            for (int kj = -radius; kj <= radius; kj++) {
                int16x8_t v16 = vreinterpretq_s16_u16(vldrbq_u16(&input[i * width + j + kj]));
                acc = vmlaq_n_s16(acc, v16, (int16_t)hor_m[kj + radius]);
            }
            vstrhq_s16(&sobel_buffer[i * width + j], acc);
        }
        // center tail
        for (; j < width - radius; j++) {
            int16_t acc = 0;
            for (int kj = -radius; kj <= radius; kj++)
                acc += input[i * width + j + kj] * hor_m[kj + radius];
            sobel_buffer[i * width + j] = acc;
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
            sobel_buffer[i * width + j] = acc;
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
            sobel_buffer[i * width + j] = acc;
        }
    }

    // Vertical
    // center
    for (int i = radius; i < height - radius; i++) {
        int j = 0;
        for (; j <= width - 8; j += 8) {
            int16x8_t acc = vdupq_n_s16(0);
            for (int ki = -radius; ki <= radius; ki++) {
                int16x8_t v16 = vldrhq_s16(&sobel_buffer[(i + ki) * width + j]);
                acc = vmlaq_n_s16(acc, v16, (int16_t)ver_m[ki + radius]);
            }
            acc = vabsq_s16(acc);
            vstrhq_s16(&output[i * width + j], acc);
            sum += vaddvq_s16(acc);
        }
        for (; j < width; j++) {
            int16_t acc = 0;
            for (int ki = -radius; ki <= radius; ki++)
                acc += sobel_buffer[(i + ki) * width + j] * ver_m[ki + radius];
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
                acc += sobel_buffer[ci * width + j] * ver_m[ki + radius];
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
                acc += sobel_buffer[ci * width + j] * ver_m[ki + radius];
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
    const int height,
    const int width,
    const uint8_t * restrict input,
    uint8_t *output
) {
    const int SUMX = separable_sobel_kernel(height, width, input, G_x, true);
    const int SUMY = separable_sobel_kernel(height, width, input, G_y, false);

    const int threshold = (SUMX + SUMY) / (2 * height * width);

    const int16x8_t v16x255 = vdupq_n_s16(255);
    const int16x8_t v16x0 = vdupq_n_s16(0);
    const int16x8_t v16xthreshold = vdupq_n_s16((int16_t)threshold);

    for (int i = 0; i < height; i++) {
        int j = 0;
        for (; j <= width - 8; j += 8) {
            int16x8_t mag = vaddq_s16(
                vldrhq_s16(&G_x[i * width + j]),
                vldrhq_s16(&G_y[i * width + j])
            );
            mag = vminq_s16(mag, v16x255);
            mag = vpselq_s16(mag, v16x0, vcmpgeq_s16(mag, v16xthreshold));
            vstrbq_u16(&output[i * width + j], vreinterpretq_u16_s16(mag));
        }
        for (; j < width; j++) {
            const int16_t mag = G_x[i * width + j] + G_y[i * width + j];
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


CY_SECTION(".cy_itcm")
NO_INLINE void mono_rgb888_to_rgb565(
    const int size,
    uint8_t *input,
    uint8_t *output
) {
    for(int i = 0; i < size; i++){
        uint16_t val =  ((input[i] & 0b11111000) << 8) |
                        ((input[i] & 0b11111100) << 3)  |
                        ((input[i] & 0b11111000) >> 3);
        output[i*2]   = val & 0xFF;
        output[i*2+1] = val >> 8;
    }
}
