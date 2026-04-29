#include <math.h>

/*
 * We use NO_INLINE for all profiled functions for these reasons:
 * 1) Easier to inspect start/end of functions in disassembly code
 * 2) More correct performance measurements
 */
#define NO_INLINE  __attribute__ ((noinline))


// sobel params
#define SED_KERNEL_SIZE 3

// gaussian blur params
#define SIGMA 1
#define GBLUR_KERNEL_SIZE 3 //(int)(2 * round(SIGMA) + 1)
