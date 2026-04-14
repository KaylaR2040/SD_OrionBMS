/*
 * ===========================================================================
 * File: volt.h
 * Description: High-level voltage (cell voltage) management via BQ79616
 *
 * Notes:
 *   - BQ79616 device driver lives in bq79616.c; this provides the service interface.
 *   - Voltage subsystem status is tracked via volt_status flag.
 * ===========================================================================
 */

#ifndef VOLT_H
#define VOLT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BQ_STATE_STARTUP_PENDING = 0,
    BQ_STATE_READY,
    BQ_STATE_FAILED
} BQ_State_t;

void Volt_RunBlockingStartup(void);
BQ_State_t Volt_GetState(void);
bool Volt_IsFaultReportingMode(void);

/* Service task for BQ79616 voltage measurements and monitoring */
void Volt_ServiceTask(void);

#ifdef __cplusplus
}
#endif

#endif /* VOLT_H */
