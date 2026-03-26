/*
 * ===========================================================================
 * File: main.c
 * Description: Application entry point that dispatches ADC tasks.
 *
 * Notes:
 *   - Service tasks run every loop iteration; per-task timing lives in modules.
 * ===========================================================================
 */

#include "master.h"

bool bq_is_active;

int main(void)
{
    System_AppInit();

    (void)BQ_WakeAndInit(BQ_STACK_COUNT);
    LED_All_Pulse(100u);

    bq_is_active = BQ_ServiceTask();
    
    while (true) {
        if (bq_is_active) {
            bq_is_active = BQ_ServiceTask();
        }
        
        CAN_ServiceTask();
        ADC_ServiceTask();
    }
}

