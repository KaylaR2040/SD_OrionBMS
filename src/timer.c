/*
 * File: timer.c
 * Description: Hardware timer for 100ms and 200ms CAN message triggers
 */

#include "master.h"

#define TIMER_VERBOSE_LOGS 0

TIM_HandleTypeDef g_htim6;
TIM_HandleTypeDef g_htim7;
TIM_HandleTypeDef htim5;

volatile bool g_flag_100ms = false;
volatile bool g_flag_200ms = false;
volatile bool g_flag_bq_wake = false;

/* CAN-dedicated timer (TIM7) flags - independent from main loop */
volatile bool g_flag_can_100ms = false;
volatile bool g_flag_can_200ms = false;

static volatile uint32_t s_counter_100ms = 0u;
static volatile uint32_t s_counter_200ms = 0u;
static volatile uint32_t s_counter_10us = 0u;
static volatile bool     s_wake_armed = false;
static volatile uint32_t s_isr_call_count = 0u;  /* DEBUG: count TIM6 ISR calls */

/* TIM7 CAN timer counters */
static volatile uint32_t s_can_counter_100ms = 0u;
static volatile uint32_t s_can_counter_200ms = 0u;

// Init all timers
void Timers_Init(void)
{
    Timer5_Init();
    Timer6_Init();
    Timer7_Init();
}

void Timer5_Init(void)
{
    __HAL_RCC_TIM5_CLK_ENABLE();

    /* Timer clock (APB1): account for x2 when prescaler != 1 */
    uint32_t timer_clk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
        timer_clk *= 2u;
    }

    uint32_t prescaler = (timer_clk / (TIMER5_TARGET_HZ * TIMER5_PERIOD_COUNTS)) - 1u;
    uint32_t period = TIMER5_PERIOD_COUNTS - 1u;

    htim5.Instance = TIM5;
    htim5.Init.Prescaler = prescaler;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = period;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&htim5);
    HAL_NVIC_SetPriority(TIM5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);
}


// Initialize TIM6 to generate periodic interrupts at TIMER6_CLK_HZ / TIMER6_PERIOD frequency
void Timer6_Init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();

    /* Get the actual APB1 timer clock (may be x2 PCLK1 if APB1_DIV > 1) */
    uint32_t timer_clk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
        timer_clk *= 2u;  /* Timers get x2 when APB prescaler > 1 */
    }

    uint32_t prescaler = (timer_clk / TIMER6_CLK_HZ) - 1u;

#if TIMER_VERBOSE_LOGS
    LOG_INFO("=== TIM6 INITIALIZATION DEBUG ===");
    LOG_INFO("  SystemCoreClock = %lu Hz", (unsigned long)SystemCoreClock);
    LOG_INFO("  HAL_RCC_GetPCLK1Freq() = %lu Hz", (unsigned long)HAL_RCC_GetPCLK1Freq());
    LOG_INFO("  Actual timer_clk = %lu Hz", (unsigned long)timer_clk);
    LOG_INFO("  TIMER6_CLK_HZ target = %lu Hz (10 kHz = 0.1ms per count)", (unsigned long)TIMER6_CLK_HZ);
    LOG_INFO("  Calculated prescaler = %lu", (unsigned long)prescaler);
    LOG_INFO("  TIMER6_PERIOD = %u -> interrupt every %u * %u = %lums",
             TIMER6_PERIOD,
             (unsigned)TIMER6_PERIOD,
             (unsigned)(prescaler + 1u),
             (unsigned long)(TIMER6_PERIOD * (prescaler + 1u) / (timer_clk / 1000u)));
    LOG_INFO("  TICKS_FOR_100MS = %u -> 100ms flag every %u interrupts", TICKS_FOR_100MS, TICKS_FOR_100MS);
    LOG_INFO("  TICKS_FOR_200MS = %u -> 200ms flag every %u interrupts", TICKS_FOR_200MS, TICKS_FOR_200MS);
#endif

    g_htim6.Instance = TIM6;
    g_htim6.Init.Prescaler = prescaler;
    g_htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_htim6.Init.Period = TIMER6_PERIOD - 1u;
    g_htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&g_htim6);
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    HAL_TIM_Base_Start_IT(&g_htim6);
}

/* Initialize TIM7 for CAN message timing (dedicated, independent from main loop) */
void Timer7_Init(void)
{
    __HAL_RCC_TIM7_CLK_ENABLE();

    /* Use same APB1 timer clock as TIM6 */
    uint32_t timer_clk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
        timer_clk *= 2u;
    }

    uint32_t prescaler = (timer_clk / TIMER7_CLK_HZ) - 1u;

#if TIMER_VERBOSE_LOGS
    LOG_INFO("=== TIM7 (CAN-DEDICATED) INITIALIZATION ===");
    LOG_INFO("  Actual timer_clk = %lu Hz", (unsigned long)timer_clk);
    LOG_INFO("  TIMER7_CLK_HZ target = %lu Hz (10 kHz = 0.1ms per count)", (unsigned long)TIMER7_CLK_HZ);
    LOG_INFO("  Calculated prescaler = %lu", (unsigned long)prescaler);
    LOG_INFO("  TIMER7_PERIOD = %u -> interrupt every %lums",
             TIMER7_PERIOD,
             (unsigned long)((TIMER7_PERIOD * 1000u) / TIMER7_CLK_HZ));
    LOG_INFO("  CAN 100ms: every %u interrupt, CAN 200ms: every %u interrupts",
             CAN_TICKS_FOR_100MS, CAN_TICKS_FOR_200MS);
#endif

    g_htim7.Instance = TIM7;
    g_htim7.Init.Prescaler = prescaler;
    g_htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_htim7.Init.Period = TIMER7_PERIOD - 1u;
    g_htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&g_htim7);
    HAL_NVIC_SetPriority(TIM7_DAC_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM7_DAC_IRQn);
    HAL_TIM_Base_Start_IT(&g_htim7);
}

/* Timer interrupt handler for both TIM6 and TIM7 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        s_isr_call_count++;  /* DEBUG: every TIM6 interrupt increments this */
        
        s_counter_100ms++;
        if (s_counter_100ms >= TICKS_FOR_100MS) {
            s_counter_100ms = 0u;
            g_flag_100ms = true;
        }

        s_counter_200ms++;
        if (s_counter_200ms >= TICKS_FOR_200MS) {
            s_counter_200ms = 0u;
            g_flag_200ms = true;
        }
    }

    /* TIM7: Dedicated CAN message timing (independent from main loop) */
    if (htim->Instance == TIM7) {
        s_can_counter_100ms++;
        if (s_can_counter_100ms >= CAN_TICKS_FOR_100MS) {
            s_can_counter_100ms = 0u;
            g_flag_can_100ms = true;
        }

        s_can_counter_200ms++;
        if (s_can_counter_200ms >= CAN_TICKS_FOR_200MS) {
            s_can_counter_200ms = 0u;
            g_flag_can_200ms = true;
        }
    }

    if (htim->Instance == TIM5 && s_wake_armed) {
        s_counter_10us++;
        if (s_counter_10us >= BQ_WAKE_TICKS) {
            s_counter_10us = 0u;
            s_wake_armed = false;
            g_flag_bq_wake = true;
        }
    }
}


bool Timer_Check100msFlag(void)
{
    if (g_flag_100ms) {
        g_flag_100ms = false;
        return true;
    }
    return false;
}

bool Timer_Check200msFlag(void)
{
    if (g_flag_200ms) {
        g_flag_200ms = false;
        return true;
    }
    return false;
}

bool Timer_CheckCan100msFlag(void)
{
    if (g_flag_can_100ms) {
        g_flag_can_100ms = false;
        return true;
    }
    return false;
}

bool Timer_CheckCan200msFlag(void)
{
    if (g_flag_can_200ms) {
        g_flag_can_200ms = false;
        return true;
    }
    return false;
}

bool Timer5_CheckWakeFlag(void)
{
    if (g_flag_bq_wake) {
        g_flag_bq_wake = false;
        return true;
    }
    return false;
}

void Timer5_ArmWake(void)
{
    s_counter_10us = 0u;
    g_flag_bq_wake = false;
    s_wake_armed = true;
    __HAL_TIM_SET_COUNTER(&htim5, 0u);
    HAL_TIM_Base_Start_IT(&htim5);
}

void Timer5_DisarmWake(void)
{
    s_wake_armed = false;
    HAL_TIM_Base_Stop_IT(&htim5);
}

/* Debug: Show internal counter state */
void Timer_DebugPrintCounters(void)
{
    LOG_INFO("TIM6 Counter State:");
    LOG_INFO("  TIM6->CNT = %u (raw counter, should cycle 0-99)", __HAL_TIM_GET_COUNTER(&g_htim6));
    LOG_INFO("  s_isr_call_count = %lu (total TIM6 ISR calls since boot)", (unsigned long)s_isr_call_count);
    LOG_INFO("  s_counter_100ms = %u (should be 0-9)", s_counter_100ms);
    LOG_INFO("  s_counter_200ms = %u (should be 0-19)", s_counter_200ms);
    LOG_INFO("  g_flag_100ms = %u (set by ISR when counter hits 10)", g_flag_100ms);
    LOG_INFO("  g_flag_200ms = %u (set by ISR when counter hits 20)", g_flag_200ms);
    LOG_INFO("Expected: TIM6 fires every 10ms, so ~10 calls per 100ms, ~20 calls per 200ms");
}

uint32_t Timer_GetIsrCallCount(void)
{
    return s_isr_call_count;
}
