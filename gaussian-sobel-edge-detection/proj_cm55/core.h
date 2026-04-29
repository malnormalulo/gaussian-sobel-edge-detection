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
