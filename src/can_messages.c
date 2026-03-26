/*
 * File: can_messages.c
 * Description: CAN message encoding for thermistor modules and Orion BMS
 TODO: L-180 CAN for voltage ADC 
 TODO: L-180 CAN for temperature ADC
 TODO: Write debugging info for L-180
 */

#include "master.h"

#define THERM_HOT_C 65

ThermistorCache_t s_cache = {0};

/* Module configuration (source address 0x80 = first Orion thermistor module) */
static const ThermJ1939ClaimMsg_t s_default_claim = {
    .unique_id = { ORION_CLAIM_UNIQUE_ID0, ORION_CLAIM_UNIQUE_ID1, ORION_CLAIM_UNIQUE_ID2 },
    .bms_target = ORION_BMS_TARGET_ADDR,
    .thermistor_module = ORION_THERM_MODULE_INDEX,
    .constant = { 0x40, 0x1E, 0x90 }
};

static uint16_t CAN_ThermMask(uint8_t therm_index)
{
    return (uint16_t)(1u << therm_index);
}

static uint8_t CAN_PackTemp(int8_t temp_c)
{
    return (uint8_t)temp_c;
}


static uint8_t CAN_ComputeBmsChecksum(const uint8_t *payload)
{
    uint16_t sum = 0x39u + 8u;

    for (uint8_t i = 0U; i < 7U; i++) {
        sum += (uint8_t)payload[i];
    }

    return (uint8_t)sum;
}

static uint16_t CAN_GlobalThermistorId(uint8_t local_index)
{
    return (uint16_t)((uint16_t)s_default_claim.thermistor_module * 80u) + (uint16_t)local_index;
}

static bool CAN_ThermistorFaulted(uint8_t therm_index)
{
    if (therm_index >= MAX_THERMISTORS) {
        return false;
    }
    return (s_cache.fault_mask & CAN_ThermMask(therm_index)) != 0u;
}


/* -------------------------------------------------------------------------- */
/* ADC to Temperature Conversion                                              */
/* -------------------------------------------------------------------------- */

void ConvertAllThermistors(const uint16_t *adc_values, uint8_t count)
{
    if (!adc_values) {
        return;
    }

    if (!CAN_Debug_IsEnabled()) {
        CAN_Debug_Clear();
    }

    if (count > MAX_THERMISTORS) {
        s_cache.count = MAX_THERMISTORS;
    } else {
        s_cache.count = count;
    }
    s_cache.valid_count = 0u;
    s_cache.min_temp = 127;
    s_cache.max_temp = -128;
    /* Orion bytes 6/7 report the module's loaded thermistor range, not hottest/coldest IDs. */
    s_cache.min_id = 0U;
    if (s_cache.count > 0U) {
        s_cache.max_id = (uint8_t)(s_cache.count - 1U);
    } else {
        s_cache.max_id = 0U;
    }
    s_cache.fault_mask = 0u;
    s_cache.module_fault = false;

    int32_t sum = 0;
    for (uint8_t i = 0; i < s_cache.count; i++) {
        const uint16_t adc_raw = adc_values[i];
        int8_t temp_c = Thermistor_ADCToTemp(adc_raw);
        const uint16_t therm_mask = CAN_ThermMask(i);
        bool therm_fault = false;
        const bool sensor_fault_forced = CAN_Debug_IsSensorFaulted(i);

        int8_t override_temp = 0;
        const bool has_override = CAN_Debug_GetTempOverride(i, &override_temp);
        const bool debug_override_active = CAN_Debug_IsEnabled() && has_override;

        if (debug_override_active) {
            temp_c = override_temp;
            therm_fault = false; /* ignore ADC/range faults when we explicitly override */
        } else {
            if (adc_raw >= ORION_THERM_ADC_OPEN_FAULT_THRESHOLD ||
                adc_raw <= ORION_THERM_ADC_SHORT_FAULT_THRESHOLD ||
                temp_c < ORION_THERM_MIN_VALID_C ||
                temp_c > ORION_THERM_MAX_VALID_C) {
                therm_fault = true;
            }
        }

        if (temp_c > THERM_HOT_C) {
            therm_fault = true;
            LOG_WARN("Thermistor %u too hot: %dC", i, (int)temp_c);
        }

        if (sensor_fault_forced) {
            therm_fault = true;
        }

        s_cache.adc_raw[i] = adc_raw;
        s_cache.temps[i] = temp_c;

        if (therm_fault) {
            s_cache.fault_mask |= therm_mask;
            s_cache.module_fault = true;
            LOG_WARN("Thermistor %u fault set (mask=0x%04X)", i, (unsigned)s_cache.fault_mask);
            continue;
        }

        if (temp_c < s_cache.min_temp) {
            s_cache.min_temp = temp_c;
        }
        if (temp_c > s_cache.max_temp) {
            s_cache.max_temp = temp_c;
        }
        sum += temp_c;
        s_cache.valid_count++;
    }

    if (s_cache.valid_count > 0U) {
        s_cache.avg_temp = (int8_t)(sum / (int32_t)s_cache.valid_count);
    } else {
        s_cache.min_temp = 0;
        s_cache.max_temp = 0;
        s_cache.avg_temp = 0;
        s_cache.min_id = 0U;
        if (s_cache.count > 0U) {
            s_cache.max_id = (uint8_t)(s_cache.count - 1U);
        } else {
            s_cache.max_id = 0U;
        }
    }

    s_cache.last_update_ms = HAL_GetTick();

    if (s_cache.max_temp > THERM_HOT_C) {
        LOG_WARN("PACK TOO HOT: max=%dC id=%u", (int)s_cache.max_temp, (unsigned)s_cache.max_id);
    }

    if (s_cache.fault_mask != 0u) {
        LOG_WARN("Thermistor fault mask=0x%04X valid=%u", (unsigned)s_cache.fault_mask, (unsigned)s_cache.valid_count);
    }

    CAN_Debug_UpdateCoreState(s_default_claim.thermistor_module,
                              ORION_THERM_SOURCE_ADDR,
                              ORION_BMS_TARGET_ADDR,
                              s_cache.count,
                              s_cache.valid_count,
                              s_cache.module_fault,
                              s_cache.fault_mask,
                              s_cache.min_temp,
                              s_cache.max_temp,
                              s_cache.avg_temp,
                              s_cache.max_id,
                              s_cache.min_id);
}

/* -------------------------------------------------------------------------- */
/* Encode J1939 Address Claim (0x18EEFF80)                                    */
/* -------------------------------------------------------------------------- */

void EncodeJ1939Claim(uint8_t *payload)
{
    payload[0] = s_default_claim.unique_id[0];              /* 0xF3 */
    payload[1] = s_default_claim.unique_id[1];              /* 0x00 */
    payload[2] = s_default_claim.unique_id[2];              /* 0x80 */
    payload[3] = s_default_claim.bms_target;                /* 0xF3 */
    payload[4] = (uint8_t)(s_default_claim.thermistor_module << 3); /* module index << 3 */
    payload[5] = s_default_claim.constant[0];               /* 0x40 */
    payload[6] = s_default_claim.constant[1];               /* 0x1E */
    payload[7] = s_default_claim.constant[2];               /* 0x90 */
}

/* -------------------------------------------------------------------------- */
/* Encode BMS Broadcast (0x1839F380)                                          */
/* Per spec:                                                                  */
/* Byte 0: Thermistor Module Number                                           */
/* Byte 1: Lowest temp (int8_t)                                               */
/* Byte 2: Highest temp (int8_t)                                              */
/* Byte 3: Average temp (int8_t)                                              */
/* Byte 4: Number of thermistors enabled (bit 7 = fault)                      */
/* Byte 5: Highest thermistor ID on module (zero-based)                       */
/* Byte 6: Lowest thermistor ID on module (zero-based)                        */
/* Byte 8: Checksum (sum of all bytes + 0x39 + length)                        */
/* -------------------------------------------------------------------------- */

void EncodeBMSBroadcast(uint8_t *payload)
{
    const uint8_t enabled = (uint8_t)(s_cache.valid_count & 0x7FU);

    payload[0] = s_default_claim.thermistor_module;   /* Module number (zero-based) */
    payload[1] = CAN_PackTemp(s_cache.min_temp);       /* Lowest temp (int8_t) */
    payload[2] = CAN_PackTemp(s_cache.max_temp);       /* Highest temp (int8_t) */
    payload[3] = CAN_PackTemp(s_cache.avg_temp);       /* Average temp (int8_t) */
    uint8_t module_fault_flag = 0u;
    if (s_cache.module_fault) {
        module_fault_flag = 0x80u;
    }
    payload[4] = (uint8_t)(enabled | module_fault_flag);
    payload[5] = s_cache.max_id;                       /* Highest loaded thermistor ID on module */
    payload[6] = s_cache.min_id;                       /* Lowest loaded thermistor ID on module */
    payload[7] = CAN_ComputeBmsChecksum(payload);
}

/* -------------------------------------------------------------------------- */
/* Encode General Broadcast (0x1838F380)                                      */
/* Per spec and Orion multi-module support:                                   */
/* Byte 0-1: Global thermistor ID (relative to all configured modules)        */
/* Byte 2: Thermistor value (int8_t, 1C units)                                */
/* Byte 3: Local thermistor ID, bit7 = this thermistor fault                  */
/* Byte 4: Lowest valid thermistor value on module                            */
/* Byte 5: Highest valid thermistor value on module                           */
/* Byte 6: Highest thermistor ID on module (zero-based)                       */
/* Byte 7: Lowest thermistor ID on module (zero-based)                        */
/* -------------------------------------------------------------------------- */

void EncodeGeneralBroadcast(uint8_t therm_index, uint8_t *payload)
{
    uint8_t capped_idx = 0U;
    if (therm_index < s_cache.count) {
        capped_idx = therm_index;
    }

    int8_t this_temp = 0;
    if (s_cache.count > 0U && therm_index < s_cache.count) {
        this_temp = s_cache.temps[therm_index];
    }

    /* Up to 10 Orion thermistor modules may be configured, so use 16-bit global ID. */
    const uint16_t global_id = CAN_GlobalThermistorId(capped_idx);
    const bool therm_fault = CAN_ThermistorFaulted(capped_idx);

    payload[0] = (uint8_t)(global_id & 0x00FFu);      /* Global ID low byte */
    payload[1] = (uint8_t)((global_id >> 8U) & 0x00FFu);/* Global ID high byte */
    payload[2] = CAN_PackTemp(this_temp);             /* Thermistor value (int8_t) */
    uint8_t therm_fault_flag = 0u;
    if (therm_fault) {
        therm_fault_flag = 0x80u;
    }
    payload[3] = (uint8_t)(capped_idx | therm_fault_flag);
    payload[4] = CAN_PackTemp(s_cache.min_temp);      /* Lowest valid temp on module */
    payload[5] = CAN_PackTemp(s_cache.max_temp);      /* Highest valid temp on module */
    payload[6] = s_cache.max_id;                      /* Highest loaded thermistor ID on module */
    payload[7] = s_cache.min_id;                      /* Lowest loaded thermistor ID on module */

    CAN_Debug_SetLastGeneralInfo(global_id, capped_idx, this_temp);
}


/* -------------------------------------------------------------------------- */
/* Main Send Function - Call from main loop                                   */
/* -------------------------------------------------------------------------- */

void CAN_SendMessages(void)
{
    static uint8_t therm_index = 0U;
    uint8_t payload[8];
    int result;

    /* Ensure we have at least one temperature snapshot before transmitting */
    if (s_cache.count == 0U && g_can_ctx.thermistors.num_active > 0U) {
        ConvertAllThermistors(g_can_ctx.thermistors.thermistor_adc_values,
                              g_can_ctx.thermistors.num_active);
    }
    /* 200 ms: J1939 Address Claim */
    if (Timer_CheckCan200msFlag()) {
        EncodeJ1939Claim(payload);
        result = CAN_Comm_SendExt(THERM_J1939_CLAIM_ID, payload, 8U);
        CAN_Debug_RecordClaim(payload, HAL_GetTick());
        if (result != 0) {
            LOG_WARN("TX 0x%08lX claim failed result=%d", (unsigned long)THERM_J1939_CLAIM_ID, result);
        } else {
            CAN_Debug_RequestLogClaim();
        }
    }

    /* 100 ms: BMS Broadcast + General Broadcast */
    if (Timer_CheckCan100msFlag()) {
        const uint32_t now_ms = HAL_GetTick();
        /* Send BMS Broadcast */
        EncodeBMSBroadcast(payload);
        result = CAN_Comm_SendExt(THERM_BMS_BROADCAST_ID, payload, 8U);

        CAN_Debug_RecordBms(payload, now_ms);
        CAN_Debug_UpdateCoreState(s_default_claim.thermistor_module,
                      ORION_THERM_SOURCE_ADDR,
                      ORION_BMS_TARGET_ADDR,
                      s_cache.count,
                      s_cache.valid_count,
                      s_cache.module_fault,
                      s_cache.fault_mask,
                      s_cache.min_temp,
                      s_cache.max_temp,
                      s_cache.avg_temp,
                      s_cache.max_id,
                      s_cache.min_id);

        if (result != 0) {
            LOG_WARN("TX 0x%08lX bms failed result=%d", (unsigned long)THERM_BMS_BROADCAST_ID, result);
        } else {
            CAN_Debug_RequestLogBms();
        }

        /* Send General Broadcast (round-robin) */
        EncodeGeneralBroadcast(therm_index, payload);
        result = CAN_Comm_SendExt(THERM_GENERAL_BROADCAST_ID, payload, 8U);

        CAN_Debug_RecordGeneral(payload, now_ms);
        CAN_Debug_UpdateCoreState(s_default_claim.thermistor_module,
                      ORION_THERM_SOURCE_ADDR,
                      ORION_BMS_TARGET_ADDR,
                      s_cache.count,
                      s_cache.valid_count,
                      s_cache.module_fault,
                      s_cache.fault_mask,
                      s_cache.min_temp,
                      s_cache.max_temp,
                      s_cache.avg_temp,
                      s_cache.max_id,
                      s_cache.min_id);

        if (result != 0) {
            LOG_WARN("TX 0x%08lX gen failed result=%d", (unsigned long)THERM_GENERAL_BROADCAST_ID, result);
        } else {
            CAN_Debug_RequestLogGeneral();
        }

        /* Next thermistor */
        if (s_cache.count > 0U) {
            therm_index = (uint8_t)((therm_index + 1U) % s_cache.count);
        }
    }

    /* Emit any pending debug logs (set via interrupt or success paths) */
    CAN_Debug_TryLogClaim();
    CAN_Debug_TryLogBms();
    CAN_Debug_TryLogGeneral();
}
