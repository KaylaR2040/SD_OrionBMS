/*
 * ===========================================================================
 * File: bq79616.h
 * Description: Minimal BQ79616 UART interface with CRC and simple helpers.
 *
 * Exposed operations:
 *   - bq79616_write(): build + TX a write frame with CRC
 *   - bq79616_read():  build a read frame, then RX payload + CRC
 *   - bq79616_transact(): write payload and read a response in one call
 *   - bq79616_crc16(): helper for diagnostics/testing
 *   - BQ_WakeSequence(): full UART wake sequence for BQ79600 + stacked BQ79616
 *
 * Notes:
 *   - Wake handling follows TI SLUUC56C (BQ79600-Q1 UART) bring-up guidance:
 *       two 2.75 ms TX-low pulses, >=3.5 ms settle, SEND_WAKE write,
 *       then 11.6 ms per stacked BQ7961x device.
 * ===========================================================================
*/

#ifndef BQ79616_H
#define BQ79616_H

#include <stdint.h>
#include <stddef.h>

/* ------------------------- Register definitions ------------------------- */

#include "B0_reg.h"

#define BQ_CONTROL1_SEND_WAKE_MASK    (1u << 5)   /* CONTROL1[SEND_WAKE] */

/* ------------------------- Datasheet-driven constants ---------------------- */

#define BQ_DEV_ADDR_MAX          (0x3Fu)
#define BQ_READ_MAX_BYTES        (128u)   /* 0x7F => 128 bytes */
#define BQ_WRITE_MAX_BYTES       (8u)     /* DATA_SIZE field is 3 bits => 1..8 bytes */

#define BQ_CMD_FRAME_TYPE        (1u)
#define BQ_RSP_FRAME_TYPE        (0u)

#define BQ_STACK_COUNT   1u
#define BQ_FAST_TIMEOUT_MS 1000u 

/* ------------------------- Wake timing (TI SLUUC56C) -------------------- */

#define BQ_WAKE_PULSE_US              2750u   /* TX low duration per wake ping */
#define BQ_WAKE_SHUT_DELAY_US         3500u   /* tSU(WAKE_SHUT) between pings */
#define BQ_WAKE_ACTIVE_DELAY_US       3500u   /* bridge ACTIVE entry after pings */
#define BQ_WAKE_PROP_DELAY_US_PER_DEV 11600u  /* 11.6 ms per stacked device */

typedef int32_t BQ_Status_t;

int BQ_WakeAndInit(uint8_t stack_count);

int bq79616_init(void);

int bq79616_write(uint8_t dev_id,
                  uint16_t reg_addr,
                  const uint8_t *data,
                  uint8_t len);

int bq79616_read(uint8_t dev_id,
                 uint16_t reg_addr,
                 uint8_t *out,
                 uint8_t len);

int bq79616_read_timeout(uint8_t dev_id,
                         uint16_t reg_addr,
                         uint8_t *out,
                         uint8_t len,
                         uint32_t timeout_ms);

int bq79616_transact(uint8_t dev_id,
                     uint16_t reg_addr,
                     const uint8_t *tx,
                     uint8_t tx_len,
                     uint8_t *rx,
                     uint8_t rx_len);

uint16_t bq79616_crc16(const uint8_t *data, uint16_t len);

int bq79616_get_part_id(uint8_t *part_id);

void BQ79616_wake_ping(void);
void BQ79616_wake_sequence(void);
BQ_Status_t BQ_WakeSequence(uint8_t stack_count);
int BQ_WakeAndInit(uint8_t stack_count);
int bq79616_stack_single_init(void);

void BQ79616_uart_smoke_test(void);

/* Read cell voltage (keeps device alive via periodic UART communication) */
int BQ79616_read_cell_voltage(uint8_t dev_addr, uint8_t cell_channel, uint16_t *voltage_mv);

/* Broadcast write - used for COMM_CLEAR and other broadcast commands */
int bq7961x_broadcast_write(uint16_t reg_addr,
                            const uint8_t *data,
                            uint8_t len,
                            uint32_t timeout_ms);

bool BQ_ServiceTask(void);

#endif /* BQ79616_H */
