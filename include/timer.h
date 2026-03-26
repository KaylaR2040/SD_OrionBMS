/*
 * ===========================================================================
 * File: timer.h
 * Description: Hardware timer for periodic CAN message triggers (100ms, 200ms)
 *
 * Notes:
 *   - Uses TIM6 basic timer with 10ms base tick
 *   - ISR sets flags, main loop checks and clears them
 *   - Keeps ISR short and CAN transmission in non-interrupt context
 * ===========================================================================
 */

#ifndef TIMER_H
#define TIMER_H

// STM32 HAL
#include "stm32g4xx_hal.h"

// C standard library
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Timer Configuration                                                        */
/* -------------------------------------------------------------------------- */

/** Base timer tick period in milliseconds */
#define TIMER_BASE_PERIOD_MS    10u

/** Interval definitions */
#define TIMER_INTERVAL_100MS    100u
#define TIMER_INTERVAL_200MS    200u

#define TIMER5_TARGET_HZ        100000u      /* 10 us tick */
#define TIMER5_PERIOD_COUNTS    10u          /* ARR+1 counts per tick */
#define TIMER5_TICK_US          10u

#define BQ_WAKE_INTERVAL_US     2500u // 2.5 ms wake pulse duration per BQ79616 datasheet recommendation
#define BQ_WAKE_TICKS           (BQ_WAKE_INTERVAL_US / TIMER5_TICK_US)

// Timer6 Speed: 10 kHz (100us tick, 0.1ms), with period of 100 ticks for 10 ms overflow
#define TIMER6_CLK_HZ        10000u
#define TIMER6_PERIOD        100u
#define TICKS_FOR_100MS     10u
#define TICKS_FOR_200MS     20u

// Timer7 (CAN dedicated): 10 kHz counter, 100 ms hardware interrupt period
#define TIMER7_CLK_HZ        10000u
#define TIMER7_PERIOD        ((TIMER7_CLK_HZ / 1000u) * TIMER_INTERVAL_100MS)
#define CAN_TICKS_FOR_100MS  1u
#define CAN_TICKS_FOR_200MS  2u

/* -------------------------------------------------------------------------- */
/* Global Variables                                                           */
/* -------------------------------------------------------------------------- */

/** Timer handle (defined in timer.c) */
extern TIM_HandleTypeDef g_htim6;
extern TIM_HandleTypeDef g_htim7;
extern TIM_HandleTypeDef htim5;

/** Flags set by ISR, checked and cleared by main loop */
extern volatile bool g_flag_100ms;
extern volatile bool g_flag_200ms;
extern volatile bool g_flag_bq_wake;

/** CAN-dedicated timer flags (independent from main loop timing) */
extern volatile bool g_flag_can_100ms;
extern volatile bool g_flag_can_200ms;

/* -------------------------------------------------------------------------- */
/* Function Declarations                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize TIM6 for periodic interrupts
 * @note  Call once during system initialization
 */
void Timer6_Init(void);
void Timer5_Init(void);
void Timer7_Init(void);
void Timers_Init(void);

bool Timer5_CheckWakeFlag(void);
void Timer5_ArmWake(void);
void Timer5_DisarmWake(void);

/**
 * @brief Check if 100ms flag is set and clear it
 * @return true if 100ms interval has elapsed, false otherwise
 */
bool Timer_Check100msFlag(void);

/**
 * @brief Check if 200ms flag is set and clear it
 * @return true if 200ms interval has elapsed, false otherwise
 */
bool Timer_Check200msFlag(void);

/**
 * @brief Check if CAN 100ms flag is set and clear it (independent timing)
 * @return true if 100ms interval has elapsed, false otherwise
 */
bool Timer_CheckCan100msFlag(void);

/**
 * @brief Check if CAN 200ms flag is set and clear it (independent timing)
 * @return true if 200ms interval has elapsed, false otherwise
 */
bool Timer_CheckCan200msFlag(void);

/**
 * @brief Debug: Print internal counter state to verify 100ms/200ms timing
 */
void Timer_DebugPrintCounters(void);

/**
 * @brief Debug: Get total TIM6 ISR call count (incremented once per interrupt)
 */
uint32_t Timer_GetIsrCallCount(void);

/**
 * @brief Get current tick count (10ms resolution)
 * @return Number of 10ms ticks since Timer6_Init()
 */
uint32_t Timer_GetTickCount(void);

/**
 * @brief Stop the timer (for low-power modes)
 */
void Timer_Stop(void);

/**
 * @brief Restart the timer after being stopped
 */
void Timer_Start(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H */
