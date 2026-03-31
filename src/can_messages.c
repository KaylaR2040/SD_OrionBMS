/*
 * File: can_messages.c
 * Description: CAN message encoding for thermistor modules and Orion BMS
 TODO: L-180 CAN for voltage ADC 
 TODO: L-180 CAN for temperature ADC
 TODO: Write debugging info for L-180
 */

#include "master.h"
#include "adc.h" /* for ADC_REF_MV and ADC_MAX_COUNTS */

#define THERM_HOT_C 65
#define CAN_DEBUG_EMU_MODULES              2u
#define CAN_DEBUG_EMU_CHANNELS_PER_MODULE  5u

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

static uint16_t CAN_GlobalThermistorIdForModule(uint8_t module_index, uint8_t local_index)
{
    return (uint16_t)((uint16_t)module_index * 80u) + (uint16_t)local_index;
}

static bool CAN_DebugDualModuleActive(void)
{
    return CAN_Debug_IsEnabled() && CAN_Debug_IsDualModuleEmulationEnabled();
}

static uint32_t CAN_ClaimIdForSource(uint8_t source_addr)
{
    return 0x18EEFF00u | (uint32_t)source_addr;
}

static uint32_t CAN_BmsIdForSource(uint8_t source_addr)
{
    return 0x18390000u | ((uint32_t)ORION_BMS_TARGET_ADDR << 8) | (uint32_t)source_addr;
}

static uint32_t CAN_GeneralIdForSource(uint8_t source_addr)
{
    return 0x18380000u | ((uint32_t)ORION_BMS_TARGET_ADDR << 8) | (uint32_t)source_addr;
}

static void CAN_LogTxFailure_(const char *frame_name, uint32_t can_id, int result)
{
    static uint32_t last_drop_log_ms = 0U;
    static uint32_t dropped_frames = 0U;
    const uint32_t now = HAL_GetTick();

    if (result == CAN_TX_RESULT_TRANSIENT_DROP) {
        dropped_frames++;
        if ((now - last_drop_log_ms) >= 1000U) {
            LOG_WARN("CAN TX congested/no-ack: dropped=%lu last=%s id=0x%08lX",
                     (unsigned long)dropped_frames,
                     frame_name,
                     (unsigned long)can_id);
            last_drop_log_ms = now;
            dropped_frames = 0U;
        }
        return;
    }

    LOG_WARN("TX 0x%08lX %s failed result=%d",
             (unsigned long)can_id,
             frame_name,
             result);
}

static void CAN_EncodeJ1939ClaimForModule(uint8_t source_addr, uint8_t module_index, uint8_t *payload)
{
    payload[0] = ORION_CLAIM_UNIQUE_ID0;         /* 0xF3 */
    payload[1] = ORION_CLAIM_UNIQUE_ID1;         /* 0x00 */
    payload[2] = source_addr;                    /* 0x80, 0x81, ... */
    payload[3] = ORION_BMS_TARGET_ADDR;          /* 0xF3 */
    payload[4] = (uint8_t)(module_index << 3);   /* module index << 3 */
    payload[5] = s_default_claim.constant[0];    /* 0x40 */
    payload[6] = s_default_claim.constant[1];    /* 0x1E */
    payload[7] = s_default_claim.constant[2];    /* 0x90 */
}

static void CAN_EncodeBmsForCache(const ThermistorCache_t *cache,
                                  uint8_t module_index,
                                  uint8_t *payload)
{
    const uint8_t enabled = (uint8_t)(cache->valid_count & 0x7FU);

    payload[0] = module_index;                    /* Module number (zero-based) */
    payload[1] = CAN_PackTemp(cache->min_temp);   /* Lowest temp (int8_t) */
    payload[2] = CAN_PackTemp(cache->max_temp);   /* Highest temp (int8_t) */
    payload[3] = CAN_PackTemp(cache->avg_temp);   /* Average temp (int8_t) */

    uint8_t module_fault_flag = 0u;
    if (cache->module_fault) {
        module_fault_flag = 0x80u;
    }

    payload[4] = (uint8_t)(enabled | module_fault_flag);
    payload[5] = cache->max_id;                   /* Highest loaded thermistor ID on module */
    payload[6] = cache->min_id;                   /* Lowest loaded thermistor ID on module */
    payload[7] = CAN_ComputeBmsChecksum(payload);
}

static void CAN_EncodeGeneralForCache(const ThermistorCache_t *cache,
                                      uint8_t module_index,
                                      uint8_t therm_index,
                                      uint8_t *payload)
{
    uint8_t capped_idx = 0U;
    if (therm_index < cache->count) {
        capped_idx = therm_index;
    }

    int8_t this_temp = 0;
    if (cache->count > 0U && therm_index < cache->count) {
        this_temp = cache->temps[therm_index];
    }

    const uint16_t global_id = CAN_GlobalThermistorIdForModule(module_index, capped_idx);
    const bool therm_fault = (cache->fault_mask & CAN_ThermMask(capped_idx)) != 0u;

    payload[0] = (uint8_t)(global_id & 0x00FFu);            /* Global ID low byte */
    payload[1] = (uint8_t)((global_id >> 8U) & 0x00FFu);    /* Global ID high byte */
    payload[2] = CAN_PackTemp(this_temp);                   /* Thermistor value (int8_t) */

    uint8_t therm_fault_flag = 0u;
    if (therm_fault) {
        therm_fault_flag = 0x80u;
    }

    payload[3] = (uint8_t)(capped_idx | therm_fault_flag);
    payload[4] = CAN_PackTemp(cache->min_temp);             /* Lowest valid temp on module */
    payload[5] = CAN_PackTemp(cache->max_temp);             /* Highest valid temp on module */
    payload[6] = cache->max_id;                             /* Highest loaded thermistor ID on module */
    payload[7] = cache->min_id;                             /* Lowest loaded thermistor ID on module */

    CAN_Debug_SetLastGeneralInfo(global_id, capped_idx, this_temp);
}

static uint8_t CAN_DebugModuleSampleCount(uint8_t start_idx)
{
    const uint8_t active = g_can_ctx.thermistors.num_active;
    if (active <= start_idx) {
        return 0U;
    }

    const uint8_t remaining = (uint8_t)(active - start_idx);
    if (remaining > CAN_DEBUG_EMU_CHANNELS_PER_MODULE) {
        return CAN_DEBUG_EMU_CHANNELS_PER_MODULE;
    }
    return remaining;
}

static void CAN_BuildModuleCacheFromAdc(uint8_t start_idx,
                                        uint8_t module_count,
                                        ThermistorCache_t *cache)
{
    if (cache == NULL) {
        return;
    }

    memset(cache, 0, sizeof(*cache));
    if (module_count > CAN_DEBUG_EMU_CHANNELS_PER_MODULE) {
        cache->count = CAN_DEBUG_EMU_CHANNELS_PER_MODULE;
    } else {
        cache->count = module_count;
    }

    cache->min_temp = 127;
    cache->max_temp = -128;
    cache->min_id = 0U;
    cache->max_id = (cache->count > 0U) ? (uint8_t)(cache->count - 1U) : 0U;

    int32_t sum = 0;
    for (uint8_t local_idx = 0U; local_idx < cache->count; local_idx++) {
        const uint8_t abs_idx = (uint8_t)(start_idx + local_idx);
        const uint16_t adc_raw = g_can_ctx.thermistors.thermistor_adc_values[abs_idx];
        int8_t temp_c = Thermistor_ADCToTemp(adc_raw);

        bool therm_fault = false;
        bool therm_too_hot = false;
        const bool sensor_fault_forced = CAN_Debug_IsSensorFaulted(abs_idx);

        int8_t override_temp = 0;
        const bool has_override = CAN_Debug_GetTempOverride(abs_idx, &override_temp);
        const bool debug_override_active = CAN_Debug_IsEnabled() && has_override;

        if (debug_override_active) {
            temp_c = override_temp;
            therm_fault = false;
        } else {
            if (adc_raw >= ORION_THERM_ADC_OPEN_FAULT_THRESHOLD ||
                adc_raw <= ORION_THERM_ADC_SHORT_FAULT_THRESHOLD ||
                temp_c < ORION_THERM_MIN_VALID_C ||
                temp_c > ORION_THERM_MAX_VALID_C) {
                therm_fault = true;
            }
        }

        if (!therm_fault && temp_c > THERM_HOT_C) {
            therm_too_hot = true;
        }

        if (sensor_fault_forced) {
            therm_fault = true;
        }

        cache->adc_raw[local_idx] = adc_raw;
        cache->temps[local_idx] = temp_c;

        /* Keep over-temperature channels "valid" so Orion sees the hot value in min/max/avg.
         * Reserve thermistor fault bits for truly invalid channels (open/short/forced sensor fault). */
        if (therm_fault) {
            cache->fault_mask |= CAN_ThermMask(local_idx);
            cache->module_fault = true;
            continue;
        }

        if (therm_too_hot) {
            const uint32_t millivolts = ((uint32_t)adc_raw * (uint32_t)ADC_REF_MV) / (uint32_t)ADC_MAX_COUNTS;
            LOG_WARN("Thermistor %u too hot: %dC (adc=%u, %lu.%03lu V)",
                     (unsigned)(abs_idx + 1U), (int)temp_c,
                     (unsigned)adc_raw,
                     (unsigned long)(millivolts / 1000u),
                     (unsigned long)(millivolts % 1000u));
        }

        if (temp_c < cache->min_temp) {
            cache->min_temp = temp_c;
        }
        if (temp_c > cache->max_temp) {
            cache->max_temp = temp_c;
        }
        sum += temp_c;
        cache->valid_count++;
    }

    if (cache->valid_count > 0U) {
        cache->avg_temp = (int8_t)(sum / (int32_t)cache->valid_count);
    } else {
        cache->min_temp = 0;
        cache->max_temp = 0;
        cache->avg_temp = 0;
    }

    cache->last_update_ms = HAL_GetTick();
}


/* -------------------------------------------------------------------------- */
/* ADC to Temperature Conversion                                              */
/* -------------------------------------------------------------------------- */

void ConvertAllThermistors(const uint16_t *adc_values, uint8_t count)
{
    if (!adc_values) {
        return;
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

        /* Track over-temperature separately from invalid-sensor faults.
         * Over-temp must remain valid in CAN stats so BMS can report a hot condition. */
        if (!therm_fault && temp_c > THERM_HOT_C) {
            const uint32_t millivolts = ((uint32_t)adc_raw * (uint32_t)ADC_REF_MV) / (uint32_t)ADC_MAX_COUNTS;
            LOG_WARN("Thermistor %u too hot: %dC (adc=%u, %lu.%03lu V)",
                     (unsigned)(i + 1U), (int)temp_c,
                     (unsigned)adc_raw,
                     (unsigned long)(millivolts / 1000u),
                     (unsigned long)(millivolts % 1000u));
        }

        if (sensor_fault_forced) {
            therm_fault = true;
        }

        s_cache.adc_raw[i] = adc_raw;
        s_cache.temps[i] = temp_c;

        if (therm_fault) {
            s_cache.fault_mask |= therm_mask;
            s_cache.module_fault = true;
            LOG_WARN("Thermistor %u fault set (mask=0x%04X) adc=%u temp=%dC", (unsigned)(i + 1U), (unsigned)s_cache.fault_mask,
                     (unsigned)adc_raw, (int)temp_c);
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
    CAN_EncodeJ1939ClaimForModule(ORION_THERM_SOURCE_ADDR,
                                  s_default_claim.thermistor_module,
                                  payload);
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
    CAN_EncodeBmsForCache(&s_cache, s_default_claim.thermistor_module, payload);
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
    CAN_EncodeGeneralForCache(&s_cache, s_default_claim.thermistor_module, therm_index, payload);
}


/* -------------------------------------------------------------------------- */
/* Main Send Function - Call from main loop                                   */
/* -------------------------------------------------------------------------- */

void CAN_SendMessages(void)
{
    static uint8_t therm_index = 0U;
    static uint8_t emu_therm_index[CAN_DEBUG_EMU_MODULES] = {0U, 0U};
    static uint8_t emu_module_slot_rr = 0U;
    static uint8_t emu_claim_slot_rr = 0U;
    uint8_t payload[8];
    int result;

    /* Ensure we have at least one temperature snapshot before transmitting */
    if (s_cache.count == 0U && g_can_ctx.thermistors.num_active > 0U) {
        ConvertAllThermistors(g_can_ctx.thermistors.thermistor_adc_values,
                              g_can_ctx.thermistors.num_active);
    }

    if (CAN_DebugDualModuleActive()) {
        if (Timer_CheckCan200msFlag()) {
            const uint32_t now_ms = HAL_GetTick();
            const uint8_t module_slot = emu_claim_slot_rr;
            const uint8_t source_addr = (uint8_t)(ORION_THERM_SOURCE_ADDR + module_slot);
            const uint8_t module_index = (uint8_t)(ORION_THERM_MODULE_INDEX + module_slot);
            const uint32_t claim_id = CAN_ClaimIdForSource(source_addr);

            CAN_EncodeJ1939ClaimForModule(source_addr, module_index, payload);
            result = CAN_Comm_SendExt(claim_id, payload, 8U);
            CAN_Debug_RecordClaim(payload, now_ms);

            if (result != 0) {
                CAN_LogTxFailure_("claim", claim_id, result);
            } else {
                CAN_Debug_UpdateCoreState(module_index,
                                          source_addr,
                                          ORION_BMS_TARGET_ADDR,
                                          0U,
                                          0U,
                                          false,
                                          0U,
                                          0,
                                          0,
                                          0,
                                          0U,
                                          0U);
                CAN_Debug_RequestLogClaim();
            }
            emu_claim_slot_rr = (uint8_t)((emu_claim_slot_rr + 1U) % CAN_DEBUG_EMU_MODULES);
        }

        if (Timer_CheckCan100msFlag()) {
            const uint32_t now_ms = HAL_GetTick();
            const uint8_t module_slot = emu_module_slot_rr;
            const uint8_t source_addr = (uint8_t)(ORION_THERM_SOURCE_ADDR + module_slot);
            const uint8_t module_index = (uint8_t)(ORION_THERM_MODULE_INDEX + module_slot);
            const uint8_t start_idx = (uint8_t)(module_slot * CAN_DEBUG_EMU_CHANNELS_PER_MODULE);
            const uint8_t module_count = CAN_DebugModuleSampleCount(start_idx);
            const uint32_t bms_id = CAN_BmsIdForSource(source_addr);
            const uint32_t gen_id = CAN_GeneralIdForSource(source_addr);

            ThermistorCache_t module_cache;
            CAN_BuildModuleCacheFromAdc(start_idx, module_count, &module_cache);

            CAN_EncodeBmsForCache(&module_cache, module_index, payload);
            result = CAN_Comm_SendExt(bms_id, payload, 8U);
            CAN_Debug_RecordBms(payload, now_ms);
            CAN_Debug_UpdateCoreState(module_index,
                                      source_addr,
                                      ORION_BMS_TARGET_ADDR,
                                      module_cache.count,
                                      module_cache.valid_count,
                                      module_cache.module_fault,
                                      module_cache.fault_mask,
                                      module_cache.min_temp,
                                      module_cache.max_temp,
                                      module_cache.avg_temp,
                                      module_cache.max_id,
                                      module_cache.min_id);
            if (result != 0) {
                CAN_LogTxFailure_("bms", bms_id, result);
            } else {
                CAN_Debug_RequestLogBms();
            }

            CAN_EncodeGeneralForCache(&module_cache,
                                      module_index,
                                      emu_therm_index[module_slot],
                                      payload);
            result = CAN_Comm_SendExt(gen_id, payload, 8U);
            CAN_Debug_RecordGeneral(payload, now_ms);
            CAN_Debug_UpdateCoreState(module_index,
                                      source_addr,
                                      ORION_BMS_TARGET_ADDR,
                                      module_cache.count,
                                      module_cache.valid_count,
                                      module_cache.module_fault,
                                      module_cache.fault_mask,
                                      module_cache.min_temp,
                                      module_cache.max_temp,
                                      module_cache.avg_temp,
                                      module_cache.max_id,
                                      module_cache.min_id);
            if (result != 0) {
                CAN_LogTxFailure_("gen", gen_id, result);
            } else {
                CAN_Debug_RequestLogGeneral();
            }

            if (module_cache.count > 0U) {
                emu_therm_index[module_slot] =
                    (uint8_t)((emu_therm_index[module_slot] + 1U) % module_cache.count);
            } else {
                emu_therm_index[module_slot] = 0U;
            }

            emu_module_slot_rr = (uint8_t)((emu_module_slot_rr + 1U) % CAN_DEBUG_EMU_MODULES);
        }

        CAN_Debug_TryLogClaim();
        CAN_Debug_TryLogBms();
        CAN_Debug_TryLogGeneral();
        return;
    }

    /* 200 ms: J1939 Address Claim */
    if (Timer_CheckCan200msFlag()) {
        EncodeJ1939Claim(payload);
        result = CAN_Comm_SendExt(THERM_J1939_CLAIM_ID, payload, 8U);
        CAN_Debug_RecordClaim(payload, HAL_GetTick());
        if (result != 0) {
            CAN_LogTxFailure_("claim", THERM_J1939_CLAIM_ID, result);
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
            CAN_LogTxFailure_("bms", THERM_BMS_BROADCAST_ID, result);
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
            CAN_LogTxFailure_("gen", THERM_GENERAL_BROADCAST_ID, result);
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
