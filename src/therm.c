/*
 * ===========================================================================
 * File: therm.c
 * Description: STM32G4 internal ADC helpers for reading Thermistors
 *
 * Notes:
 * ===========================================================================
 */

// Master project header
#include "master.h"

#define THERM_CONVERSION_TIMEOUT_MS  10U
#define THERM_SAMPLE_TIME            ADC_SAMPLETIME_247CYCLES_5
#define THERM_SETTLE_DELAY_ITERS     64U

static void Therm_DelayForSettling(void);
static HAL_StatusTypeDef Therm_PerformConversion(uint16_t *result);

const therm_channel_pin_t therm_channel_pins[THERM_APP_CHANNEL_COUNT] = {
    {ADC_CHANNEL_1,  "PA0",  "12"}, // Thermistor  1: ADC1_IN1
    {ADC_CHANNEL_2,  "PA1",  "13"}, // Thermistor  2: ADC1_IN2
    {ADC_CHANNEL_11, "PB12", "14"}, // Thermistor  3: ADC1_IN11
    {ADC_CHANNEL_14, "PB11", "15"}, // Thermistor  4: ADC1_IN14
    {ADC_CHANNEL_5,  "PB14", "16"}, // Thermistor  5: ADC1_IN5
    {ADC_CHANNEL_6,  "PC0",  "17"}, // Thermistor  6: ADC1_IN6
    {ADC_CHANNEL_7,  "PC1",  "18"}, // Thermistor  7: ADC1_IN7
    {ADC_CHANNEL_8,  "PC2",  "19"}, // Thermistor  8: ADC1_IN8
    {ADC_CHANNEL_9,  "PC3",  "20"}, // Thermistor  9: ADC1_IN9
    {ADC_CHANNEL_15, "PB0",  "21"}  // Thermistor 10: ADC1_IN15
};

/*
 * Thermistor ADC sampling math:
 *   - Sample time = 247.5 cycles and conversion adds 12.5 cycles.
 *   - Total cycles per sample = 260 cycles (longer window improves accuracy).
 *   - Asynchronous ADC clock = 170 MHz (PLL output with DIV1 prescaler).
 *   - Effective sample frequency ~ 170e6 / 260 ~ 653.8 kHz (~1.53 us / sample).
 */

/** Primary ADC handle used across the application. */
ADC_HandleTypeDef hadc1;

/* Sampling happens immediately and continuously in the service loop */
#define THERM_SAMPLE_PERIOD_MS      100U



/* Initialize the STM32G4 on-chip ADC for thermistor measurements */
void Therm_App_Init(void)
{
    __HAL_RCC_ADC12_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK) Error_Handler();
}

/* Blocking read for a single thermistor channel on the STM32 ADC */
uint16_t Therm_App_ReadChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = THERM_SAMPLE_TIME;
#ifdef ADC_SINGLE_ENDED
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
#endif
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) return 0;
    Therm_DelayForSettling();
    /* Discard the first conversion after switching channels to bleed off charge. */
    (void)Therm_PerformConversion(NULL);

    uint16_t val = 0U;
    if (Therm_PerformConversion(&val) != HAL_OK) {
        return 0U;
    }
    return val;
}

/* Iterate across the thermistor channels so higher layers get consistent data */
void Therm_App_SampleAll(uint16_t out_vals[THERM_APP_CHANNEL_COUNT])
{
    // const uint32_t chans[THERM_APP_CHANNEL_COUNT] = {
    //     ADC_CHANNEL_1,  // PA0  = ADC1_IN1  = Thermistor 1
    //     ADC_CHANNEL_2,  // PA1  = ADC1_IN2  = Thermistor 2
    //     ADC_CHANNEL_11, // PB12 = ADC1_IN11 = Thermistor 3
    //     ADC_CHANNEL_14, // PB11 = ADC1_IN14 = Thermistor 4
    //     ADC_CHANNEL_5,  // PB14 = ADC1_IN5  = Thermistor 5
    //     ADC_CHANNEL_6,  // PC0  = ADC1_IN6  = Thermistor 6
    //     ADC_CHANNEL_7,  // PC1  = ADC1_IN7  = Thermistor 7
    //     ADC_CHANNEL_8,  // PC2  = ADC1_IN8  = Thermistor 8
    //     ADC_CHANNEL_9,  // PC3  = ADC1_IN9  = Thermistor 9
    //     ADC_CHANNEL_15  // PB0  = ADC1_IN15 = Thermistor 10
    // };

    if (!out_vals) return;
    for (size_t i = 0; i < THERM_APP_CHANNEL_COUNT; ++i) {
        out_vals[i] = Therm_App_ReadChannel(therm_channel_pins[i].adc_channel);
    }
}

/* Emit a log snapshot of the thermistor readings so the CLI can visualize sensor values */
void Therm_App_LogSnapshot(void)
{
    //const char *channel_pins[THERM_APP_CHANNEL_COUNT] = {
    //     "PA0", "PA1", "PB14", "PC0", "PC1", "PC2", "PC3", "PB12", "PB11", "PB0"
    // };
    uint16_t samples[THERM_APP_CHANNEL_COUNT] = {0};


    /* Capture one full sweep so the log shows a consistent instant in time. */
    Therm_App_SampleAll(samples);

    /* Update the CAN context thermistor values for transmission. */
    for (uint8_t i = 0; i < THERM_APP_CHANNEL_COUNT && i < MAX_THERMISTORS; i++) {
        g_can_ctx.thermistors.thermistor_adc_values[i] = samples[i];
    }
    
    g_can_ctx.thermistors.num_active = THERM_APP_CHANNEL_COUNT;

    /* Header label separates thermistor dump from prior task output. */
    LOG_INFO("Thermistor Snapshot:");
    for (size_t i = 0; i < THERM_APP_CHANNEL_COUNT; ++i) {
        const uint32_t millivolts =
            ((uint32_t)samples[i] * (uint32_t)THERM_REF_MV) / (uint32_t)THERM_MAX_COUNTS;
        const uint32_t volts_whole = millivolts / 1000U;
        const uint32_t volts_frac = millivolts % 1000U;
        /* Each channel line shows ID, software pin, chip pin, and computed voltage in volts. */
        LOG_INFO("  %s%-2lu%s %-6s (Pin %-2s) %5lu -> %s%lu.%03lu V%s",
            LOG_COLOR_FIELD,
            (unsigned long)(i + 1U),
            LOG_COLOR_RESET,
            therm_channel_pins[i].software_pin,
            therm_channel_pins[i].physical_pin,
            (unsigned long)samples[i],
            LOG_COLOR_VALUE,
            (unsigned long)volts_whole,
            (unsigned long)volts_frac,
            LOG_COLOR_RESET);
    }
    /* Spacer to separate thermistor output from subsequent task logs. */
    LOG_INFO("\n");
}


void Therm_LogCachedSnapshot(void)
{
    const uint8_t count = g_can_ctx.thermistors.num_active;

    if (count == 0U) {
        LOG_INFO("Thermistor Cache: no active thermistors");
        return;
    }

    LOG_INFO("Thermistor Cache:");
    for (uint8_t i = 0U; i < count && i < THERM_APP_CHANNEL_COUNT; ++i) {
        const uint16_t sample = g_can_ctx.thermistors.thermistor_adc_values[i];
        const uint32_t millivolts =
            ((uint32_t)sample * (uint32_t)THERM_REF_MV) / (uint32_t)THERM_MAX_COUNTS;
        const uint32_t volts_whole = millivolts / 1000U;
        const uint32_t volts_frac  = millivolts % 1000U;
        const int temp_c = (int)Thermistor_ADCToTemp(sample);

        LOG_INFO("  %s%-2lu%s %-6s (Pin %-2s) ch%-3lu %5u -> %s%lu.%03lu V%s %dC",
             LOG_COLOR_FIELD,
             (unsigned long)(i + 1U),
             LOG_COLOR_RESET,
             therm_channel_pins[i].software_pin,
             therm_channel_pins[i].physical_pin,
             (unsigned long)therm_channel_pins[i].adc_channel,
             (unsigned)sample,
             LOG_COLOR_VALUE,
             (unsigned long)volts_whole,
             (unsigned long)volts_frac,
             LOG_COLOR_RESET,
             temp_c);
    }

    LOG_INFO("\n");
}

/* Execute the thermistor periodic task that toggles LEDs and logs samples */
/* Execute the thermistor periodic task - samples continuously, logs on offset 0 interrupt */
void Therm_ServiceTask(void)
{
    /* Check if thermistor subsystem is active */
    if (therm_status == FAILED) {
        LED_On(THERM_LED);  /* Light up THERM_LED on THERM failure */
        return;
    }

    /* Sample immediately and continuously to feed CAN cache with fresh data */
    uint16_t samples[THERM_APP_CHANNEL_COUNT] = {0};
    Therm_App_SampleAll(samples);

        for (uint8_t i = 0U; i < THERM_APP_CHANNEL_COUNT && i < MAX_THERMISTORS; i++) {
            g_can_ctx.thermistors.thermistor_adc_values[i] = samples[i];
        }
        if (THERM_APP_CHANNEL_COUNT < MAX_THERMISTORS) {
            g_can_ctx.thermistors.num_active = THERM_APP_CHANNEL_COUNT;
        } else {
            g_can_ctx.thermistors.num_active = MAX_THERMISTORS;
        }

        ConvertAllThermistors(g_can_ctx.thermistors.thermistor_adc_values,
                              g_can_ctx.thermistors.num_active);

    /* Logging only happens when interrupt flag fires (offset 0: every 1s, 4s, 7s...) */
    if (Timer_CheckLogOffset0Flag()) {
        Therm_App_LogSnapshot();
        // Therm_LogCachedSnapshot();
    }
}

/* Burn cycles so the sampling cap settles after a channel switch */
static void Therm_DelayForSettling(void)
{
    /*
     * The thermistor ADC sample-and-hold capacitor retains some charge from the prior
     * channel. This short NOP loop burns roughly
     *   THERM_SETTLE_DELAY_ITERS / SystemCoreClock seconds
     * (~0.38 us with 64 iters at 170 MHz) so the capacitor discharges through
     * the newly selected channel before we start a conversion.
     */
    for (volatile uint32_t i = 0; i < THERM_SETTLE_DELAY_ITERS; ++i) {
        __NOP();
    }
}

/* Start a single thermistor ADC conversion and optionally return the result */
static HAL_StatusTypeDef Therm_PerformConversion(uint16_t *result)
{
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_ADC_PollForConversion(&hadc1, THERM_CONVERSION_TIMEOUT_MS) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return HAL_ERROR;
    }
    if (result) {
        *result = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);
    return HAL_OK;
}
