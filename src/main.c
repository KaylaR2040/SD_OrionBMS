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

/* Boolean Flags to Track if each subsystem properly inits and is active
    If flag is ACTIVE, the service task will run
    If flag is FAILED, the service task will be skipped and an error will be logged
*/

volatile bool log_status = ACTIVE;
volatile bool can_status = ACTIVE;
volatile bool therm_status = ACTIVE;
volatile bool volt_status = ACTIVE;

/* USER CODE BEGIN (2) */
int UART_RX_RDY = 0;
int RTI_TIMEOUT = 0;
/* USER CODE END */

int main(void)
{
    System_AppInit();
    LED_All_Pulse(1000u);


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
        }
        
    }
}
