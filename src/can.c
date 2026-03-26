/*
 * ===========================================================================
 * File: can.c
 * Description: High-level CAN messaging helpers following CMU guidelines.
 * ===========================================================================
 */

#include "master.h"
#include <stdio.h>
#include <string.h>

// Init Variables
FDCAN_HandleTypeDef g_fdcan1;
FDCAN_TxHeaderTypeDef g_can_tx_header;
uint8_t g_can_tx_data[8];
can_app_ctx_t g_can_ctx;

#ifndef DEFAULT_CAN_KBPS
#define DEFAULT_CAN_KBPS CAN_APP_DEFAULT_KBPS
#endif


/* -------------------------------------------------------------------------- */
/* BITRATE FDCAN CONFIGURATION                          */
/* -------------------------------------------------------------------------- */
void CAN_App_Init(uint32_t kbps)
{
    uint32_t init_kbps = kbps;
    if (init_kbps == 0U) {
        init_kbps = DEFAULT_CAN_KBPS;
    }

    if (CAN_Comm_Init_kbps(init_kbps) != 0) {
        LOG_ERROR("FDCAN init failed");
        Error_Handler();
    }
}


int CAN_Comm_Init_kbps(uint32_t kbps)
{
    uint32_t bitrate_hz = kbps * 1000U;
    uint32_t presc = 0U, tseg1 = 0U, tseg2 = 0U, sjw = 0U;

    if (CAN_FindTiming(FDCAN_KERNEL_CLK_HZ, bitrate_hz, &presc, &tseg1, &tseg2, &sjw) != 0) {
        const uint32_t tq = 10U;
        uint32_t presc_try = (FDCAN_KERNEL_CLK_HZ + (bitrate_hz * tq)/2U) / (bitrate_hz * tq);
        if (presc_try < 1U) {
            presc_try = 1U;
        }
        if (presc_try > 512U) {
            presc_try = 512U;
        }
        presc = presc_try;
        uint32_t tseg_total = tq - 1U;
        uint32_t tseg2_candidate = tseg_total / 4U;
        if (tseg2_candidate != 0U) {
            tseg2 = tseg2_candidate;
        } else {
            tseg2 = 1U;
        }
        tseg1 = tseg_total - tseg2;
        if (tseg2 >= 4U) {
            sjw = 4U;
        } else {
            sjw = tseg2;
        }
    }

    g_fdcan1.Instance = FDCAN1;
    g_fdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    g_fdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    g_fdcan1.Init.AutoRetransmission = ENABLE;
    g_fdcan1.Init.TransmitPause = DISABLE;
    g_fdcan1.Init.ProtocolException = DISABLE;

    g_fdcan1.Init.NominalPrescaler = presc;
    g_fdcan1.Init.NominalSyncJumpWidth = sjw;
    g_fdcan1.Init.NominalTimeSeg1 = tseg1;
    g_fdcan1.Init.NominalTimeSeg2 = tseg2;

    g_fdcan1.Init.DataPrescaler = 1;
    g_fdcan1.Init.DataSyncJumpWidth = 1;
    g_fdcan1.Init.DataTimeSeg1 = 1;
    g_fdcan1.Init.DataTimeSeg2 = 1;

    if (HAL_FDCAN_Init(&g_fdcan1) != HAL_OK) {
        return -1;
    }

    FDCAN_FilterTypeDef sFilterConfig;
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_RANGE;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1 = 0x000;
    sFilterConfig.FilterID2 = 0x7FF;
    if (HAL_FDCAN_ConfigFilter(&g_fdcan1, &sFilterConfig) != HAL_OK) {
        return -1;
    }

    FDCAN_FilterTypeDef xFilterConfig;
    xFilterConfig.IdType = FDCAN_EXTENDED_ID;
    xFilterConfig.FilterIndex = 0;
    xFilterConfig.FilterType = FDCAN_FILTER_RANGE;
    xFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    xFilterConfig.FilterID1 = 0x00000000;
    xFilterConfig.FilterID2 = 0x1FFFFFFF;
    if (HAL_FDCAN_ConfigFilter(&g_fdcan1, &xFilterConfig) != HAL_OK) {
        return -1;
    }

    if (HAL_FDCAN_Start(&g_fdcan1) != HAL_OK) {
        return -1;
    }

    CAN_App_InitData(&g_can_ctx);
    return 0;
}

// Initalize the CAN content
void CAN_App_InitData(can_app_ctx_t *ctx)
{
    if (!ctx) return;

    memset(ctx, 0, sizeof(*ctx));

    ctx->thermistors.num_active = 0;

    /* Prevent immediate sends on startup by stamping timestamps to now */
    uint32_t now = HAL_GetTick();
    ctx->last_claim_ms = now;
    ctx->last_bms_ms = now;
    ctx->last_general_ms = now;

    ctx->therm_index = 0; 

    uint32_t err = HAL_FDCAN_GetError(&g_fdcan1);
    LOG_INFO("ERROR CODE AFTER INIT: 0x%08lX", (unsigned long)err);
}

//TODO: This should be changed to use an interrupt instead of the next sample and now thing.. 
void CAN_ServiceTask(void)
{
    CAN_SendMessages();
}



/* -------------------------------------------------------------------------- */
/* CAN Communication Helper Functions                           */
/* -------------------------------------------------------------------------- */

/** Default kernel clock for the FDCAN peripheral. */
#ifndef FDCAN_KERNEL_CLK_HZ
#define FDCAN_KERNEL_CLK_HZ 85000000UL
#endif

/** Attempt to derive timing parameters for the requested bitrate. */
int CAN_FindTiming(uint32_t fclk_hz, uint32_t bitrate_hz,
                          uint32_t *out_presc, uint32_t *out_tseg1,
                          uint32_t *out_tseg2, uint32_t *out_sjw)
{
    for (uint32_t tq = 8U; tq <= 25U; ++tq) {
        for (uint32_t presc = 1U; presc <= 512U; ++presc) {
            uint64_t num = (uint64_t)fclk_hz;
            uint64_t den = (uint64_t)presc * (uint64_t)tq;
            if (den == 0U) {
                continue;
            }
            if (num % den != 0U) {
                continue;
            }
            uint32_t calc = (uint32_t)(num / den);
            if (calc != bitrate_hz) {
                continue;
            }
            uint32_t tseg_total = tq - 1U;
            uint32_t tseg2 = tseg_total / 4U;
            if (tseg2 < 1U) {
                tseg2 = 1U;
            }
            uint32_t tseg1 = tseg_total - tseg2;
            if (tseg1 < 1U || tseg1 > 255U) {
                continue;
            }
            uint32_t sjw;
            if (tseg2 >= 4U) {
                sjw = 4U;
            } else {
                sjw = tseg2;
            }
            *out_presc = presc;
            *out_tseg1 = tseg1;
            *out_tseg2 = tseg2;
            *out_sjw = sjw;
            return 0;
        }
    }
    return -1;
}



/* Send an extended (29-bit) CAN frame for J1939/BMS messages */
int CAN_Comm_SendExt(uint32_t ext_id, const uint8_t *data, uint8_t len)
{
    if (len > 8U) {
        return -1;
    }

    FDCAN_TxHeaderTypeDef txHeader;
    txHeader.Identifier = ext_id & 0x1FFFFFFFU;
    txHeader.IdType = FDCAN_EXTENDED_ID;
    txHeader.TxFrameType = FDCAN_DATA_FRAME;

    switch (len) {
        case 0: txHeader.DataLength = FDCAN_DLC_BYTES_0; break;
        case 1: txHeader.DataLength = FDCAN_DLC_BYTES_1; break;
        case 2: txHeader.DataLength = FDCAN_DLC_BYTES_2; break;
        case 3: txHeader.DataLength = FDCAN_DLC_BYTES_3; break;
        case 4: txHeader.DataLength = FDCAN_DLC_BYTES_4; break;
        case 5: txHeader.DataLength = FDCAN_DLC_BYTES_5; break;
        case 6: txHeader.DataLength = FDCAN_DLC_BYTES_6; break;
        case 7: txHeader.DataLength = FDCAN_DLC_BYTES_7; break;
        default: txHeader.DataLength = FDCAN_DLC_BYTES_8; break;
    }

#if defined(FDCAN_ERROR_STATE_ACTIVE)
    txHeader.ErrorStateIndicator = FDCAN_ERROR_STATE_ACTIVE;
#elif defined(FDCAN_ERROR_STATE_NORMAL)
    txHeader.ErrorStateIndicator = FDCAN_ERROR_STATE_NORMAL;
#else
    txHeader.ErrorStateIndicator = 0;
#endif

    txHeader.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader.FDFormat = FDCAN_CLASSIC_CAN;
    txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    txHeader.MessageMarker = 0;


        if (HAL_FDCAN_AddMessageToTxFifoQ(&g_fdcan1, &txHeader, (uint8_t *)data) != HAL_OK) {
        LOG_ERROR("FDCAN TX enqueue failed ID=0x%08lX, HAL ErrorCode=0x%08lX, state=%lu",
                  (unsigned long)ext_id,
                  (unsigned long)g_fdcan1.ErrorCode,
                  (unsigned long)HAL_FDCAN_GetState(&g_fdcan1));
        return -1;
    }
    return 0;
}
