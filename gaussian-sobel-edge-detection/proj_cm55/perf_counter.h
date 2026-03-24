#ifndef PERF_COUNTER_H
#define PERF_COUNTER_H

#include <stdint.h>
#include "cybsp.h"

typedef struct
{
    uint32_t cycles;
    uint32_t instructions;
} perf_result_t;

/*
 *  Init the perf counters
 */
static inline void perf_counter_init()
{
    // Enable DWT (Data Watchpoint and Trace)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    ARM_PMU_Enable();
    ARM_PMU_Set_EVTYPER(0, ARM_PMU_INST_RETIRED);
}

/*
 *  Start/reset the performance counters
 */
static inline void perf_counter_start()
{
    ARM_PMU_EVCNTR_ALL_Reset();
    ARM_PMU_CYCCNT_Reset();
    ARM_PMU_CNTR_Enable(PMU_CNTENSET_CCNTR_ENABLE_Msk | \
                        PMU_CNTENSET_CNT0_ENABLE_Msk);
}

/*
 *  Stop the performance counters and return their values
 */
static inline perf_result_t perf_counter_stop()
{
    ARM_PMU_CNTR_Disable(PMU_CNTENSET_CCNTR_ENABLE_Msk | \
                         PMU_CNTENSET_CNT0_ENABLE_Msk);
    perf_result_t res = {
        .cycles = ARM_PMU_Get_CCNTR(),
        .instructions = ARM_PMU_Get_EVCNTR(0),
    };

    // event counters are only 16-bit on the CM55
    // use the event counter overflow bit to extend the range from 16-bit to 17-bit
    const uint32_t ovf_mask = ARM_PMU_Get_CNTR_OVS();
    res.instructions |= ((ovf_mask & PMU_CNTENSET_CNT0_ENABLE_Msk) << (16 - PMU_CNTENSET_CNT0_ENABLE_Pos));

    // reset event counter overflow status
    ARM_PMU_Set_CNTR_OVS(PMU_CNTENSET_CNT0_ENABLE_Msk);

    return res;
}

#endif /* PERF_COUNTER_H */
