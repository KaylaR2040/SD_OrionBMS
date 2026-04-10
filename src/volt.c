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

/* Service task wrapper for voltage subsystem */
void Volt_ServiceTask(void)
{
    /* Delegate to BQ79616 service task; it handles volt_status internally */
    (void)bq79616_service_task();
}
