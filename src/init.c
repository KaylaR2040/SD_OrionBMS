/*
 * ===========================================================================
 * File: init.c
 * Description: System-wide initialization helpers for clocks, GPIO, and HAL.
 *
 * Notes:
 *   - Keeps startup wiring minimal so main() remains orchestration-only.
 * ===========================================================================
 */

#include "master.h"

/* Initialize HAL, clocks, GPIO, LEDs, and the logging UART */
void System_AppInit(void)
{
    HAL_Init();              /* Reset peripherals, init flash interface and SysTick */
    SystemClock_Config();    /* Set up PLL and system bus clocks */
    LED_Init();              /* Configure on-board LED GPIOs */
    clocks_configure_all();  /* Board-specific clock tree helpers */
    UART_Stlink_Init();               /* Console/logging UART on USART2 (PA2/PA3) */
    UART_BQ79616_Init();      /* BQ79616 transport UART on USART1 (PC4/PC5) */
    Therm_App_Init();        /* Initialize internal ADC driver for thermistors */
    CAN_App_Init(CAN_APP_DEFAULT_KBPS); /* Start FDCAN at the project-configured bitrate */
    Timers_Init();           /* Start hardware timers for periodic tasks */
    CAN_SetSchedulingEnabled(false, false);

    /* Load the developer thermistor override table before the thermistor/CAN tasks start. */
    CAN_Debug_Init();
    CAN_Debug_SetMode(false);
    CAN_Debug_ApplyManualOverrides();

    /* Blocking BQ bring-up owns startup. CAN scheduling is released only after
     * BQ reaches a terminal READY or FAILED state. */
    Volt_RunBlockingStartup();

    if (Volt_GetState() == BQ_STATE_READY) {
        CAN_SetSchedulingEnabled(true, false);
    } else if (Volt_GetState() == BQ_STATE_FAILED) {
        CAN_SetSchedulingEnabled(true, true);
    }

    // Log that all Init Complete and Print Subsystem Statuses
    LOG_PRINT(LOG_TYPE_INFO, MAGENTA,"System initialization complete. Entering main loop.");
    LOG_PRINT(LOG_TYPE_INFO, MAGENTA, "Subsystem statuses:");
    LOG_PRINT(LOG_TYPE_INFO, MAGENTA, " - CAN: %s", can_status == ACTIVE ? "ACTIVE" : "FAILED");
    LOG_PRINT(LOG_TYPE_INFO, MAGENTA, " - BQ: %s", volt_status == ACTIVE ? "ACTIVE" : "FAILED");
    LOG_PRINT(LOG_TYPE_INFO, MAGENTA, " - Thermistors: %s", therm_status == ACTIVE ? "ACTIVE" : "FAILED");
}

/* Configure PLL and bus clocks for the STM32G4 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM = RCC_PLLM_DIV4;
    osc.PLL.PLLN = 40;
    osc.PLL.PLLR = RCC_PLLR_DIV2;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2;
    osc.PLL.PLLP = RCC_PLLP_DIV7;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}
