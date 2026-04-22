/*
 * ===========================================================================
 * File: volt.c
 * Description: High-level voltage (cell voltage) service task via BQ79616
 *
 * Notes:
 *   - Wraps bq79616_service_task() and manages volt_status flag.
 *   - Device driver (bq79616 protocol) lives in bq79616.c.
 * ===========================================================================
 */

#include "master.h"

static volatile BQ_State_t s_bq_state = BQ_STATE_STARTUP_PENDING;

// void Volt_RunBlockingStartup(void)
// {
//     s_bq_state = BQ_STATE_STARTUP_PENDING;

//     // if(bq_shutdown_status == BQ_TURN_OFF) {
//     //     // bq79616_shutdown();
//     //     LOG_PRINT(LOG_TYPE_INFO, "BQ DISABLED: bq_shutdown_status == BQ_TURN_OFF");
//     //     volt_status = FAILED;
//     //     s_bq_state = BQ_STATE_FAILED;
//     //     return;
//     // }else{
//     if (bq79616_try_init()) {
//         volt_status = ACTIVE;
//         s_bq_state = BQ_STATE_READY;
//         LOG_PRINT(LOG_TYPE_INFO, "BQ startup state: READY");
//         return;
//     } else {
//         volt_status = FAILED;
//         s_bq_state = BQ_STATE_FAILED;
//         LOG_PRINT(LOG_TYPE_WARN, "BQ startup state: FAILED; CAN fault/status reporting stays active.");
//     }
//     // }
// }

void Volt_RunBlockingStartup(void)
{
    s_bq_state = BQ_STATE_STARTUP_PENDING;

    if(bq_shutdown_status == BQ_TURN_OFF) {
        // bq79616_shutdown();
        LOG_PRINT(LOG_TYPE_INFO, "BQ DISABLED: bq_shutdown_status == TURN_OFF");
        volt_status = FAILED;
        s_bq_state = BQ_STATE_FAILED;
        return;
    }else{
        if (bq79616_try_init()) {
            volt_status = ACTIVE;
            s_bq_state = BQ_STATE_READY;
            LOG_PRINT(LOG_TYPE_INFO, "BQ startup state: READY");
            return;
        }else{
            volt_status = FAILED;
            s_bq_state = BQ_STATE_FAILED;
            LOG_PRINT(LOG_TYPE_WARN, "BQ startup state: FAILED; CAN fault/status reporting stays active.");
        }
    }

}











BQ_State_t Volt_GetState(void)
{
    return s_bq_state;
}

bool Volt_IsFaultReportingMode(void)
{
    return s_bq_state == BQ_STATE_FAILED;
}

/* Service task wrapper for voltage subsystem */
void Volt_ServiceTask(void) {
    if (volt_status == ACTIVE && s_bq_state == BQ_STATE_READY) {
        if (!bq79616_service_task()) {
            volt_status = FAILED;
            s_bq_state = BQ_STATE_FAILED;
            LOG_PRINT(LOG_TYPE_ERROR, "BQ runtime state transitioned to FAILED; CAN remains on schedule.");
        }
    } else {
        LED_On(VOLT_LED);  /* Light up VOLT_LED on VOLT/BQ failure */
        return;
    }
}




