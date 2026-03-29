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

int main(void)
{
    System_AppInit();

    int bq_status;
    // bool bq_is_active = false;

     /* WAKE SEQUENCE */
    if((bq_status = BQ_Wake(BQ_STACK_COUNT)) != 0) {
        LOG_ERROR("BQ_Wake failed: bq_status=%d", bq_status);
        Error_Handler();
    }


    // Auto Addressing is Skipped because we interface with BQ79616 directly (using isolated UART Logic Level Shifter [3.3V - 5.0V]) instead of using BQ79600
    // Use Device's Default Address: 0x01
    
    // bq_is_active = BQ_ServiceTask();


    uint8_t partid = 0u;
    bq_status = bq79616_read_partid_once(&partid);
    if (bq_status != 0) {
        LOG_ERROR("Initial PARTID read failed: bq_status=%d", bq_status);
        Error_Handler();
    }

    LOG_INFO("Initial PARTID read success (0x%02X). Communication verified.", partid);
    LED_All_Pulse(1000u);

    
    while (true) {
        // if (bq_is_active) {
        //     bq_is_active = BQ_ServiceTask();
        // }
        
        // CAN_ServiceTask();
        // ADC_ServiceTask();
    }
}

