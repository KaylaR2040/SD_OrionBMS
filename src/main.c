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

    /* Mirror TI sample main flow using our HAL-backed TI-style functions */
    Wake79616();
    delayms(12 * 1); /* TOTALBOARDS=1 */
    Wake79616();
    delayms(12 * 1);

    AutoAddress();
    delayus(4000);
    WriteReg(0, FAULT_MSK2, 0x40, 1, FRMWRT_ALL_W); /* mask CUST_CRC */
    ResetAllFaults(0, FRMWRT_ALL_W);

    /* Configure ADC like TI sample */
    WriteReg(0, ACTIVE_CELL, 0x0A, 1, FRMWRT_ALL_W);   /* all cells active */
    WriteReg(0, ADC_CONF1, 0x02, 1, FRMWRT_ALL_W);     /* 26 Hz LPF */
    WriteReg(0, ADC_CTRL1, 0x0E, 1, FRMWRT_ALL_W);     /* continuous, LPF, MAIN_GO */
    delayus(38000 + 5 * 1);

    uint8_t response_frame[(16*2+6)*1] = {0};

    while (true) {
        /* Read all 16 cell voltages via TI-style ReadReg */
        memset(response_frame, 0, sizeof(response_frame));
        ReadReg(0, VCELL16_HI, response_frame, 16*2, 0, FRMWRT_ALL_R);

        /* Parse and log like TI sample: top-of-stack first in response */
        for (int i = 0; i < 32; i += 2) {
            uint16_t raw = (uint16_t)((response_frame[4 + i] << 8) | response_frame[5 + i]);
            float cellVoltage = ((int16_t)raw) * 0.00019073f;
            LOG_INFO("Cell %d = %.3f V", (32 - i)/2, cellVoltage);
        }

        HAL_Delay(200u);
    }
}
