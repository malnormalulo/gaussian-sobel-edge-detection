#include "cybsp.h"
#include <arm_mve.h>

#include "retarget_io_init.h"
#include "perf_counter.h"

#include "input.h"
#include "out_monochrome.h"
#include "out_gaussian_blur.h"
#include "out_sobel_edge.h"

// #include "core_naive.h"
#include "core.h"


void print_summary(const char * fname,
    const uint8_t * actual,
    const uint8_t * expected,
    size_t size,
    float mac,
    perf_result_t metrics)
{
    int max_err = 0, n_within = 0, n_exact = 0;
    long sum_err = 0;

    for (size_t i = 0; i < size; i++) {
        const int d = (int)actual[i] - (int)expected[i*3];
        const int ad = d < 0 ? -d : d;
        if (ad > max_err) max_err = ad;
        sum_err += ad;
        if (ad == 0) n_exact++;
        if (ad <= 4) n_within++;
    }
    const float mean_err  = (float)sum_err  / (float)size;
    const float pct_exact = 100.f * (float)n_exact  / (float)size;
    const float pct_within = 100.f * (float)n_within / (float)size;

    printf("%s: cycles = %7lu,\t instr = %6lu,\t mac/cycle = %f,\t instr/mac = %f,"
           "\t IPC = %f,\t cycles/px = %f\n",
           fname,
           metrics.cycles,
           metrics.instructions,
           mac / metrics.cycles,
           metrics.instructions / mac,
           (float) metrics.instructions / metrics.cycles,
           (float) metrics.cycles / size);
    printf("%s accuracy vs reference: max_err = %d, mean_err = %.3f, "
           "exact = %.2f%%, within_4 = %.2f%%\n\n",
           fname, max_err, mean_err, pct_exact, pct_within);
}

void dump_buffer(const char *tag, const uint8_t *buf, size_t n) {
    printf("===BEGIN %s %u===\n", tag, (unsigned)n);
    for (size_t i = 0; i < n; i++) {
        printf("%02x", buf[i]);
        if ((i & 63u) == 63u) printf("\n");
    }
    if ((n & 63u) != 0u) printf("\n");
    printf("===END %s===\n", tag);
}


static uint8_t actual_out_monochrome[SIZE];
static uint8_t actual_out_gaussian_blur[SIZE];
static uint8_t actual_out_sobel[SIZE];

CY_SECTION(".cy_itcm")
int main(void)
{
    perf_result_t res;
    float mac;

    cy_rslt_t result = cybsp_init();
    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    init_retarget_io();
    perf_counter_init();
    // printf("Init finished\n");
    // printf("SystemCoreClock: %lu Hz (%.2f MHz)\n\n", SystemCoreClock, SystemCoreClock / 1000000.0);

    fill_gaussian_blur_kernel();
    // print_gaussian_kernel();

    mac = SIZE * 3;
    perf_counter_start();
    convert_to_monochrome(SIZE, input_image, actual_out_monochrome);
    res = perf_counter_stop();
    print_summary("\nmonochrome", actual_out_monochrome, out_monochrome, SIZE, mac, res);

    mac = GBLUR_KERNEL_SIZE*GBLUR_KERNEL_SIZE*SIZE;
    perf_counter_start();
    gaussian_blur(IN_HEIGHT, IN_WIDTH, actual_out_monochrome, actual_out_gaussian_blur);
    res = perf_counter_stop();
    print_summary("gaussian blur", actual_out_gaussian_blur, out_gaussian_blur, SIZE, mac, res);

    mac = SED_KERNEL_SIZE*SED_KERNEL_SIZE*SIZE;
    perf_counter_start();
    sobel_edge_detection(IN_HEIGHT, IN_WIDTH, actual_out_gaussian_blur, actual_out_sobel);
    res = perf_counter_stop();
    print_summary("sobel edge", actual_out_sobel, out_sobel_edge, SIZE, mac, res);

    dump_buffer("monochrome",    actual_out_monochrome,    SIZE);
    dump_buffer("gaussian_blur", actual_out_gaussian_blur, SIZE);
    dump_buffer("sobel_edge",    actual_out_sobel,         SIZE);

    // mac = SIZE*3 + GBLUR_KERNEL_SIZE*GBLUR_KERNEL_SIZE*SIZE + SED_KERNEL_SIZE*SED_KERNEL_SIZE*SIZE;

    // perf_counter_start();
    // convert_to_monochrome(SIZE, input_image, actual_out_monochrome);
    // gaussian_blur(IN_HEIGHT, IN_WIDTH, actual_out_monochrome, actual_out_gaussian_blur);
    // sobel_edge_detection(IN_HEIGHT, IN_WIDTH, actual_out_gaussian_blur, actual_out_sobel);
    // res = perf_counter_stop();

    // print_summary("full sobel edge pipeline", actual_out_sobel, out_sobel_edge, SIZE, mac, res);
    
    while(1) {
    }

    return 0;
}
