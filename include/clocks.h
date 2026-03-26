/*
 * ===========================================================================
 * File: clocks.h
 * Description: Clock tree configuration entry points for system and peripherals.
 *
 * Notes:
 *   - Implementations live in clocks.c and SystemClock_Config in init.c.
 * ===========================================================================
 */

#ifndef CLOCKS_H
#define CLOCKS_H

// STM32 HAL
#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef can_clk_cfg(void);
HAL_StatusTypeDef adc_clk_cfg(void);
void clocks_configure_all(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOCKS_H */
