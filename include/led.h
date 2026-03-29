/*
 * ===========================================================================
 * File: led.h
 * Description: GPIO-driven LED control APIs for board status indications.
 *
 * Notes:
 *   - LED wiring defined in main.h; functions are non-blocking except Pulse/Flash.
 * ===========================================================================
 */

#ifndef LED_H
#define LED_H

// C standard library
#include <stdint.h>

// Project headers
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_ID_LD1,
    LED_ID_LD2,
    LED_ID_LD3,
    LED_ID_COUNT
} led_id_t;

#define ERROR_LED LED_ID_LD3

/* Default flash duration for LED_Flash (ms) */
#ifndef LED_FLASH_DURATION_MS
#define LED_FLASH_DURATION_MS 100U
#endif

void LED_Init(void);
void LED_On(led_id_t id);
void LED_Off(led_id_t id);
void LED_Toggle(led_id_t id);
void LED_Flash(led_id_t id);
void LED_Pulse(led_id_t id, uint32_t duration_ms);
void LED_All_On(void);
void LED_All_Off(void);
void LED_All_Toggle(void);
void LED_All_Flash(void);
void LED_All_Pulse(uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
