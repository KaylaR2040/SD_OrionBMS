/*
 * ===========================================================================
 * File: init.h
 * Description: System-wide initialization entry points for HAL, GPIO, and clocks.
 *
 * Notes:
 *   - System_AppInit orchestrates startup; see init.c for implementations.
 * ===========================================================================
 */

#ifndef INIT_H
#define INIT_H

// STM32 HAL
#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void System_AppInit(void);
void SystemClock_Config(void);
void GPIO_App_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* INIT_H */
