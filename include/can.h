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

#define CAN_APP_DEFAULT_KBPS 1000U

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
void CAN_ServiceTask(void);
int CAN_Comm_Init_kbps(uint32_t kbps);
int CAN_FindTiming(uint32_t fclk_hz, uint32_t bitrate_hz, uint32_t *out_presc, uint32_t *out_tseg1,uint32_t *out_tseg2, uint32_t *out_sjw);
int CAN_Comm_SendStd(uint32_t std_id, const uint8_t *data, uint8_t len);
int CAN_Comm_SendExt(uint32_t ext_id, const uint8_t *data, uint8_t len);



#ifdef __cplusplus
}
#endif

#endif /* CAN_H */
