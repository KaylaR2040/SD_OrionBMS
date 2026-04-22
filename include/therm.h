/*
 * ===========================================================================
 * File: therm.h
 * Description: STM32G4 internal ADC API for reading Thermistors
 *
 * Notes:
 *   - The BQ79616 external ADC for reading Cell Voltages is handled separately in volt.h
 */

#ifndef THERM_H
#define THERM_H

/* STM32 HAL */
#include "stm32g4xx_hal.h"

/* STM32G4 internal ADC interface for thermistor measurements. */

#ifdef __cplusplus
extern "C" {
#endif
/* --- Channel and ADC mapping --- */
#define THERM_APP_CHANNEL_COUNT 10U

typedef struct {
    uint32_t    adc_channel;   /* HAL ADC channel constant */
    const char *software_pin;  /* GPIO port/pin label */
    const char *physical_pin;  /* Package pin number */
} therm_channel_pin_t;

/* --- ADC conversion and scaling constants --- */
#define THERM_CONVERSION_TIMEOUT_MS  10U
#define THERM_SAMPLE_TIME            ADC_SAMPLETIME_247CYCLES_5
#define THERM_SETTLE_DELAY_ITERS     64U
#define THERM_MAX_COUNTS 4095U
#define THERM_REF_MV     3300U
#define THERM_MV_PER_VOLT 1000U

extern ADC_HandleTypeDef hadc1;
extern const therm_channel_pin_t therm_channel_pins[THERM_APP_CHANNEL_COUNT];

void Therm_App_Init(void);
uint16_t Therm_App_ReadChannel(uint32_t channel);
void Therm_App_SampleAll(uint16_t out_vals[THERM_APP_CHANNEL_COUNT]);
void Therm_LogCachedSnapshot(void);
void Therm_ServiceTask(void);

#ifdef __cplusplus
}
#endif

#endif /* THERM_H */
