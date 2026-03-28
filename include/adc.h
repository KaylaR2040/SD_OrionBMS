/*
 * ===========================================================================
 * File: adc.h
 * Description: STM32G4 internal ADC API for reading Thermistors
 *
 * Notes:
 *   - The BQ79616 external ADC for reading Voltage Taps is handled separately in bq79616.c
 */

#ifndef ADC_H
#define ADC_H

/* STM32G4 internal ADC interface (BQ79616 ADC handled in bq79616.c). */

// Project headers
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define ADC_APP_CHANNEL_COUNT 10U

/* Shared ADC constants for voltage/count conversions. */
#define ADC_MAX_COUNTS 4095U
#define ADC_REF_MV     3300U

extern ADC_HandleTypeDef hadc1;

void ADC_App_Init(void);
uint16_t ADC_App_ReadChannel(uint32_t channel);
void ADC_App_SampleAll(uint16_t out_vals[ADC_APP_CHANNEL_COUNT]);
void ADC_App_LogSnapshot(void);
void ADC_ServiceTask(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_H */
