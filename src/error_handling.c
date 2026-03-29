/*
 * ===========================================================================
 * File: error_handling.c
 * Description: Fatal error handling (trap and signal via LED5).
 * ===========================================================================
 */

#include "master.h"

/** Busy-wait loop used while blinking the fatal error LED. */
static void error_delay(void)
{ // Required because HAL (along with all other interrupts) is Disabled when Error_Handler is called
    for (volatile uint32_t i = 0; i < 500000U; ++i) {
        __NOP();
    }
}

// TODO: Errors shoould ot be handled liike this
// Currnetly, we have two different error handling implmentaions:
// 1. The Error_Handler() function in this file, which is called by HAL functions when they encounter an error. This function disables interrupts and blinks LED5 indefinitely to indicate a fatal error.
// 2. The LOG_ERROR() macro, which is used to log error messages to the UART console. This macro does not currently trigger the Error_Handler() function or any other error handling mechanism.
// We should unify these two approaches so that all errors are handled consistently. 

/* Fatal error handler that blinks LED5 forever */
void Error_Handler(void)
{
    __disable_irq();
    LED_Init();
    LED_On(LED_ID_LD1);
    // Write to UART Logging system here
    LOG_ERROR("Error occurred!");

    while (1) {
        error_delay();
    }
}
