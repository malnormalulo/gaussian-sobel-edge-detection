#include <math.h>

/*
 * We use NO_INLINE for all profiled functions for these reasons:
 * 1) Easier to inspect start/end of functions in disassembly code
 * 2) More correct performance measurements
 */
#define NO_INLINE  __attribute__ ((noinline))


// sobel params
#define SED_KERNEL_SIZE 3
#define THRESHOLD 25.f

// gaussian blur params
#define SIGMA 1
#define GBLUR_KERNEL_SIZE 3 //(int)(2 * round(SIGMA) + 1)

static float gaussian_kernel [GBLUR_KERNEL_SIZE][GBLUR_KERNEL_SIZE];

void fill_gaussian_blur_kernel() {
    const int r = GBLUR_KERNEL_SIZE / 2;  // radius
    float sum = 0.f;

    for (int i = 0; i < GBLUR_KERNEL_SIZE; i++) {
        for (int j = 0; j < GBLUR_KERNEL_SIZE; j++) {
            const int x = i - r;  // offset from center
            const int y = j - r;

            gaussian_kernel[i][j] = exp(-(x*x + y*y) / (2.0 * SIGMA * SIGMA))
                           / (2.0 * M_PI * SIGMA * SIGMA);
            sum += gaussian_kernel[i][j];
        }
    }

    // normalization
    for (int i = 0; i < GBLUR_KERNEL_SIZE; i++)
        for (int j = 0; j < GBLUR_KERNEL_SIZE; j++)
            gaussian_kernel[i][j] /= sum;
}
