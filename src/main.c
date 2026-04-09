/*
 * ===========================================================================
 * File: main.c
 * Description: Application entry point.
 *
 * Startup policy:
 *   - BQ bring-up is best-effort.
 *   - If BQ fails or later loses comms, disable only BQ handling.
 *   - ADC + CAN thermistor path must continue running.
 * ===========================================================================
 */

#include "master.h"

bool bq_is_active = false;

/* USER CODE BEGIN (2) */
int UART_RX_RDY = 0;
int RTI_TIMEOUT = 0;
/* USER CODE END */

int main(void)
{
    System_AppInit();
    LED_All_Pulse(100u);

    bq_is_active = bq79616_try_init();

    while (true) {
        /* Thermistor pipeline has priority and must always run. */
        ADC_ServiceTask();
        CAN_ServiceTask();

        if (bq_is_active) {
            if (!bq79616_service_task()) {
                bq_is_active = false;
                LOG_WARN("BQ comm lost. BQ task disabled; ADC/CAN thermistor TX continues.");
            }
            // Log hello world
            LOG_INFO("Hello world! Tick=%lu", HAL_GetTick());
        }

    }
}
