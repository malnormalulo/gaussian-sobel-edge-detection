#include "cy_pdl.h"
#include "cybsp.h"

#define CM33_NS_APP_BOOT_ADDR      (CYMEM_CM33_0_m33_nvm_START + CYBSP_MCUBOOT_HEADER_SIZE) 
int main(void)
{
    uint32_t ns_stack;
    cy_cmse_funcptr NonSecure_ResetHandler;
    cy_rslt_t result;

    /* Set up internal routing, pins, and clock-to-peripheral connections */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        /* Disable all interrupts. */
        __disable_irq();

        CY_ASSERT(0);

        /* Infinite loop */
        while(true);
    }

    /* Enable global interrupts */
    __enable_irq();

    ns_stack = (uint32_t)(*((uint32_t*)CM33_NS_APP_BOOT_ADDR));
    __TZ_set_MSP_NS(ns_stack);

    NonSecure_ResetHandler = (cy_cmse_funcptr)(*((uint32_t*)(CM33_NS_APP_BOOT_ADDR + 4)));

    /* Start non-secure application */
    NonSecure_ResetHandler();

    for (;;)
    {
    }
}
