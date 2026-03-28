#ifndef CAN_DEBUGGING_H
#define CAN_DEBUGGING_H

#include "master.h"

typedef struct ThermistorCache ThermistorCache_t;

/* Debug/fault injection API for thermistor channels */
void CAN_Debug_Init(void);
void CAN_Debug_Clear(void);
void CAN_Debug_SetMode(bool enabled);
bool CAN_Debug_IsEnabled(void);
void CAN_Debug_SetStreamEnabled(bool enabled);
bool CAN_Debug_IsStreamEnabled(void);
void CAN_Debug_SetDualModuleEmulation(bool enabled);
bool CAN_Debug_IsDualModuleEmulationEnabled(void);

/* Manual override hook (hard-coded table inside can_debugging.c) */
void CAN_Debug_ApplyManualOverrides(void);

void CAN_Debug_All_TempOverride(int8_t temp_c);
bool CAN_Debug_SetTempOverride(uint8_t idx, int8_t temp_c);
bool CAN_Debug_GetTempOverride(uint8_t idx, int8_t *temp_c);

bool CAN_Debug_SetSensorFault(uint8_t idx, bool enabled);
bool CAN_Debug_IsSensorFaulted(uint8_t idx);

uint16_t CAN_Debug_GetTempOverrideMask(void);
uint16_t CAN_Debug_GetSensorFaultMask(void);

/* Snapshot management */
typedef struct {
	uint8_t claim[8];
	uint8_t bms[8];
	uint8_t general[8];
	uint8_t last_bms_checksum;
	uint8_t module_index;
	uint8_t source_addr;
	uint8_t bms_target_addr;
	uint8_t enabled_count;
	uint8_t valid_count;
	uint8_t module_fault;
	uint16_t thermistor_fault_mask;
	int8_t min_temp_c;
	int8_t max_temp_c;
	int8_t avg_temp_c;
	uint8_t max_id;
	uint8_t min_id;
	uint16_t last_general_global_id;
	uint8_t last_general_local_id;
	int8_t last_general_temp_c;
	uint16_t temp_override_mask;
	uint16_t sensor_fault_mask;
	uint32_t last_claim_ts_ms;
	uint32_t last_bms_ts_ms;
	uint32_t last_general_ts_ms;
	uint32_t last_claim_interval_ms;
	uint32_t last_bms_interval_ms;
	uint32_t last_general_interval_ms;
} CAN_DebugSnapshot_t;

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
							   uint8_t min_id);

void CAN_Debug_RecordClaim(const uint8_t payload[8], uint32_t now_ms);
void CAN_Debug_RecordBms(const uint8_t payload[8], uint32_t now_ms);
void CAN_Debug_RecordGeneral(const uint8_t payload[8], uint32_t now_ms);
void CAN_Debug_SetLastGeneralInfo(uint16_t global_id, uint8_t local_id, int8_t temp_c);

void CAN_Debug_RequestLogClaim(void);
void CAN_Debug_RequestLogBms(void);
void CAN_Debug_RequestLogGeneral(void);

void CAN_Debug_TryLogClaim(void);
void CAN_Debug_TryLogBms(void);
void CAN_Debug_TryLogGeneral(void);

const CAN_DebugSnapshot_t *CAN_GetDebugSnapshot(void);
void CAN_DebugPrintDump(const ThermistorCache_t *cache);
void CAN_DebugPrintTiming(void);
void CAN_DebugPrintThermistor(const ThermistorCache_t *cache, uint8_t therm_index);
void CAN_DebugPrintFaultState(void);
void CAN_DebugPrintCanSnapshot(void);

#endif /* CAN_DEBUGGING_H */
