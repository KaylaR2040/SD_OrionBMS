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
    ADC_App_Init();          /* Initialize internal ADC driver */
    CAN_App_Init(CAN_APP_DEFAULT_KBPS); /* Start FDCAN at the project-configured bitrate */
    UART_Stlink_Init();      /* Console/logging UART (LPUART1 or USART1) */
    UART_BQ79616_Init(1000000U);     /* BQ79616 transport UART on USART1 (PC4/PC5) */
    // UART_BQ79616_Init(115200U);     /* BQ79616 transport UART on USART1 (PC4/PC5) */
    Timers_Init();           /* Start hardware timers for periodic tasks */

    /* Load the developer thermistor override table before the ADC/CAN tasks start. */
    CAN_Debug_Init();
    CAN_Debug_SetMode(true);
    CAN_Debug_ApplyManualOverrides();
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
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) 
    {
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
