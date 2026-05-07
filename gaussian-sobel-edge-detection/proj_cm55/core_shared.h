#include <arm_mve.h>
#include <math.h>

// Driver of the OV7675 camera (over DVP for stream and I2C for configuration)
#include "mtb_dvp_camera_ov7675.h"

/*
 * We use NO_INLINE for all profiled functions for these reasons:
 * 1) Easier to inspect start/end of functions in disassembly code
 * 2) More correct performance measurements
 */
#define NO_INLINE  __attribute__ ((noinline))

#define HEIGHT OV7675_FRAME_HEIGHT
#define WIDTH OV7675_FRAME_WIDTH
#define SIZE (HEIGHT * WIDTH)


// sobel params
#define SED_KERNEL_SIZE 3

// gaussian blur params
#define SIGMA 1
#define GBLUR_KERNEL_SIZE 3 //(int)(2 * round(SIGMA) + 1)


CY_SECTION(".cy_socmem_data")
static uint8_t out_monochrome[SIZE];
CY_SECTION(".cy_socmem_data")
static uint8_t out_gaussian_blur[SIZE];
CY_SECTION(".cy_socmem_data")
static uint8_t out_sobel[SIZE];
