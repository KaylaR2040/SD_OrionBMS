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

/* USER CODE BEGIN (2) */
int UART_RX_RDY = 0;
int RTI_TIMEOUT = 0;
/* USER CODE END */

int main(void)
{
    System_AppInit();

    /* Use our HAL-based init + read helpers (previous working path) */
    int bq_status = bq79616_init_device();
    if (bq_status != 0) {
        LOG_ERROR("BQ init failed: bq_status=%d", bq_status);
        Error_Handler();
    }

    uint8_t partid = 0u;
    if (bq79616_read_partid_once(&partid) == 0) {
        LOG_INFO("BQ79616 PARTID=0x%02X", partid);
    }

    uint16_t cell_mv[16] = {0};
    while (true) {
        (void)BQ_ServiceTask(); /* keep-alive */

        bq_status = bq79616_read_all_cells(cell_mv, 16u);
        if (bq_status == 0) {
            LOG_INFO("Cell mV: "
                     "1:%u 2:%u 3:%u 4:%u 5:%u 6:%u 7:%u 8:%u "
                     "9:%u 10:%u 11:%u 12:%u 13:%u 14:%u 15:%u 16:%u",
                     cell_mv[0], cell_mv[1], cell_mv[2], cell_mv[3],
                     cell_mv[4], cell_mv[5], cell_mv[6], cell_mv[7],
                     cell_mv[8], cell_mv[9], cell_mv[10], cell_mv[11],
                     cell_mv[12], cell_mv[13], cell_mv[14], cell_mv[15]);
        }

        HAL_Delay(200u);
    }
}
