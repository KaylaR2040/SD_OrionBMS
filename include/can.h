/*
 * ===========================================================================
 * File: can.h
 * Description: High-level CAN/FDCAN service interfaces and app data context.
 *
 * Notes:
 *   - Transport primitives live in can.c; message encoders/IDs in can_messages.h.
 * ===========================================================================
 */

#ifndef CAN_H
#define CAN_H

// STM32 HAL
#include "stm32g4xx_hal.h"

// Project headers
#include "can_messages.h"

/* --- Application defaults and return codes --- */
#define CAN_APP_DEFAULT_KBPS 1000U
#define CAN_TX_RESULT_TRANSIENT_DROP (-2)

/* --- Peripheral and frame limits --- */
#define CAN_FDCAN_KERNEL_CLK_HZ 85000000UL
#define CAN_MAX_DLEN_BYTES      8U
#define CAN_EXT_ID_MASK         0x1FFFFFFFU

/* --- Filter ranges --- */
#define CAN_STD_FILTER_ID_MIN 0x000U
#define CAN_STD_FILTER_ID_MAX 0x7FFU
#define CAN_EXT_FILTER_ID_MIN 0x00000000U
#define CAN_EXT_FILTER_ID_MAX CAN_EXT_ID_MASK

/* --- Timing search constants --- */
#define CAN_TIMING_TQ_MIN         8U
#define CAN_TIMING_TQ_MAX         25U
#define CAN_TIMING_FALLBACK_TQ    10U
#define CAN_TIMING_PRESC_MIN      1U
#define CAN_TIMING_PRESC_MAX      512U
#define CAN_TIMING_TSEG1_MIN      1U
#define CAN_TIMING_TSEG1_MAX      255U
#define CAN_TIMING_TSEG2_MIN      1U
#define CAN_TIMING_SJW_MAX        4U
#define CAN_TIMING_TSEG2_DIVISOR  4U
#define CAN_FD_DATA_TIMING_DEFAULT 1U
#define CAN_FILTER_INDEX_DEFAULT   0U
#define CAN_TX_MESSAGE_MARKER_DEFAULT 0U

#ifdef __cplusplus
extern "C" {
#endif

/* Application-level CAN telemetry context.
 * Uses types defined in can_messages.h (ThermistorADCData_t, L180CellVoltageMsg_t).
 */
typedef struct {
    ThermistorADCData_t thermistors;
    VoltageADCData_t voltages;
    uint8_t therm_index;
    uint32_t last_claim_ms;
    uint32_t last_bms_ms;
    uint32_t last_general_ms;
} can_app_ctx_t;

/* Global context instance (defined in can_messages.c) */
extern can_app_ctx_t g_can_ctx;

void CAN_App_Init(uint32_t kbps);
void CAN_App_InitData(can_app_ctx_t *ctx);
void CAN_SetSchedulingEnabled(bool enabled, bool send_immediately);
bool CAN_IsSchedulingEnabled(void);
void CAN_ServiceTask(void);
int CAN_Comm_Init_kbps(uint32_t kbps);
int CAN_FindTiming(uint32_t fclk_hz, uint32_t bitrate_hz, uint32_t *out_presc, uint32_t *out_tseg1,uint32_t *out_tseg2, uint32_t *out_sjw);
int CAN_Comm_SendStd(uint32_t std_id, const uint8_t *data, uint8_t len);
int CAN_Comm_SendExt(uint32_t ext_id, const uint8_t *data, uint8_t len);



#ifdef __cplusplus
}
#endif

#endif /* CAN_H */
