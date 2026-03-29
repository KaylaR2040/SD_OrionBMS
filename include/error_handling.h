/*
 * ===========================================================================
 * File: error_handling.h
 * Description: Application-wide error handlers and assert-style traps.
 *
 * Notes:
 *   - Implementations live in error_handling.c; keep interfaces minimal.
 * ===========================================================================
 */

#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

// STM32 HAL
#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void Error_Handler(void);



#ifdef __cplusplus
}
#endif

#endif /* ERROR_HANDLING_H */
