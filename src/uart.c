/*
 * ===========================================================================
 * File: uart.c
 * Description: Consolidated UART support:
 *   - USART2 on PA2 (TX) / PA3 (RX) for console logging
 *   - USART1 on PC4 (TX) / PC5 (RX) for BQ79616 transport
 * ===========================================================================
 */

#include "master.h"

/* Handles per channel */
UART_HandleTypeDef uart_stlink;
UART_HandleTypeDef uart_bq79616;

/* Cached UART pointer used by the lightweight logging layer. */
static UART_HandleTypeDef *log_uart = NULL;

/* Log level tags printed ahead of each line. */
static const char *const level_tags[LOG_LEVEL_COUNT] = {
    "[INFO]",
    "[WARN]",
    "[ERROR]",
    "[DEBUG]"
};

/* ANSI color codes that correspond to each log level tag. */
static const char *const level_colors[LOG_LEVEL_COUNT] = {
    LOG_COLOR_INFO,
    LOG_COLOR_WARN,
    LOG_COLOR_ERROR,
    LOG_COLOR_DEBUG
};

/* Initialize logging UART on USART2 -> PA2/PA3 (AF7). Pin mapping is set in
 * HAL_UART_MspInit (src/stm32g4xx_hal_msp.c).
 */
void UART_Stlink_Init(void)
{

    /* Logging UART is USART2 => PA2/PA3 (AF7) per HAL_UART_MspInit. */
    uart_stlink.Instance = USART2;
    uart_stlink.Init.BaudRate = 115200;
    uart_stlink.Init.WordLength = UART_WORDLENGTH_8B;
    uart_stlink.Init.StopBits = UART_STOPBITS_1;
    uart_stlink.Init.Parity = UART_PARITY_NONE;
    uart_stlink.Init.Mode = UART_MODE_TX_RX;
    uart_stlink.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_stlink.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&uart_stlink) != HAL_OK) {
        log_status = FAILED;
        Error_Handler();
    } else {
        log_status = ACTIVE;
        /* Point the logging layer at the initialized UART. */
        Log_Init(&uart_stlink);
    }
}

/* Configure BQ79616 UART on PC4 (TX) / PC5 (RX) via USART1 */
void UART_BQ79616_Init(void)
{
    uart_bq79616.Instance = USART1;
    uart_bq79616.Init.BaudRate = BQ_BAUDRATE; /* BQ79616 supports baudrate of 1 Mbps */
    uart_bq79616.Init.WordLength = UART_WORDLENGTH_8B;
    uart_bq79616.Init.StopBits = UART_STOPBITS_1;
    uart_bq79616.Init.Parity = UART_PARITY_NONE;
    uart_bq79616.Init.Mode = UART_MODE_TX_RX;
    uart_bq79616.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_bq79616.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&uart_bq79616) != HAL_OK) {
        Error_Handler();
    }
}

/* Bind the logging layer to the provided UART handle */
void Log_Init(UART_HandleTypeDef *huart)
{
    /* Caller owns the UART lifetime; we only cache the pointer. */
    log_uart = huart;
}

/* Emit a formatted, colorized log line over UART */
void Log_Printf(log_level_t level, const char *fmt, ...)
{
    /* Check if logging subsystem is active */
    if (log_status == FAILED) {
        return;
    }

    if (!log_uart || level >= LOG_LEVEL_COUNT || fmt == NULL) {
        return;
    }

    /* Render the formatted message into a local buffer. */
    char line[256];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    if (len < 0) {
        return;
    }
    if ((size_t)len >= sizeof(line)) {
        len = (int)sizeof(line) - 1;
    }

    /* Prefix with level tag + color for clarity on the console. */
    const char *color = level_colors[level];
    const char *tag = level_tags[level];

    /* Emit ANSI color, tag, payload, then reset color. */
    HAL_UART_Transmit(log_uart, (uint8_t *)color, (uint16_t)strlen(color), HAL_MAX_DELAY);
    HAL_UART_Transmit(log_uart, (uint8_t *)tag, (uint16_t)strlen(tag), HAL_MAX_DELAY);
    HAL_UART_Transmit(log_uart, (uint8_t *)" ", 1, HAL_MAX_DELAY);
    HAL_UART_Transmit(log_uart, (uint8_t *)line, (uint16_t)len, HAL_MAX_DELAY);
    HAL_UART_Transmit(log_uart, (uint8_t *)LOG_COLOR_RESET, (uint16_t)strlen(LOG_COLOR_RESET), HAL_MAX_DELAY);
    /* Always end log entries with CRLF for terminal line discipline. */
    const uint8_t newline[2] = {'\r', '\n'};
    HAL_UART_Transmit(log_uart, newline, 2, HAL_MAX_DELAY);
}
