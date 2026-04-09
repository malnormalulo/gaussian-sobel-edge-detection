#include "core_shared.h"

CY_SECTION(".cy_itcm")
NO_INLINE void convert_to_monochrome(
    size_t size, 
    const uint8_t* in_image, 
    uint8_t* out_image
) {
    uint32_t block_count = size / 8; 
    uint32_t tail_count = size % 8;

    // offsets for every 3rd byte
    static const uint16_t offsets[8] = {0, 3, 6, 9, 12, 15, 18, 21};
    uint16x8_t v_offsets = vld1q_u16(offsets);

    while (block_count > 0) {
        // Gather load to form channels arrays for every pixel
        uint16x8_t r = vldrbq_gather_offset_u16(in_image, v_offsets);
        uint16x8_t g = vldrbq_gather_offset_u16(in_image + 1, v_offsets);
        uint16x8_t b = vldrbq_gather_offset_u16(in_image + 2, v_offsets);

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
        mve_pred16_t p = vctp16q(tail_count);
        
        // Gather Loads with mask (zeroing)
        uint16x8_t r = vldrbq_gather_offset_z_u16(in_image, v_offsets, p);
        uint16x8_t g = vldrbq_gather_offset_z_u16(in_image + 1, v_offsets, p);
        uint16x8_t b = vldrbq_gather_offset_z_u16(in_image + 2, v_offsets, p);
        
        uint16x8_t y = vmulq_n_u16(r, 77);
        y = vmlaq_n_u16(y, g, 151);
        y = vmlaq_n_u16(y, b, 28);

        y = vshrq_n_u16(y, 8);

        vstrbq_p_u16(out_image, y, p);
    }
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
