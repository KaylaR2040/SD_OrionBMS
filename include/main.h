/*
 * ===========================================================================
 * File: main.h
 * Description: Project-wide hardware mappings and shared handles for main.c.
 *
 * Notes:
 *   - HAL and LL coexist intentionally; keep both includes available.
 * ===========================================================================
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------- Includes ----------------------- */
#include "stm32g4xx_hal.h"
#include "error_handling.h"

/* Use LL alongside HAL: enabled from build system (platformio.ini -D) */
#ifdef USE_FULL_LL_DRIVER
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_exti.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_gpio.h"
#endif /* USE_FULL_LL_DRIVER */

/* Explicitly include FDCAN HAL (pulled in via hal_conf when enabled) */
#include "stm32g4xx_hal_fdcan.h"

#if defined(USE_FULL_ASSERT)
#include "stm32_assert.h"
#endif

/* ----------------------- Exported types / externs ----------------------- */
extern FDCAN_HandleTypeDef    g_fdcan1;
extern FDCAN_TxHeaderTypeDef  g_can_tx_header;
extern uint8_t                g_can_tx_data[8];

/* LED pins (adjust to match board wiring as needed) */
#define LED1_Pin          GPIO_PIN_9
#define LED1_GPIO_Port    GPIOA
#define LED2_Pin          GPIO_PIN_8
#define LED2_GPIO_Port    GPIOA
#define LED3_Pin          GPIO_PIN_9
#define LED3_GPIO_Port    GPIOC

/* ----------------------- Exported functions ----------------------- */
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
