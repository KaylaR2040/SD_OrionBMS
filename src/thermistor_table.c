/*
 * ===========================================================================
 * File: thermistor_table.c
 * Description: Holds the thermistor lookup table and functions to convert
 *
 * ===========================================================================
 */

#include "master.h"

 static const ThermistorTableEntry_t s_temp_table[] = {
    {3813, -30}, {3798, -29}, {3782, -28}, {3765, -27}, {3747, -26},
    {3729, -25}, {3710, -24}, {3690, -23}, {3670, -22}, {3649, -21},
    {3627, -20}, {3605, -19}, {3581, -18}, {3557, -17}, {3532, -16},
    {3507, -15}, {3480, -14}, {3453, -13}, {3425, -12}, {3397, -11},
    {3368, -10}, {3337,  -9}, {3307,  -8}, {3275,  -7}, {3243,  -6},
    {3210,  -5}, {3177,  -4}, {3143,  -3}, {3108,  -2}, {3073,  -1},
    {3037,   0}, {3001,   1}, {2964,   2}, {2926,   3}, {2888,   4},
    {2850,   5}, {2812,   6}, {2773,   7}, {2733,   8}, {2694,   9},
    {2654,  10}, {2614,  11}, {2573,  12}, {2533,  13}, {2492,  14},
    {2451,  15}, {2411,  16}, {2370,  17}, {2329,  18}, {2289,  19},
    {2248,  20}, {2208,  21}, {2167,  22}, {2127,  23}, {2087,  24},
    {2048,  25}, {2008,  26}, {1969,  27}, {1930,  28}, {1892,  29},
    {1854,  30}, {1816,  31}, {1778,  32}, {1742,  33}, {1705,  34},
    {1669,  35}, {1634,  36}, {1599,  37}, {1564,  38}, {1530,  39},
    {1496,  40}, {1463,  41}, {1431,  42}, {1399,  43}, {1368,  44},
    {1337,  45}, {1307,  46}, {1277,  47}, {1248,  48}, {1219,  49},
    {1191,  50}, {1163,  51}, {1137,  52}, {1110,  53}, {1084,  54},
    {1059,  55}, {1034,  56}, {1010,  57}, { 986,  58}, { 963,  59},
    { 940,  60}, { 918,  61}, { 897,  62}, { 875,  63}, { 855,  64},
    { 834,  65}, { 815,  66}, { 795,  67}, { 777,  68}, { 758,  69},
    { 740,  70}, { 723,  71}, { 706,  72}, { 689,  73}, { 673,  74},
    { 657,  75}, { 642,  76}, { 627,  77}, { 612,  78}, { 598,  79},
    { 584,  80}, { 570,  81}, { 557,  82}, { 544,  83}, { 531,  84},
    { 519,  85}
};

int8_t Thermistor_ADCToTemp(uint16_t adc_value) {
    uint16_t low = 0;
    uint16_t high = THERMISTOR_TABLE_SIZE - 1;
    /* Max Clamp: if adc_value is above the highest entry, return the lowest temp; if below the lowest entry, return the highest temp. */
    if(adc_value >= s_temp_table[0].adc) {
        return s_temp_table[0].temp;
    }
    
    /* Min Clamp: if adc_value is below the lowest entry, return the highest temp; if above the highest entry, return the lowest temp. */
    if(adc_value <= s_temp_table[THERMISTOR_TABLE_SIZE - 1].adc) {
        return s_temp_table[THERMISTOR_TABLE_SIZE - 1].temp;
    }

    /* Binary search for the correct temperature range */
    while (high - low > 1) {
        uint16_t mid = (low + high) / 2;
        if (adc_value < s_temp_table[mid].adc) {
            low = mid;
        } else {
            high = mid;
        }
    }

    /* Return average of the two bracketing temperatures (rounds toward zero) */
    return (int8_t)((s_temp_table[high].temp + s_temp_table[low].temp) / 2);
}
