/*
 * ===========================================================================
 * File: uart.h
 * Description: Consolidated UART interface for console (ST-Link VCP) and
 *              BQ79616 transport channels.
 *
 * Layers:
 *   - Console/logging: USART1 on ST-Link VCP
 *   - BQ transport:    USART2 to BQ79616 battery monitor
 *
 * Notes:
 *   - Logging helpers remain for convenience (LOG_INFO/WARN/ERROR/DEBUG).
 *   - Transport-specific init is provided for each channel.
 * ===========================================================================
 */

#ifndef UART_H
#define UART_H

// STM32 HAL
#include "stm32g4xx_hal.h"

// Project headers
#include "error_handling.h"

// C standard library
#include <stdbool.h>
#include <stdarg.h>

/* Console/logging UART (USART1 via ST-Link) */
extern UART_HandleTypeDef uart_stlink;

/* BQ79616 transport UART (USART2) */
extern UART_HandleTypeDef uart_bq79616;

#define UART_LOG_ENABLED 1

typedef enum {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_COUNT
} log_level_t;

/* ANSI colors for terminal output */
#define LOG_COLOR_RESET "\x1b[0m"
#define LOG_COLOR_INFO  "\x1b[32m"
#define LOG_COLOR_WARN  "\x1b[33m"
#define LOG_COLOR_ERROR "\x1b[31m"
#define LOG_COLOR_DEBUG "\x1b[36m"
#define LOG_COLOR_FIELD "\x1b[94m"
#define LOG_COLOR_VALUE "\x1b[35m"
#define LOG_COLOR_HILITE "\x1b[96m"

/* Initialization */
void UART_Stlink_Init(void);
void UART_BQ79616_Init(uint32_t baudrate);

/* Logging helpers (console channel only) */
void Log_Init(UART_HandleTypeDef *huart);
void Log_Printf(log_level_t level, const char *fmt, ...);

#define LOG_INFO(...)  Log_Printf(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...)  Log_Printf(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERROR(...) Log_Printf(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_DEBUG(...) Log_Printf(LOG_LEVEL_DEBUG, __VA_ARGS__)

#endif /* UART_H */
