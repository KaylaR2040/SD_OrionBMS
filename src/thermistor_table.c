/*
 * ===========================================================================
 * File: thermistor_table.c
 * Description: Holds the thermistor lookup table and functions to convert
 * 
 * ===========================================================================
 */

#include "master.h"


static const ThermistorTableEntry_t s_temp_table[] = {
    {  88,  85}, {  90,  84}, {  92,  83}, {  95,  82}, {  97,  81},
    {  99,  80}, { 102,  79}, { 104,  78}, { 107,  77}, { 109,  76},
    { 112,  75}, { 114,  74}, { 117,  73}, { 120,  72}, { 123,  71},
    { 126,  70}, { 129,  69}, { 132,  68}, { 135,  67}, { 139,  66},
    { 142,  65}, { 145,  64}, { 149,  63}, { 152,  62}, { 156,  61},
    { 160,  60}, { 163,  59}, { 167,  58}, { 171,  57}, { 176,  56},
    { 180,  55}, { 184,  54}, { 189,  53}, { 193,  52}, { 198,  51},
    { 202,  50}, { 207,  49}, { 212,  48}, { 217,  47}, { 222,  46},
    { 227,  45}, { 233,  44}, { 238,  43}, { 243,  42}, { 249,  41},
    { 254,  40}, { 260,  39}, { 266,  38}, { 272,  37}, { 278,  36},
    { 284,  35}, { 290,  34}, { 296,  33}, { 302,  32}, { 309,  31},
    { 315,  30}, { 322,  29}, { 328,  28}, { 335,  27}, { 341,  26},
    { 348,  25}, { 355,  24}, { 361,  23}, { 368,  22}, { 375,  21},
    { 382,  20}, { 389,  19}, { 396,  18}, { 403,  17}, { 410,  16},
    { 417,  15}, { 424,  14}, { 431,  13}, { 438,  12}, { 444,  11},
    { 451,  10}, { 458,   9}, { 465,   8}, { 471,   7}, { 478,   6},
    { 484,   5}, { 491,   4}, { 497,   3}, { 504,   2}, { 510,   1},
    { 516,   0}, { 522,  -1}, { 528,  -2}, { 534,  -3}, { 540,  -4},
    { 546,  -5}, { 551,  -6}, { 557,  -7}, { 562,  -8}, { 568,  -9},
    { 573, -10}, { 578, -11}, { 582, -12}, { 587, -13}, { 592, -14},
    { 596, -15}, { 601, -16}, { 605, -17}, { 610, -18}, { 614, -19},
    { 618, -20}, { 621, -21}, { 625, -22}, { 629, -23}, { 632, -24},
    { 635, -25}, { 639, -26}, { 642, -27}, { 645, -28}, { 648, -29},
    { 650, -30}
};

int8_t Thermistor_ADCToTemp(uint16_t adc_value) {
    uint16_t low = 0;
    uint16_t high = THERMISTOR_TABLE_SIZE - 1;

    /* Clamp with flipped polarity: low counts = hottest, high counts = coldest. */
    if (adc_value <= s_temp_table[0].adc) {
        return s_temp_table[0].temp;
    }

    if (adc_value >= s_temp_table[THERMISTOR_TABLE_SIZE - 1].adc) {
        return s_temp_table[THERMISTOR_TABLE_SIZE - 1].temp;
    }

    /* Binary search for the correct temperature range */
    while (high - low > 1) {
        uint16_t mid = (low + high) / 2;
        /* Table ADC values increase as temperature decreases. Use ascending search. */
        if (adc_value < s_temp_table[mid].adc) {
            high = mid;
        } else {
            low = mid;
        }
    }

    /* Return average of the two bracketing temperatures (rounds toward zero) */
    return (int8_t)((s_temp_table[high].temp + s_temp_table[low].temp) / 2);
}
