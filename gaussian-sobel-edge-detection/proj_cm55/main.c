#include "cybsp.h"
#include <arm_mve.h>
// #include "arm_math.h"

#include "retarget_io_init.h"
#include "perf_counter.h"
//#include "data_odd.h"
#include "data.h"

/*
 * We use NO_INLINE for all profiled functions for these reasons:
 * 1) Easier to inspect start/end of functions in disassembly code
 * 2) More correct performance measurements
 */
#define NO_INLINE  __attribute__ ((noinline))

void print_summary(const char* fname, const int32_t* actual, const int32_t* expected, size_t rows, size_t cols, float mac, perf_result_t metrics)
{
    char check_output[256];
    int error = 0;

    for (int i = 0; i < (rows*cols); i++) {
        if (actual[i] != expected[i])
            error++;
    }
    if (error > 0)
        snprintf(check_output, sizeof(check_output), "NOK, found %d erros", error);
    else
        snprintf(check_output, sizeof(check_output), "OK");


    printf("%s: cycles = %7ld,\t instr = %6ld,\t mac/cycle = %.3f,\t instr/mac = %.3f,\t output = %s\n",
           fname, metrics.cycles, metrics.instructions, mac/metrics.cycles, metrics.instructions/mac, check_output);
}

/*
 * Transpose a matrix
 *
 * Example: if mat_in = | b11 b12 b13 b14 |
 *                      | b21 b22 b23 b24 |
 *
 * or in memory: b11, b12, b13, b14, b21, b22, b23, b24
 *
 * then mat_out = | b11 b21 |
 *                | b12 b22 |
 *                | b13 b23 |
 *                | b14 b24 |
 *
 * or in memory: b11, b21, b12, b22, b13, b23, b14, b24
 */
void mat_transpose_int8(const int8_t* mat_in, int8_t* mat_out, size_t rows, size_t cols)
{
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            mat_out[j*rows + i] = mat_in[i*cols + j];
        }
    }
}

/*
 *  Matrix multiplication
 *
 *  mat_a x mat_b = mat_c
 *
 *  mat_a dims are: a_rows * a_cols_b_rows
 *  mat_b dims are: a_cols_b_rows * b_cols
 *  mat_c dims are: a_rows * b_cols
 *
 *  example:
 *
 *  | a0 a1 |   | b0 b1 b2 b3 |   | c0 c1 c2 c3 |
 *  | a2 a3 | x | b4 b5 b6 b7 | = | c4 c5 c6 c7 |
 *  | a4 a5 |                     | c8 c9 cA cB |
 *
 *  where:
 *   c0 = a0 * b0 + a1 * b4
 *   c1 = a0 * b1 + a1 * b5
 *   ...
 */
CY_SECTION(".cy_itcm") NO_INLINE void matmul_int8(const int8_t* mat_a, const int8_t* mat_b, int32_t* mat_c, size_t a_rows, size_t a_cols_b_rows, size_t b_cols)
{
    for (int i = 0; i < a_rows; i++) {
        for (int j = 0; j < b_cols; j++) {
            int32_t acc = 0;
            for (int k = 0; k < a_cols_b_rows; k++) {
                acc += mat_a[i*a_cols_b_rows + k] * mat_b[k*b_cols + j];
            }
            *mat_c++ = acc;
        }
    }
}

// CY_SECTION(".cy_itcm") NO_INLINE void matmul_int8_vect(const int8_t* mat_a, const int8_t* mat_b, int32_t* mat_c, size_t a_rows, size_t a_cols_b_rows, size_t b_cols)
// {
//     // for (int i = 0; i < a_rows; i++) {
//     //     for (int j = 0; j < b_cols; j++) {
//     //         int32_t acc = 0;
//     //         for (int k = 0; k < a_cols_b_rows; k++) {
//     //             acc += mat_a[i*a_cols_b_rows + k] * mat_b[k*b_cols + j];
//     //         }
//     //         *mat_c++ = acc;
//     //     }
//     // }
// }

/*
 *  Matrix multiplication with loop unrolling
 *
 *  mat_a x mat_b = mat_c
 *
 *  mat_a  dims are: a_rows * a_cols_b_rows
 *  mat_b  dims are: a_cols_b_rows * b_cols
 *  mat_c  dims are: a_rows * b_cols
 */
CY_SECTION(".cy_itcm") NO_INLINE void matmul_unrolled_int8(const int8_t* mat_a, const int8_t* mat_b, int32_t* mat_c, size_t a_rows, size_t a_cols_b_rows, size_t b_cols)
{
    const int b_cols_mult_4 = b_cols & ~(0x03);

    for (int i = 0; i < a_rows; i++) {
        for (int j = 0; j < b_cols_mult_4; j+=4) {
            int32_t acc0 = 0;
            int32_t acc1 = 0;
            int32_t acc2 = 0;
            int32_t acc3 = 0;

            for (int k = 0; k < a_cols_b_rows; k++) {
                const int8_t val = mat_a[i*a_cols_b_rows + k];
                acc0 += val * mat_b[k*b_cols + j];
                acc1 += val * mat_b[k*b_cols + j + 1];
                acc2 += val * mat_b[k*b_cols + j + 2];
                acc3 += val * mat_b[k*b_cols + j + 3];
            }
            *mat_c++ = acc0;
            *mat_c++ = acc1;
            *mat_c++ = acc2;
            *mat_c++ = acc3;
        }

        for (int j=b_cols_mult_4; j < b_cols; j++) {
            int32_t acc = 0;

            for (int k = 0; k < a_cols_b_rows; k++)
                acc += mat_a[i*a_cols_b_rows + k] * mat_b[k*b_cols + j];

            *mat_c++ = acc;
        }
    }
}


static int8_t mat_a[] = DATA_A;
static int8_t mat_b[] = DATA_B;
static int8_t mat_bt[DATA_B_ROWS*DATA_B_COLS];       // result for transposed mat_b
static int32_t mat_c_expected[] = DATA_C;            // expected result
static int32_t mat_c[DATA_C_ROWS*DATA_C_COLS];       // result is stored here
static int8_t mat_c_int8[DATA_C_ROWS*DATA_C_COLS];   // 8-bit result is stored here

CY_SECTION(".cy_itcm") int main(void)
{
    perf_result_t res;
    float mac = DATA_A_ROWS * DATA_A_COLS * DATA_B_COLS;

    cy_rslt_t result = cybsp_init();
    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    init_retarget_io();
    perf_counter_init();
    printf("Init finished\n");

    mat_transpose_int8(mat_b, mat_bt, DATA_B_ROWS, DATA_B_COLS);

    // Baseline matmul
    perf_counter_start();
    matmul_int8(mat_a, mat_b, mat_c, DATA_A_ROWS, DATA_A_COLS, DATA_B_COLS);
    res = perf_counter_stop();
    print_summary("matmul_int8                  ", mat_c, mat_c_expected, DATA_C_ROWS, DATA_C_COLS, mac, res);

    // Loop unolled matmul
    perf_counter_start();
    matmul_unrolled_int8(mat_a, mat_b, mat_c, DATA_A_ROWS, DATA_A_COLS, DATA_B_COLS);
    res = perf_counter_stop();
    print_summary("matmul_unrolled_int8         ", mat_c, mat_c_expected, DATA_C_ROWS, DATA_C_COLS, mac, res);

    while(1) {
    }

    return 0;
}
