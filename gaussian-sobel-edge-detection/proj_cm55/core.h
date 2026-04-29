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
    const size_t size, 
    const uint8_t * restrict in_image, 
    uint8_t * out_image
) {
    uint32_t block_count = size / 8; 
    uint32_t tail_count = size % 8;

    // offsets for every 3rd byte
    static const uint16_t offsets[8] = {0, 3, 6, 9, 12, 15, 18, 21};
    const uint16x8_t v_offsets = vld1q_u16(offsets);

    while (block_count > 0) {
        // Gather load to form channels arrays for every pixel
        const uint16x8_t r = vldrbq_gather_offset_u16(in_image, v_offsets);
        const uint16x8_t g = vldrbq_gather_offset_u16(in_image + 1, v_offsets);
        const uint16x8_t b = vldrbq_gather_offset_u16(in_image + 2, v_offsets);

        // calculation
        uint16x8_t y = vmulq_n_u16(r, 77);
        y = vmlaq_n_u16(y, g, 151);
        y = vmlaq_n_u16(y, b, 28);

        // shifting
        y = vshrq_n_u16(y, 8);

        // converting to 8 bit
        vstrbq_u16(out_image, y);

        in_image += 8 * 3; 
        out_image += 8;
        block_count--;
    }

    if (tail_count > 0) {
        const mve_pred16_t p = vctp16q(tail_count);
        
        // Gather Loads with mask (zeroing)
        const uint16x8_t r = vldrbq_gather_offset_z_u16(in_image, v_offsets, p);
        const uint16x8_t g = vldrbq_gather_offset_z_u16(in_image + 1, v_offsets, p);
        const uint16x8_t b = vldrbq_gather_offset_z_u16(in_image + 2, v_offsets, p);
        
        uint16x8_t y = vmulq_n_u16(r, 77);
        y = vmlaq_n_u16(y, g, 151);
        y = vmlaq_n_u16(y, b, 28);

        y = vshrq_n_u16(y, 8);

        vstrbq_p_u16(out_image, y, p);
    }
}

CY_SECTION(".cy_itcm")
NO_INLINE void gaussian_blur(
    const size_t height, 
    const size_t width,
    uint8_t * restrict input, 
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
                const uint16x8_t v16 = vldrbq_u16(&input[i * width + j + kj]);
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
                    const uint8_t w = gaussian_kernel[kj + radius];
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
                    const uint8_t w = gaussian_kernel[kj + radius];
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
                const uint16x8_t v16 = vldrbq_u16(&buffer[(i + ki) * width + j]);
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
                    const uint8_t w = gaussian_kernel[ki + radius];
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
                    const uint8_t w = gaussian_kernel[ki + radius];
                    cell_sum  += buffer[(i + ki) * width + j] * w;
                    weight_sum += w;
                }
            }
            output[i * width + j] = (uint8_t)(cell_sum / weight_sum);
        }
    }
}


static const int16_t G_m1 [SED_KERNEL_SIZE] = {1, 2, 1};
static const int16_t G_m2 [SED_KERNEL_SIZE] = {-1, 0, 1};

// G_x = G_m1 * G_m2
// G_y = G_m2 * G_m1

CY_SECTION(".cy_itcm")
NO_INLINE int separable_sobel_kernel(
    const int height,
    const int width,
    uint8_t * restrict input,
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
        int j = radius;
        for (; j <= width - radius - 8; j += 8) {
            int16x8_t acc = vdupq_n_s16(0);
            for (int kj = -radius; kj <= radius; kj++) {
                int16x8_t v16 = vreinterpretq_s16_u16(vldrbq_u16(&input[i * width + j + kj]));
                acc = vmlaq_n_s16(acc, v16, (int16_t)hor_m[kj + radius]);
            }
            vstrhq_s16(&buffer[i * width + j], acc);
        }
        // center tail
        for (; j < width - radius; j++) {
            int16_t acc = 0;
            for (int kj = -radius; kj <= radius; kj++)
                acc += input[i * width + j + kj] * hor_m[kj + radius];
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
        int j = 0;
        for (; j <= width - 8; j += 8) {
            int16x8_t acc = vdupq_n_s16(0);
            for (int ki = -radius; ki <= radius; ki++) {
                int16x8_t v16 = vldrhq_s16(&buffer[(i + ki) * width + j]);
                acc = vmlaq_n_s16(acc, v16, (int16_t)ver_m[ki + radius]);
            }
            acc = vabsq_s16(acc);
            vstrhq_s16(&output[i * width + j], acc);
            sum += vaddvq_s16(acc);
        }
        for (; j < width; j++) {
            int16_t acc = 0;
            for (int ki = -radius; ki <= radius; ki++)
                acc += buffer[(i + ki) * width + j] * ver_m[ki + radius];
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
    const int height,
    const int width,
    uint8_t * restrict input,
    uint8_t output[height * width],
    int16_t G_x[height * width],
    int16_t G_y[height * width],
    int16_t buffer[height * width]
) {
    const int SUMX = separable_sobel_kernel(height, width, input, G_x, true, buffer);
    const int SUMY = separable_sobel_kernel(height, width, input, G_y, false, buffer);

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
