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

/* Log type tags printed ahead of each line. */
static const char *const type_tags[LOG_TYPE_COUNT] = {
    "",
    "[INFO]",
    "[WARN]",
    "[ERROR]",
    "[DEBUG]"
};

/* ANSI color codes that correspond to each log type tag. */
static const char *const type_colors[LOG_TYPE_COUNT] = {
    GREEN,
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
    uart_stlink.Init.BaudRate = UART_STLINK_BAUDRATE;
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

static bool Log_IsAnsiColorToken_(const char *token)
{
    if (token == NULL) {
        return false;
    }

    return (strcmp(token, RESET) == 0) ||
           (strcmp(token, BLACK) == 0) ||
           (strcmp(token, RED) == 0) ||
           (strcmp(token, ORANGE) == 0) ||
           (strcmp(token, GREEN) == 0) ||
           (strcmp(token, YELLOW) == 0) ||
           (strcmp(token, BLUE) == 0) ||
           (strcmp(token, MAGENTA) == 0) ||
           (strcmp(token, CYAN) == 0) ||
           (strcmp(token, WHITE) == 0) ||
           (strcmp(token, BRIGHT_BLACK) == 0) ||
           (strcmp(token, BRIGHT_RED) == 0) ||
           (strcmp(token, BRIGHT_ORANGE) == 0) ||
           (strcmp(token, BRIGHT_GREEN) == 0) ||
           (strcmp(token, BRIGHT_YELLOW) == 0) ||
           (strcmp(token, BRIGHT_BLUE) == 0) ||
           (strcmp(token, BRIGHT_MAGENTA) == 0) ||
           (strcmp(token, BRIGHT_CYAN) == 0) ||
           (strcmp(token, BRIGHT_WHITE) == 0);
}

static void Log_VPrint(log_type_t type, const char *fmt, va_list ap, bool parse_color_prefix)
{
    /* Check if logging subsystem is active */
    if (log_status == FAILED) {
        return;
    }

    if (!log_uart || type >= LOG_TYPE_COUNT || fmt == NULL) {
        return;
    }

    const char *message_prefix = NULL;
    const char *format = fmt;
    va_list format_args;
    va_copy(format_args, ap);

    /* Support LOG_PRINT(type, COLOR, "fmt", ...) for explicit message color. */
    if (parse_color_prefix && Log_IsAnsiColorToken_(fmt)) {
        const char *candidate_fmt = va_arg(format_args, const char *);
        if (candidate_fmt != NULL) {
            message_prefix = fmt;
            format = candidate_fmt;
        }
    }

    /* Render the formatted message into a local buffer. */
    char line[LOG_LINE_BUFFER_SIZE];
    int len = vsnprintf(line, sizeof(line), format, format_args);
    va_end(format_args);
    if (len < 0) {
        return;
    }
    if ((size_t)len >= sizeof(line)) {
        len = (int)LOG_LINE_BUFFER_SIZE - 1;
    }

    /* Prefix with type tag + color for clarity on the console. */
    const char *type_color = type_colors[type];
    const char *tag = type_tags[type];
    /* Explicit prefix wins; otherwise use type color (no forced white). */
    const char *message_color = (message_prefix != NULL) ? message_prefix : type_color;

    /*
     * Emit one physical line at a time and normalize any embedded '\n' so every
     * line starts from column 0 on serial consoles with strict line discipline.
     */
    size_t start = 0U;
    for (size_t i = 0U; i <= (size_t)len; ++i) {
        const bool at_end = (i == (size_t)len);
        if (!at_end && line[i] != '\n') {
            continue;
        }

        size_t part_len = i - start;
        if (part_len > 0U && line[start + part_len - 1U] == '\r') {
            part_len--;
        }

        /* Drop trailing empty segment when format string ends with '\n'. */
        if (!(at_end && part_len == 0U)) {
            const uint8_t line_start = '\r';
            HAL_UART_Transmit(log_uart, (uint8_t *)&line_start, LOG_SINGLE_CHAR_LEN, HAL_MAX_DELAY);

            if (type != LOG_TYPE_NONE) {
                HAL_UART_Transmit(log_uart, (uint8_t *)type_color, (uint16_t)strlen(type_color), HAL_MAX_DELAY);
                HAL_UART_Transmit(log_uart, (uint8_t *)tag, (uint16_t)strlen(tag), HAL_MAX_DELAY);
                HAL_UART_Transmit(log_uart, (uint8_t *)" ", LOG_SINGLE_CHAR_LEN, HAL_MAX_DELAY);
                HAL_UART_Transmit(log_uart, (uint8_t *)RESET, (uint16_t)strlen(RESET), HAL_MAX_DELAY);
            }

            HAL_UART_Transmit(log_uart, (uint8_t *)message_color, (uint16_t)strlen(message_color), HAL_MAX_DELAY);
            if (part_len > 0U) {
                HAL_UART_Transmit(log_uart, (uint8_t *)&line[start], (uint16_t)part_len, HAL_MAX_DELAY);
            }
            HAL_UART_Transmit(log_uart, (uint8_t *)RESET, (uint16_t)strlen(RESET), HAL_MAX_DELAY);

            const uint8_t line_end[LOG_CRLF_LEN] = {'\r', '\n'};
            HAL_UART_Transmit(log_uart, line_end, LOG_CRLF_LEN, HAL_MAX_DELAY);
        }

        start = i + 1U;
    }
}

/* Emit a formatted, colorized log line over UART */
void Log_Print(log_type_t type, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Log_VPrint(type, fmt, ap, true);
    va_end(ap);
}

/* Backward-compatible wrapper */
void Log_Printf(log_level_t level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Log_VPrint((log_type_t)level, fmt, ap, false);
    va_end(ap);
}
