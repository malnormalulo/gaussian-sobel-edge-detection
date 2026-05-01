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
    char check_output[256];
    int error = 0;

    for (size_t i = 0; i < size; i++) {
        if ((int)actual[i] > (int)expected[i*3] + 4 || (int)actual[i] < (int)expected[i*3] - 4)
            error++;
    }
    if (error > 0)
        snprintf(check_output, sizeof(check_output), "NOK, found %d erros", error);
    else
        snprintf(check_output, sizeof(check_output), "OK");


    printf("%s: cycles = %7lu,\t instr = %6lu,\t mac/cycle = %f,\t instr/mac = %f,"
           "\t IPC = %f,\t cycles/px = %f,\t output = %s\n\n",
           fname,
           metrics.cycles,
           metrics.instructions,
           mac / metrics.cycles,
           metrics.instructions / mac,
           (float) metrics.instructions / metrics.cycles,
           (float) metrics.cycles / size,
           check_output);
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
