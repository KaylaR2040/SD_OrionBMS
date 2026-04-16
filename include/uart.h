/*
 * ===========================================================================
 * File: uart.h
 * Description: Consolidated UART interface for console logging and BQ79616
 *              transport channels.
 *
 * Layers:
 *   - Console/logging: USART2 on PA2/PA3 (ST-Link VCP)
 *   - BQ transport:    USART1 on PC4/PC5 (BQ79616)
 *
 * Notes:
 *   - Logging API uses LOG_PRINT(type, ...).
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

// Logging Status Flag used to enable or disable Logging if there are prehiperal issues
extern volatile bool log_status;

/* Console/logging UART (USART2 via ST-Link) */
extern UART_HandleTypeDef uart_stlink;

/* BQ79616 transport UART (USART1) */
extern UART_HandleTypeDef uart_bq79616;

typedef enum {
    LOG_TYPE_NONE = 0,
    LOG_TYPE_INFO,
    LOG_TYPE_WARN,
    LOG_TYPE_ERROR,
    LOG_TYPE_DEBUG,
    LOG_TYPE_COUNT
} log_type_t;

/* Backward-compatible type alias. */
typedef log_type_t log_level_t;

/* Backward-compatible enumerator aliases. */
#define LOG_LEVEL_NONE  LOG_TYPE_NONE
#define LOG_LEVEL_INFO  LOG_TYPE_INFO
#define LOG_LEVEL_WARN  LOG_TYPE_WARN
#define LOG_LEVEL_ERROR LOG_TYPE_ERROR
#define LOG_LEVEL_DEBUG LOG_TYPE_DEBUG
#define LOG_LEVEL_COUNT LOG_TYPE_COUNT

/* ANSI colors for terminal output */
/* Standard colors */
#define RESET   "\x1b[0m"
#define BLACK   "\x1b[30m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"

/* Bright colors */
#define BRIGHT_BLACK   "\x1b[90m"
#define BRIGHT_RED     "\x1b[91m"
#define BRIGHT_GREEN   "\x1b[92m"
#define BRIGHT_YELLOW  "\x1b[93m"
#define BRIGHT_BLUE    "\x1b[94m"
#define BRIGHT_MAGENTA "\x1b[95m"
#define BRIGHT_CYAN    "\x1b[96m"
#define BRIGHT_WHITE   "\x1b[97m"

/* Verbose aliases (compatibility) */
#define LOG_COLOR_RESET RESET
#define LOG_COLOR_BLACK BLACK
#define LOG_COLOR_RED RED
#define LOG_COLOR_GREEN GREEN
#define LOG_COLOR_YELLOW YELLOW
#define LOG_COLOR_BLUE BLUE
#define LOG_COLOR_MAGENTA MAGENTA
#define LOG_COLOR_CYAN CYAN
#define LOG_COLOR_WHITE WHITE

#define LOG_COLOR_INFO  GREEN
#define LOG_COLOR_WARN  YELLOW
#define LOG_COLOR_ERROR RED
#define LOG_COLOR_DEBUG CYAN
#define LOG_COLOR_FIELD BRIGHT_BLUE
#define LOG_COLOR_VALUE MAGENTA
#define LOG_COLOR_HILITE "\x1b[96m"

/* Initialization */
void UART_Stlink_Init(void);
void UART_BQ79616_Init(void);

/* Logging helpers (console channel only) */
void Log_Init(UART_HandleTypeDef *huart);
void Log_Print(log_type_t type, const char *fmt, ...);
void Log_Printf(log_level_t level, const char *fmt, ...);

#define LOG_PRINT(type, ...) Log_Print((type), __VA_ARGS__)

#endif /* UART_H */
