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
