#ifndef CAN_MESSAGES_H
#define CAN_MESSAGES_H

#include <stdbool.h>
#include <stdint.h>

/* --- Application dimensions --- */
#define MAX_THERMISTORS 10u
#define MAX_VOLTAGE     14u

/* --- Common CAN payload and packing constants --- */
#define CAN_FRAME_LEN_BYTES             8u
#define CAN_BYTE_MASK                   0x00FFu
#define CAN_SOURCE_ADDR_SHIFT_BITS      8u
#define CAN_EXTV_SEGMENT_INDEX_MASK     0x03u
#define CAN_GLOBAL_THERM_STRIDE         80u
#define CAN_BMS_CHECKSUM_BASE           0x39u
#define CAN_BMS_CHECKSUM_DATA_BYTES     7u
#define CAN_ENABLED_COUNT_MASK          0x7Fu
#define CAN_MODULE_FAULT_FLAG_MASK      0x80u
#define CAN_THERM_FAULT_FLAG_MASK       0x80u

/* --- External ADC voltage broadcast layout --- */
#define EXTERNAL_ADC_VOLTAGE_FRAMES            4u
#define EXTERNAL_ADC_VOLTAGES_PER_FRAME        4u
#define EXTERNAL_ADC_VOLTAGE_BASE_ID           0x18FF3000u

/* --- Orion thermistor expansion defaults --- */
#define ORION_BMS_TARGET_ADDR       0xF3u
#define ORION_THERM_SOURCE_ADDR     0x80u
#define ORION_THERM_MODULE_INDEX    ((uint8_t)(ORION_THERM_SOURCE_ADDR - 0x80u))
#define ORION_THERM_MIN_VALID_C     (-128)
#define ORION_THERM_MAX_VALID_C     127
#define ORION_THERM_HOT_THRESHOLD_C 65
#define ORION_THERM_ADC_OPEN_FAULT_THRESHOLD  4090u
#define ORION_THERM_ADC_SHORT_FAULT_THRESHOLD 5u
#define ORION_CLAIM_UNIQUE_ID0      0xF3u
#define ORION_CLAIM_UNIQUE_ID1      0x00u
#define ORION_CLAIM_UNIQUE_ID2      ORION_THERM_SOURCE_ADDR
#define ORION_CLAIM_MODULE_INDEX_SHIFT 3u
#define ORION_CLAIM_CONSTANT0       0x40u
#define ORION_CLAIM_CONSTANT1       0x1Eu
#define ORION_CLAIM_CONSTANT2       0x90u

/* --- CAN debug and fault logging timing --- */
#define CAN_DEBUG_STREAM_DEFAULT    1
#define CAN_DEBUG_LOG_THROTTLE_MS   2000u
#define CAN_DEBUG_EMU_MODULES               2u
#define CAN_DEBUG_EMU_CHANNELS_PER_MODULE   5u
#define THERM_FAULT_PERSIST_LOG_MS          5000u
#define CAN_TX_DROP_LOG_PERIOD_MS           3000u

/* --- Orion CAN ID bases and defaults --- */
#define THERM_J1939_CLAIM_ID_BASE        0x18EEFF00u
#define THERM_BMS_BROADCAST_ID_BASE      0x18390000u
#define THERM_GENERAL_BROADCAST_ID_BASE  0x18380000u
#define THERM_J1939_CLAIM_ID       (THERM_J1939_CLAIM_ID_BASE | (uint32_t)ORION_THERM_SOURCE_ADDR)
#define THERM_BMS_BROADCAST_ID     (THERM_BMS_BROADCAST_ID_BASE | ((uint32_t)ORION_BMS_TARGET_ADDR << CAN_SOURCE_ADDR_SHIFT_BITS) | (uint32_t)ORION_THERM_SOURCE_ADDR)
#define THERM_GENERAL_BROADCAST_ID (THERM_GENERAL_BROADCAST_ID_BASE | ((uint32_t)ORION_BMS_TARGET_ADDR << CAN_SOURCE_ADDR_SHIFT_BITS) | (uint32_t)ORION_THERM_SOURCE_ADDR)

typedef struct {
    uint8_t unique_id[3];
    uint8_t bms_target;
    uint8_t thermistor_module;
    uint8_t constant[3];
} ThermJ1939ClaimMsg_t;

typedef struct {
    uint16_t thermistor_adc_values[MAX_THERMISTORS];
    uint8_t num_active;
} ThermistorADCData_t;

typedef struct ThermistorCache {
    uint16_t adc_raw[MAX_THERMISTORS];
    int8_t temps[MAX_THERMISTORS];
    uint8_t count;
    uint8_t valid_count;
    int8_t min_temp;
    uint8_t min_id;
    int8_t max_temp;
    uint8_t max_id;
    int8_t avg_temp;
    uint16_t fault_mask;
    bool module_fault;
    uint32_t last_update_ms;
} ThermistorCache_t;

typedef struct {
    uint16_t voltage_adc_values[MAX_VOLTAGE];
    uint8_t num_active;
} VoltageADCData_t;

/* Convert single ADC value to temperature */
int8_t ADC_ToTemperature(uint16_t adc_value);

/* Convert all thermistor ADC values to temperatures */
void ConvertAllThermistors(const uint16_t *adc_values, uint8_t count);

/* Encode messages */
void EncodeBMSBroadcast(uint8_t *payload);
void EncodeGeneralBroadcast(uint8_t therm_index, uint8_t *payload);
void CAN_UpdateExternalADCVoltages(const uint16_t *out_mv, uint8_t count);

/* Main send function */
void CAN_SendMessages(void);

/* Debug helpers for UART inspection */
/* Debug helpers and fault injection now live in can_debugging.h */

#endif
