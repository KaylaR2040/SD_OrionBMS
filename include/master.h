// Include all project headers in a single "master" header for convenience
/*
 * ===========================================================================
 * File: master.h
 * Description: Centralized header that includes all project headers for easy
 *              inclusion in source files.
 *
 * Notes:
 *   - This file is intended to be included in all .c files to provide access
 *     to the full project API without needing to include individual headers.
 *   - It also serves as a single point of maintenance for header dependencies.
 * ===========================================================================
 */

#ifndef MASTER_H
#define MASTER_H

/* Standard C library */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* STM32 HAL base */
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_fdcan.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_gpio.h"

/* Master Macros */
#define ERROR -1
#define SUCCESS 0

/* Project headers */
#include "adc.h"
#include "bq79616.h"
#include "can.h"
#include "can_messages.h"
#include "can_debugging.h"
#include "clocks.h"
#include "error_handling.h"
#include "init.h"
#include "led.h"
#include "main.h"
#include "thermistor_table.h"
#include "timer.h"
#include "uart.h"

#endif /* MASTER_H */
