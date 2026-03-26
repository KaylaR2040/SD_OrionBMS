/*
 * ===========================================================================
 * File: stm32g4xx_hal_msp.c
 * Description: HAL MSP init/de-init hooks configuring clocks, DMA, and interrupts.
 *
 * Notes:
 *   - Called by HAL; keep peripheral-specific setup in their sections.
 * ===========================================================================
 */

#include "master.h"

/* Global MSP init: enable core clocks and power interface */
void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

/* Configure ADC1 GPIO clocks/pins for analog input sampling */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  if (hadc->Instance == ADC1) {
    __HAL_RCC_ADC12_CLK_ENABLE();

    /* GPIO clocks for analog inputs */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;

    /* ADC1: PA0=IN1, PA1=IN2 */
    g.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOA, &g);

    /* ADC1: PB14=IN5, PB12=IN11, PB11=IN14, PB0=IN15 */
    g.Pin = GPIO_PIN_14 | GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_0;
    HAL_GPIO_Init(GPIOB, &g);

    /* ADC1: PC0=IN6, PC1=IN7, PC2=IN8, PC3=IN9 */
    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    HAL_GPIO_Init(GPIOC, &g);
  }
}

/* Return ADC1 GPIO and clocks to reset state */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
  if (hadc->Instance == ADC1) {
    __HAL_RCC_ADC12_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_1);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_14 | GPIO_PIN_12 | GPIO_PIN_11 | GPIO_PIN_0);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
  }
}

/* Configure FDCAN1 pins, clocks, and NVIC priority */
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef* hfdcan)
{
  if (hfdcan->Instance == FDCAN1) {
    __HAL_RCC_FDCAN_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    /* FDCAN1: PA11=RX, AF9; PA12=TX, AF9 */
    g.Pin = GPIO_PIN_11;
    g.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOA, &g);

    g.Pin = GPIO_PIN_12;
    g.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOA, &g);

    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    HAL_NVIC_SetPriority(FDCAN1_IT1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT1_IRQn);
  }
}

/* Disable FDCAN1 resources when no longer used */
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef* hfdcan)
{
  if (hfdcan->Instance == FDCAN1) {
    __HAL_RCC_FDCAN_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
    HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
    HAL_NVIC_DisableIRQ(FDCAN1_IT1_IRQn);
  }
}

/* Configure USART1/LPUART1/USART2 pins and NVIC for UART channels */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  if (huart->Instance == USART1) {
    /* USART1 (BQ79616) = PC4 (TX), PC5 (RX), AF7  */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_4 | GPIO_PIN_5; /* PC4 TX, PC5 RX */
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOC, &g);

    HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
  else if (huart->Instance == LPUART1) {
    /* LPUART1 (optional USB-UART logging) = PA2 (TX), PA3 (RX), AF12 */
    __HAL_RCC_LPUART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_2 | GPIO_PIN_3; /* PA2 TX, PA3 RX */
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF12_LPUART1;
    HAL_GPIO_Init(GPIOA, &g);

    HAL_NVIC_SetPriority(LPUART1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(LPUART1_IRQn);
  }
  else if (huart->Instance == USART2) {
    /* USART2 (BQ79616) = PA2 (TX), PA3 (RX), AF7 — blocking transport */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_2 | GPIO_PIN_3; /* PA2 TX, PA3 RX */
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);
  }
}
