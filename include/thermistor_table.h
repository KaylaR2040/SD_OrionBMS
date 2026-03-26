/*
 * ===========================================================================
 * File: thermistor_table.h
 * Description: Holds the thermistor lookup table and functions to convert
 *
 * ===========================================================================
 */

#ifndef THERMISTOR_TABLE_H
#define THERMISTOR_TABLE_H

#include <stdint.h>

#define THERMISTOR_TABLE_SIZE  115U

typedef struct {
    uint16_t adc;
    int8_t   temp;
} ThermistorTableEntry_t;

/*
 * Convert ADC value to temperature using lookup table + interpolation
 * Uses binary search to find nearest entries, then linear interpolation
 */
int8_t Thermistor_ADCToTemp(uint16_t adc_value);

#endif
