#include "core_shared.h"

NO_INLINE void convert_to_monochrome(const size_t size, const uint8_t* in_image, uint8_t* out_image) {
    for (size_t i = 0; i < size; i++)
        out_image[i] = (uint8_t)(
            // 0.3f  * in_image[i*3] +
            // 0.59f * in_image[i*3 + 1] +
            // 0.11f * in_image[i*3 + 2]);
            (77 * in_image[i*3] +
            151 * in_image[i*3 + 1] +
            28  * in_image[i*3 + 2]) >> 8);
}

