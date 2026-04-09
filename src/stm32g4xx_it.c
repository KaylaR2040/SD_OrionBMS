/*
 * ===========================================================================
 * File: stm32g4xx_it.c
 * Description: Interrupt service routines for STM32G4 peripheral/CPU exceptions.
 *
 * Notes:
 *   - Generated layout retained; handlers may be customized for application needs.
 * ===========================================================================
 */

#include "master.h"

/* Non-maskable interrupt handler (left minimal for now) */
void NMI_Handler(void) {}

/* Trap unrecoverable CPU faults */
void HardFault_Handler(void) {
    while (1) {}
}

/* Supervisor call handler (unused) */
void SVC_Handler(void) {}

/* PendSV handler (unused in this project) */
void PendSV_Handler(void) {}

/* Tick interrupt increments the HAL time base */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* Route FDCAN interrupt line 0 to HAL handler */
void FDCAN1_IT0_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&g_fdcan1);
}

/* Route FDCAN interrupt line 1 to HAL handler */
void FDCAN1_IT1_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&g_fdcan1);
}

/* UART interrupt handler used by logging interface (USART2 -> PA2/PA3) */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&uart_stlink);
}

/* TIM6 global interrupt handler */
extern TIM_HandleTypeDef g_htim6;
extern TIM_HandleTypeDef g_htim7;
extern TIM_HandleTypeDef htim5;

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_htim6);
}

void TIM7_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_htim7);
}

void TIM5_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim5);
}
