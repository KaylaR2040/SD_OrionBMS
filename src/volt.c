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

void Volt_RunBlockingStartup(void)
{
    s_bq_state = BQ_STATE_STARTUP_PENDING;

    if (bq79616_try_init()) {
        volt_status = ACTIVE;
        s_bq_state = BQ_STATE_READY;
        LOG_INFO("BQ startup state: READY");
        return;
    }

    volt_status = FAILED;
    s_bq_state = BQ_STATE_FAILED;
    LOG_WARN("BQ startup state: FAILED; CAN fault/status reporting stays active.");
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
void Volt_ServiceTask(void)
{
    if (s_bq_state != BQ_STATE_READY) {
        LED_On(VOLT_LED);  /* Light up VOLT_LED on VOLT/BQ failure */
        return;
    }

    if (!bq79616_service_task()) {
        volt_status = FAILED;
        s_bq_state = BQ_STATE_FAILED;
        LOG_ERROR("BQ runtime state transitioned to FAILED; CAN remains on schedule.");
    }
}
