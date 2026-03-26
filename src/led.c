/*
 * ===========================================================================
 * File: led.c
 * Description: GPIO-backed LED helpers that obey CMU coding practices.
 *
 * Notes:
 *   - Timing-sensitive pulses use HAL_Delay; other calls are non-blocking.
 * ===========================================================================
 */

#include "master.h"

/** Hardware mapping that binds a logical LED ID to a GPIO port/pin. */
typedef struct {
    GPIO_TypeDef *port;
    uint32_t      pin;
} led_hw_t;

/** Lookup table describing every LED instance exposed to the application. */
static const led_hw_t g_led_hw[LED_ID_COUNT] = {
    [LED_ID_LD1] = { LED1_GPIO_Port, LED1_Pin },
    [LED_ID_LD2] = { LED2_GPIO_Port, LED2_Pin },
    [LED_ID_LD3] = { LED3_GPIO_Port, LED3_Pin }
};

/* Resolve LED hardware mapping or NULL on invalid id */
static const led_hw_t *LED_Get_Hardware(led_id_t id)
{
    if (id >= LED_ID_COUNT) return NULL;
    return &g_led_hw[id];
}

/* Ensure the corresponding GPIO clock is enabled (Rule 4.1) */
static void LED_Enable_Clock(GPIO_TypeDef *port)
{
    if (port == GPIOA) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    } else if (port == GPIOB) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    } else if (port == GPIOC) {
        LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    }
}

/* Configure a LED pin using LL drivers */
static void LED_Config_Pin(GPIO_TypeDef *port, uint32_t pin)
{
    if (!port || pin == 0u) return;
    LED_Enable_Clock(port);

    LL_GPIO_SetPinMode(port, pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinPull(port, pin, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinOutputType(port, pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(port, pin, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_ResetOutputPin(port, pin);
}

/* Initialize all LEDs so they are ready for use */
void LED_Init(void)
{
    for (size_t i = 0; i < LED_ID_COUNT; ++i) {
        const led_hw_t *hw = &g_led_hw[i];
        LED_Config_Pin(hw->port, hw->pin);
    }
}

/* Drive a LED high */
void LED_On(led_id_t id)
{
    const led_hw_t *hw = LED_Get_Hardware(id);
    if (!hw) return;
    LL_GPIO_SetOutputPin(hw->port, hw->pin);
}

/* Drive a LED low */
void LED_Off(led_id_t id)
{
    const led_hw_t *hw = LED_Get_Hardware(id);
    if (!hw) return;
    LL_GPIO_ResetOutputPin(hw->port, hw->pin);
}

/* Toggle a LED state */
void LED_Toggle(led_id_t id)
{
    const led_hw_t *hw = LED_Get_Hardware(id);
    if (!hw) return;
    LL_GPIO_TogglePin(hw->port, hw->pin);
}

/* Pulse a LED for the requested duration (ms) */
void LED_Pulse(led_id_t id, uint32_t duration_ms)
{
    const led_hw_t *hw = LED_Get_Hardware(id);
    if (!hw) return;

    LL_GPIO_SetOutputPin(hw->port, hw->pin);
    HAL_Delay(duration_ms);
    LL_GPIO_ResetOutputPin(hw->port, hw->pin);
}

/* Flash a LED for a short, predefined duration */
void LED_Flash(led_id_t id)
{
    LED_Pulse(id, LED_FLASH_DURATION_MS);
}

/* Turn on all LEDs */
void LED_All_On(void)
{
    for (size_t i = 0; i < LED_ID_COUNT; ++i) {
        LED_On((led_id_t)i);
    }
}

/* Turn off all LEDs */
void LED_All_Off(void)
{
    for (size_t i = 0; i < LED_ID_COUNT; ++i) {
        LED_Off((led_id_t)i);
    }
}

/* Toggle every LED */
void LED_All_Toggle(void)
{
    for (size_t i = 0; i < LED_ID_COUNT; ++i) {
        LED_Toggle((led_id_t)i);
    }
}

/* Flash every LED for the default interval */
void LED_All_Flash(void)
{
    LED_All_Pulse(LED_FLASH_DURATION_MS);
}

/* Pulse every LED for the requested interval */
void LED_All_Pulse(uint32_t duration_ms)
{
    for (size_t i = 0; i < LED_ID_COUNT; ++i) {
        LED_On((led_id_t)i);
    }

    HAL_Delay(duration_ms);

    for (size_t i = 0; i < LED_ID_COUNT; ++i) {
        LED_Off((led_id_t)i);
    }
}
