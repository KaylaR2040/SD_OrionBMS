/*
 * ===========================================================================
 * File: bq79616.h
 * Description: Minimal BQ79616 UART interface with CRC and simple helpers.
 *
 * Exposed operations:
 *   - bq79616_write(): build + TX a write frame with CRC
 *   - bq79616_crc16(): helper for diagnostics/testing
 *   - bq_log_hex(): hex dump helper for debugging
 *
 * Notes:
 *   - Wake handling follows TI SLUUC56C (BQ79600-Q1 UART) bring-up guidance:
 *       two 2.75 ms TX-low pulses, >=3.5 ms settle, SEND_WAKE write,
 *       then 11.6 ms per stacked BQ7961x device.
 * ===========================================================================
*/

#ifndef BQ79616_H
#define BQ79616_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "B0_reg.h"

#define BQ_GPIO_PORT GPIOC
#define BQ_GPIO_PIN GPIO_PIN_4

#define GPIO_LOW GPIO_PIN_RESET
#define GPIO_HIGH GPIO_PIN_SET

#define BQ_CONTROL1_SEND_WAKE_MASK    (1u << 5)   /* CONTROL1[SEND_WAKE] */

/* ------------------------- Datasheet-driven constants ---------------------- */

#define BQ_DEV_ADDR_MAX          (0x3Fu)
#define BQ_READ_MAX_BYTES        (128u)   /* 0x7F => 128 bytes */
#define BQ_WRITE_MAX_BYTES       (8u)     /* DATA_SIZE field is 3 bits => 1..8 bytes */

#define BQ_CMD_FRAME_TYPE (1u)

#define BQ_STACK_COUNT   1u
#define BQ_FAST_TIMEOUT_MS 1000u 

/* TI reference code compatibility (frame write/read types) */
#define FRMWRT_SGL_W 0x10u
#define FRMWRT_ALL_R 0x40u
#define FRMWRT_ALL_W 0x50u

/* ------------------------- Device Addressing ------------------------------ */
#define DEVICE_ADDR 0x01u  /* Use factory default address */

/* Direct bring-up helpers */
#define BQ_PARTID_EXPECTED 0x21u
#define BQ_PARTID_REG      PARTID
#define BQ_SINGLE_READ_FRAME_LEN 7u

int bq79616_write(uint8_t dev_id, uint16_t reg_addr, const uint8_t *data, uint8_t len);
uint16_t bq79616_crc16(const uint8_t *data, uint16_t len);
void bq_log_hex(const char *label, const uint8_t *buf, size_t len);

int bq79616_read_partid_once(uint8_t *partid_out);
int bq79616_auto_address_single(void);
int bq79616_config_main_adc(void);
int bq79616_init_device(void);
int bq79616_read_all_cells(uint16_t *out_mv, size_t cell_count);
int bq79616_log_fault_registers(void);
int bq79616_update_cust_crc(void);
void bq79616_wake(void);
void delayus(uint16_t us);
void delayms(uint16_t ms);


/* Read cell voltage (keeps device alive via periodic UART communication) */
int bq79616_read_cell_voltage(uint8_t dev_addr, uint8_t cell_channel, uint16_t *voltage_mv);

/* Broadcast write - used for COMM_CLEAR and other broadcast commands */
int bq7961x_broadcast_write(uint16_t reg_addr, const uint8_t *data, uint8_t len, uint32_t timeout_ms);

bool bq79616_service_task(void);
bool bq79616_try_init(void);

/* ----------------------- BQ Interfacing Functions ----------------------- */
int bq_uart_tx(const uint8_t *buf, uint16_t len, uint32_t timeout_ms);
int bq_uart_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms);
int bq_uart_txrx(const uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len, uint32_t timeout_ms);
int bq_uart_reinit(void);
int bq7961x_single_write(uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t len, uint32_t timeout_ms);
int bq7961x_single_read(uint8_t dev_addr, uint16_t reg_addr, uint8_t *out, uint8_t len, uint32_t timeout_ms);
int bq79616_clear_startup_faults(void);

/* TI reference compatible helpers */
int WriteReg(uint8_t bID, uint16_t wAddr, uint64_t dwData, uint8_t bLen, uint8_t bWriteType);
int ReadReg(uint8_t bID, uint16_t wAddr, uint8_t *pData, uint8_t bLen, uint32_t timeout_ms, uint8_t bWriteType);
void ResetAllFaults(uint8_t bID, uint8_t bWriteType);

#endif /* BQ79616_H */
