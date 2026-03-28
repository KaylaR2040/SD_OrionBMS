/*
 * ===========================================================================
 * File: can_debugging.c
 * Description: Debug utilities for CAN thermistor broadcasts. Centralizes
 *              developer-only overrides, snapshots of last-sent frames, and
 *              logging helpers to keep other modules clean.
 * ===========================================================================
 */

#include "master.h"
#include <string.h>

static bool s_debug_mode = false;
static bool s_stream_enabled = (CAN_DEBUG_STREAM_DEFAULT != 0);
static bool s_dual_module_emulation_enabled = false;
static CAN_DebugSnapshot_t s_dbg = {0};
static bool s_log_claim_pending = false;
static bool s_log_bms_pending = false;
static bool s_log_gen_pending = false;
static uint32_t s_log_claim_last_ms = 0;
static uint32_t s_log_bms_last_ms = 0;
static uint32_t s_log_gen_last_ms = 0;

#define LOG_MIN_CLAIM_INTERVAL_MS CAN_DEBUG_LOG_THROTTLE_MS
#define LOG_MIN_BMS_INTERVAL_MS   CAN_DEBUG_LOG_THROTTLE_MS
#define LOG_MIN_GEN_INTERVAL_MS   CAN_DEBUG_LOG_THROTTLE_MS
#define LOG_MIN_DUAL_MODE_MS      100u

static uint32_t Debug_LogMinInterval(uint32_t default_interval_ms)
{
    if (CAN_Debug_IsDualModuleEmulationEnabled()) {
        return LOG_MIN_DUAL_MODE_MS;
    }
    return default_interval_ms;
}

typedef struct {
    uint16_t temp_override_mask;
    uint16_t sensor_fault_mask;
    int8_t temp_override_c[MAX_THERMISTORS];
} CAN_DebugState_t;

static CAN_DebugState_t s_debug = {0};

/* ---------------------------------------------------------------------------
 * Helpers (private)
 * ---------------------------------------------------------------------------
 */
static void Debug_RecordPayload(uint8_t *dst, const uint8_t *src, uint32_t now_ms,
                                uint32_t *last_ts_ms, uint32_t *interval_ms)
{
    memcpy(dst, src, 8U);
    if (*last_ts_ms != 0U) {
        *interval_ms = now_ms - *last_ts_ms;
    } else {
        *interval_ms = 0U;
    }
    *last_ts_ms = now_ms;
}

/* ---------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------------
 */
void CAN_Debug_Init(void)
{
    memset(&s_debug, 0, sizeof(s_debug));
}

void CAN_Debug_Clear(void)
{
    memset(&s_debug, 0, sizeof(s_debug));
    memset(&s_dbg, 0, sizeof(s_dbg));
    s_dual_module_emulation_enabled = false;
    s_log_claim_pending = false;
    s_log_bms_pending = false;
    s_log_gen_pending = false;
}

/* ---------------------------------------------------------------------------
 * Mode and streaming toggles
 * ---------------------------------------------------------------------------
 */
void CAN_Debug_SetMode(bool enabled)
{
    s_debug_mode = enabled;
}

bool CAN_Debug_IsEnabled(void)
{
    return s_debug_mode;
}

void CAN_Debug_SetStreamEnabled(bool enabled)
{
    s_stream_enabled = enabled;
}

bool CAN_Debug_IsStreamEnabled(void)
{
    return s_stream_enabled;
}

void CAN_Debug_SetDualModuleEmulation(bool enabled)
{
    s_dual_module_emulation_enabled = enabled;
}

bool CAN_Debug_IsDualModuleEmulationEnabled(void)
{
    return s_dual_module_emulation_enabled;
}

/* ---------------------------------------------------------------------------
 * Manual overrides (DEV WORK HERE - add hard-coded overrides)
 NOTE: MANUAL OVERRIDE HERE
 * ---------------------------------------------------------------------------
 */
void CAN_Debug_ApplyManualOverrides(void) 
{
    CAN_Debug_Clear();

    /* DEBUG TEST MODE: emulate two modules (0x80 + 0x81), 5 thermistors each. */
    CAN_Debug_SetDualModuleEmulation(true);

    /* Live ADC test: keep real sensor values (no forced temperature override). */
    // CAN_Debug_All_TempOverride(25);

    /* Fault: Pack TOO HOT*/
    //CAN_Debug_All_TempOverride(85);

    /* FAULT: Sensor Fault*/
    //CAN_Debug_SetSensorFault(5, true);

    /* OVERRIDE: All thermistors */
    // CAN_Debug_SetTempOverride(0, 25);
    // CAN_Debug_SetTempOverride(1, 25);
    // CAN_Debug_SetTempOverride(2, 25);
    // CAN_Debug_SetTempOverride(3, 25);
    // CAN_Debug_SetTempOverride(4, 25);
    // CAN_Debug_SetTempOverride(5, 25);
    // CAN_Debug_SetTempOverride(6, 25);
    // CAN_Debug_SetTempOverride(7, 25);
    // CAN_Debug_SetTempOverride(8, 25);
    // CAN_Debug_SetTempOverride(9, 25);
}

void CAN_Debug_All_TempOverride(int8_t temp_c)
{
    for (uint8_t i = 0; i < MAX_THERMISTORS; i++) {
        CAN_Debug_SetTempOverride(i, temp_c);
    }
}



bool CAN_Debug_SetTempOverride(uint8_t idx, int8_t temp_c)
{
    if (idx >= MAX_THERMISTORS) {
        return false;
    }

    s_debug.temp_override_mask |= (uint16_t)(1u << idx);
    s_debug.temp_override_c[idx] = temp_c;
    return true;
}

bool CAN_Debug_GetTempOverride(uint8_t idx, int8_t *temp_c)
{
    if (idx >= MAX_THERMISTORS) {
        return false;
    }
    if ((s_debug.temp_override_mask & (uint16_t)(1u << idx)) == 0u) {
        return false;
    }
    if (temp_c) {
        *temp_c = s_debug.temp_override_c[idx];
    }
    return true;
}

bool CAN_Debug_SetSensorFault(uint8_t idx, bool enabled)
{
    if (idx >= MAX_THERMISTORS) {
        return false;
    }

    if (enabled) {
        s_debug.sensor_fault_mask |= (uint16_t)(1u << idx);
    } else {
        s_debug.sensor_fault_mask &= (uint16_t)~(1u << idx);
    }
    return true;
}

bool CAN_Debug_IsSensorFaulted(uint8_t idx)
{
    if (idx >= MAX_THERMISTORS) {
        return false;
    }
    return (s_debug.sensor_fault_mask & (uint16_t)(1u << idx)) != 0u;
}

uint16_t CAN_Debug_GetTempOverrideMask(void)
{
    return s_debug.temp_override_mask;
}

uint16_t CAN_Debug_GetSensorFaultMask(void)
{
    return s_debug.sensor_fault_mask;
}

/* ---------------------------------------------------------------------------
 * Snapshot/state updates
 * ---------------------------------------------------------------------------
 */
void CAN_Debug_UpdateCoreState(uint8_t module_index,
                               uint8_t source_addr,
                               uint8_t bms_target_addr,
                               uint8_t enabled_count,
                               uint8_t valid_count,
                               bool module_fault,
                               uint16_t thermistor_fault_mask,
                               int8_t min_temp_c,
                               int8_t max_temp_c,
                               int8_t avg_temp_c,
                               uint8_t max_id,
                               uint8_t min_id)
{
    s_dbg.module_index = module_index;
    s_dbg.source_addr = source_addr;
    s_dbg.bms_target_addr = bms_target_addr;
    s_dbg.enabled_count = enabled_count;
    s_dbg.valid_count = valid_count;
    if (module_fault) {
        s_dbg.module_fault = 1u;
    } else {
        s_dbg.module_fault = 0u;
    }
    s_dbg.thermistor_fault_mask = thermistor_fault_mask;
    s_dbg.min_temp_c = min_temp_c;
    s_dbg.max_temp_c = max_temp_c;
    s_dbg.avg_temp_c = avg_temp_c;
    s_dbg.max_id = max_id;
    s_dbg.min_id = min_id;
    s_dbg.temp_override_mask = CAN_Debug_GetTempOverrideMask();
    s_dbg.sensor_fault_mask = CAN_Debug_GetSensorFaultMask();
}

void CAN_Debug_RecordClaim(const uint8_t payload[8], uint32_t now_ms)
{
    Debug_RecordPayload(s_dbg.claim, payload, now_ms,
                       &s_dbg.last_claim_ts_ms, &s_dbg.last_claim_interval_ms);
}

void CAN_Debug_RecordBms(const uint8_t payload[8], uint32_t now_ms)
{
    Debug_RecordPayload(s_dbg.bms, payload, now_ms,
                       &s_dbg.last_bms_ts_ms, &s_dbg.last_bms_interval_ms);
    s_dbg.last_bms_checksum = payload[7];
}

void CAN_Debug_RecordGeneral(const uint8_t payload[8], uint32_t now_ms)
{
    Debug_RecordPayload(s_dbg.general, payload, now_ms,
                       &s_dbg.last_general_ts_ms, &s_dbg.last_general_interval_ms);
}

void CAN_Debug_SetLastGeneralInfo(uint16_t global_id, uint8_t local_id, int8_t temp_c)
{
    s_dbg.last_general_global_id = global_id;
    s_dbg.last_general_local_id = local_id;
    s_dbg.last_general_temp_c = temp_c;
}

const CAN_DebugSnapshot_t *CAN_GetDebugSnapshot(void)
{
    return &s_dbg;
}

/* ---------------------------------------------------------------------------
 * Logging flags and emitters (flag-driven; respect stream enable)
 * ---------------------------------------------------------------------------
 */
void CAN_Debug_RequestLogClaim(void)
{
    s_log_claim_pending = true;
}

void CAN_Debug_RequestLogBms(void)
{
    s_log_bms_pending = true;
}

void CAN_Debug_RequestLogGeneral(void)
{
    s_log_gen_pending = true;
}

void CAN_Debug_TryLogClaim(void)
{
    if (!CAN_Debug_IsStreamEnabled()) {
        return;
    }
    if (!s_log_claim_pending) {
        return;
    }

    const uint32_t now_ms = HAL_GetTick();
    if ((now_ms - s_log_claim_last_ms) < Debug_LogMinInterval(LOG_MIN_CLAIM_INTERVAL_MS)) {
        return;
    }

    const uint32_t claim_id = 0x18EEFF00u | (uint32_t)s_dbg.source_addr;
    LOG_DEBUG("TX 0x%08lX dt=%lums module=%u target=0x%02X [%02X %02X %02X %02X %02X %02X %02X %02X]",
              (unsigned long)claim_id,
              (unsigned long)s_dbg.last_claim_interval_ms,
              s_dbg.module_index,
              s_dbg.bms_target_addr,
              s_dbg.claim[0], s_dbg.claim[1], s_dbg.claim[2], s_dbg.claim[3],
              s_dbg.claim[4], s_dbg.claim[5], s_dbg.claim[6], s_dbg.claim[7]);
    s_log_claim_pending = false;
    s_log_claim_last_ms = now_ms;
}

void CAN_Debug_TryLogBms(void)
{
    if (!CAN_Debug_IsStreamEnabled()) {
        return;
    }
    if (!s_log_bms_pending) {
        return;
    }

    const uint32_t now_ms = HAL_GetTick();
    if ((now_ms - s_log_bms_last_ms) < Debug_LogMinInterval(LOG_MIN_BMS_INTERVAL_MS)) {
        return;
    }

    const uint32_t bms_id = 0x18390000u | ((uint32_t)s_dbg.bms_target_addr << 8) | (uint32_t)s_dbg.source_addr;
    LOG_DEBUG("TX 0x%08lX bms dt=%lums module=%u low=%d high=%d avg=%d en=%u valid=%u fault=%u hi_id=%u lo_id=%u csum=0x%02X [%02X %02X %02X %02X %02X %02X %02X %02X]",
              (unsigned long)bms_id,
              (unsigned long)s_dbg.last_bms_interval_ms,
              s_dbg.module_index,
              (int)s_dbg.min_temp_c,
              (int)s_dbg.max_temp_c,
              (int)s_dbg.avg_temp_c,
              s_dbg.enabled_count,
              s_dbg.valid_count,
              s_dbg.module_fault,
              s_dbg.max_id,
              s_dbg.min_id,
              s_dbg.last_bms_checksum,
              s_dbg.bms[0], s_dbg.bms[1], s_dbg.bms[2], s_dbg.bms[3],
              s_dbg.bms[4], s_dbg.bms[5], s_dbg.bms[6], s_dbg.bms[7]);
    s_log_bms_pending = false;
    s_log_bms_last_ms = now_ms;
}

void CAN_Debug_TryLogGeneral(void)
{
    if (!CAN_Debug_IsStreamEnabled()) {
        return;
    }
    if (!s_log_gen_pending) {
        return;
    }

    const uint32_t now_ms = HAL_GetTick();
    if ((now_ms - s_log_gen_last_ms) < Debug_LogMinInterval(LOG_MIN_GEN_INTERVAL_MS)) {
        return;
    }

    const uint16_t fault_mask = s_dbg.thermistor_fault_mask;
    const uint8_t idx = s_dbg.last_general_local_id;
    const unsigned fault = (unsigned)((fault_mask & (uint16_t)(1u << idx)) != 0u);

    const uint32_t general_id = 0x18380000u | ((uint32_t)s_dbg.bms_target_addr << 8) | (uint32_t)s_dbg.source_addr;
    LOG_DEBUG("TX 0x%08lX gen dt=%lums global_id=%u local_id=%u fault=%u temp=%d low=%d high=%d hi_id=%u lo_id=%u [%02X %02X %02X %02X %02X %02X %02X %02X]",
              (unsigned long)general_id,
              (unsigned long)s_dbg.last_general_interval_ms,
              (unsigned)s_dbg.last_general_global_id,
              s_dbg.last_general_local_id,
              fault,
              (int)s_dbg.last_general_temp_c,
              (int)s_dbg.min_temp_c,
              (int)s_dbg.max_temp_c,
              s_dbg.max_id,
              s_dbg.min_id,
              s_dbg.general[0], s_dbg.general[1], s_dbg.general[2], s_dbg.general[3],
              s_dbg.general[4], s_dbg.general[5], s_dbg.general[6], s_dbg.general[7]);
    s_log_gen_pending = false;
    s_log_gen_last_ms = now_ms;
}

/* ---------------------------------------------------------------------------
 * Debug print helpers
 * ---------------------------------------------------------------------------
 */
void CAN_DebugPrintDump(const ThermistorCache_t *cache)
{
    if (!cache) {
        return;
    }
    LOG_INFO("THERM DUMP count=%u updated=%lums ago", cache->count,
             (unsigned long)(HAL_GetTick() - cache->last_update_ms));
    for (uint8_t i = 0U; i < cache->count; i++) {
        LOG_INFO("  ch=%u adc=%u temp=%dC", i,
                 (unsigned)cache->adc_raw[i], (int)cache->temps[i]);
    }
}

void CAN_DebugPrintTiming(void)
{
    LOG_INFO("TIMING claim=%lums@%lums bms=%lums@%lums gen=%lums@%lums",
             (unsigned long)s_dbg.last_claim_interval_ms,
             (unsigned long)s_dbg.last_claim_ts_ms,
             (unsigned long)s_dbg.last_bms_interval_ms,
             (unsigned long)s_dbg.last_bms_ts_ms,
             (unsigned long)s_dbg.last_general_interval_ms,
             (unsigned long)s_dbg.last_general_ts_ms);
}

void CAN_DebugPrintThermistor(const ThermistorCache_t *cache, uint8_t therm_index)
{
    if (!cache) {
        return;
    }
    if (therm_index >= cache->count) {
        LOG_WARN("Thermistor %u out of range (n=%u)", therm_index, cache->count);
        return;
    }

    LOG_INFO("THERM %u adc=%u temp=%dC", therm_index,
             (unsigned)cache->adc_raw[therm_index],
             (int)cache->temps[therm_index]);
    LOG_INFO("  fault=%u override=%u",
             (unsigned)((cache->fault_mask & (uint16_t)(1u << therm_index)) != 0u),
             (unsigned)(CAN_Debug_GetTempOverrideMask() & (uint16_t)(1u << therm_index)));
}

void CAN_DebugPrintFaultState(void)
{
    LOG_INFO("FAULT module_fault=%u enabled=%u valid=%u fault_mask=0x%04X temp_override_mask=0x%04X sensor_fault_mask=0x%04X",
             s_dbg.module_fault,
             s_dbg.enabled_count,
             s_dbg.valid_count,
             (unsigned)s_dbg.thermistor_fault_mask,
             (unsigned)s_dbg.temp_override_mask,
             (unsigned)s_dbg.sensor_fault_mask);
}

void CAN_DebugPrintCanSnapshot(void)
{
    const uint32_t claim_id = 0x18EEFF00u | (uint32_t)s_dbg.source_addr;
    const uint32_t bms_id = 0x18390000u | ((uint32_t)s_dbg.bms_target_addr << 8) | (uint32_t)s_dbg.source_addr;
    const uint32_t general_id = 0x18380000u | ((uint32_t)s_dbg.bms_target_addr << 8) | (uint32_t)s_dbg.source_addr;

    LOG_INFO("CAN SNAPSHOT (last sent)");
    LOG_INFO("  Claim 0x%08lX interval=%lums source=0x%02X target=0x%02X module=%u [%02X %02X %02X %02X %02X %02X %02X %02X]",
             (unsigned long)claim_id,
             (unsigned long)s_dbg.last_claim_interval_ms,
             s_dbg.source_addr,
             s_dbg.bms_target_addr,
             s_dbg.module_index,
             s_dbg.claim[0], s_dbg.claim[1], s_dbg.claim[2], s_dbg.claim[3],
             s_dbg.claim[4], s_dbg.claim[5], s_dbg.claim[6], s_dbg.claim[7]);
    LOG_INFO("  BMS   0x%08lX interval=%lums module=%u low=%d high=%d avg=%d enabled=%u valid=%u fault=%u hi_id=%u lo_id=%u checksum=0x%02X [%02X %02X %02X %02X %02X %02X %02X %02X]",
             (unsigned long)bms_id,
             (unsigned long)s_dbg.last_bms_interval_ms,
             s_dbg.module_index,
             (int)s_dbg.min_temp_c,
             (int)s_dbg.max_temp_c,
             (int)s_dbg.avg_temp_c,
             s_dbg.enabled_count,
             s_dbg.valid_count,
             s_dbg.module_fault,
             s_dbg.max_id,
             s_dbg.min_id,
             s_dbg.last_bms_checksum,
             s_dbg.bms[0], s_dbg.bms[1], s_dbg.bms[2], s_dbg.bms[3],
             s_dbg.bms[4], s_dbg.bms[5], s_dbg.bms[6], s_dbg.bms[7]);
    LOG_INFO("  GEN   0x%08lX interval=%lums global_id=%u local_id=%u fault=%u temp=%d low=%d high=%d hi_id=%u lo_id=%u [%02X %02X %02X %02X %02X %02X %02X %02X]",
             (unsigned long)general_id,
             (unsigned long)s_dbg.last_general_interval_ms,
             (unsigned)s_dbg.last_general_global_id,
             s_dbg.last_general_local_id,
             (unsigned)((s_dbg.thermistor_fault_mask & (uint16_t)(1u << s_dbg.last_general_local_id)) != 0u),
             (int)s_dbg.last_general_temp_c,
             (int)s_dbg.min_temp_c,
             (int)s_dbg.max_temp_c,
             s_dbg.max_id,
             s_dbg.min_id,
             s_dbg.general[0], s_dbg.general[1], s_dbg.general[2], s_dbg.general[3],
             s_dbg.general[4], s_dbg.general[5], s_dbg.general[6], s_dbg.general[7]);
}
