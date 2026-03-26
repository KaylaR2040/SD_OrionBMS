/*
 * ===========================================================================
 * File: clocks.c
 * Description: Peripheral clock configuration helpers (CAN, ADC).
 * ===========================================================================
 */

#include "master.h"

/* Configure the ADC12 clock tree for the on-chip ADC */
HAL_StatusTypeDef adc_clk_cfg(void)
{
    RCC_PeriphCLKInitTypeDef periph = {0};
    periph.PeriphClockSelection = RCC_PERIPHCLK_ADC12;
    periph.Adc12ClockSelection = RCC_ADC12CLKSOURCE_PLL;
    return HAL_RCCEx_PeriphCLKConfig(&periph);
}

/* Configure the dedicated FDCAN clock domain */
HAL_StatusTypeDef can_clk_cfg(void)
{
    RCC_PeriphCLKInitTypeDef periph = {0};
    periph.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    periph.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;
    return HAL_RCCEx_PeriphCLKConfig(&periph);
}

/* Configure all required peripheral clocks in one shot and trap failures */
void clocks_configure_all(void)
{
    HAL_StatusTypeDef st = adc_clk_cfg();
    if (st != HAL_OK) {
        LOG_ERROR("ADC clock config failed");
        Error_Handler();
    }
    
    st = can_clk_cfg();
    if (st != HAL_OK) {
        LOG_ERROR("FDCAN clock config failed");
        Error_Handler();
    }
}
