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

#define BQ_CMD_FRAME_TYPE        (1u)
#define BQ_RSP_FRAME_TYPE        (0u)

#define BQ_STACK_COUNT   1u
#define BQ_FAST_TIMEOUT_MS 1000u 

/* TI reference code compatibility (frame write/read types) */
#define FRMWRT_SGL_R        0x00u
#define FRMWRT_SGL_W        0x10u
#define FRMWRT_STK_R        0x20u
#define FRMWRT_STK_W        0x30u
#define FRMWRT_ALL_R        0x40u
#define FRMWRT_ALL_W        0x50u
#define FRMWRT_REV_ALL_W    0x60u

/* ------------------------- Wake timing (TI SLUUC56C) -------------------- */

#define BQ_WAKE_PULSE_US              2750u   /* TX low duration per wake ping */
#define BQ_WAKE_SHUT_DELAY_US         3500u   /* tSU(WAKE_SHUT) between pings */
#define BQ_WAKE_ACTIVE_DELAY_US       3500u   /* bridge ACTIVE entry after pings */
#define BQ_WAKE_PROP_DELAY_US_PER_DEV 11600u  /* 11.6 ms per stacked device */

/* ------------------------- Device Addressing ------------------------------ */
#define BQ_DUMMY_VALUE  0x00u
#define BQ_ENABLE_CONTROL1 0x00u
#define BQ_COMM_CTRL_VALUE 0x02u
#define BQ_TOP_OF_STACK_VALUE 0x03u
#define DEVICE_ADDR 0x01u  /* Use factory default address */
#define DEV_CONF1 0x2001u // DEVCONF=0x02, REG_ADDR=0x001, word address 0x2001
#define BQ_WAKE_POST_DELAY_US  10000u // 10mS - per Datasheet (pg. 17), wait after wake pulse before UART traffic

/* Direct bring-up helpers */
#define BQ_PARTID_EXPECTED       0x21u
#define BQ_PARTID_REG            PARTID
#define BQ_SINGLE_READ_FRAME_LEN 7u

int BQ_Wake(uint8_t stack_count);

int bq79616_init(void);

int bq79616_write(uint8_t dev_id,
                  uint16_t reg_addr,
                  const uint8_t *data,
                  uint8_t len);

int bq79616_transact(uint8_t dev_id,
                     uint16_t reg_addr,
                     const uint8_t *tx,
                     uint8_t tx_len,
                     uint8_t *rx,
                     uint8_t rx_len);

uint16_t bq79616_crc16(const uint8_t *data, uint16_t len);
uint16_t bq_crc16_ibm(const uint8_t *data, uint16_t len);
void bq_log_hex(const char *label, const uint8_t *buf, size_t len);

int bq79616_build_single_read_frame(uint16_t reg_addr,
                                    uint8_t n_minus_1,
                                    uint8_t *out,
                                    size_t out_max,
                                    size_t *out_len);
int bq79616_read_partid_once(uint8_t *partid_out);

int bq79616_get_part_id(uint8_t *part_id);

void BQ79616_wake_ping(void);
int BQ_WakeSequence(uint8_t stack_count);



int bq79616_stack_single_init(void);
int bq79616_auto_address_single(void);
int bq79616_config_main_adc(void);
int bq79616_init_device(void);
int bq79616_read_all_cells(uint16_t *out_mv, size_t cell_count);
int bq79616_log_fault_registers(void);
int bq79616_update_cust_crc(void);
void Wake79616(void);
void AutoAddress(void);
void delayus(uint16_t us);
void delayms(uint16_t ms);


/* Read cell voltage (keeps device alive via periodic UART communication) */
int BQ79616_read_cell_voltage(uint8_t dev_addr, uint8_t cell_channel, uint16_t *voltage_mv);

/* Broadcast write - used for COMM_CLEAR and other broadcast commands */
int bq7961x_broadcast_write(uint16_t reg_addr,
                            const uint8_t *data,
                            uint8_t len,
                            uint32_t timeout_ms);

bool BQ_ServiceTask(void);

/* ----------------------- BQ Interfacing Functions ----------------------- */
int bq_uart_tx(const uint8_t *buf, uint16_t len, uint32_t timeout_ms);
int bq_uart_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms);
int bq_uart_txrx(const uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len, uint32_t timeout_ms);
int bq_uart_reinit(void);
int bq7961x_single_write(uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t len, uint32_t timeout_ms);
int bq7961x_single_read(uint8_t dev_addr, uint16_t reg_addr, uint8_t *out, uint8_t len, uint32_t timeout_ms);
int bq7961x_stack_read(uint8_t dev_addr, uint16_t reg_addr, uint8_t *out, uint8_t len, uint32_t timeout_ms);
int bq7961x_broadcast_write_consecutive(uint16_t reg_addr, uint8_t stack_count, uint32_t timeout_ms);
int bq7961x_stack_write(uint16_t reg_addr, const uint8_t *data, uint8_t len, uint32_t timeout_ms);
int BQ_Reset_Comms(uint8_t data, uint32_t timeout_ms);
int bq79616_direct_wake(void);
int bq79616_clear_startup_faults(void);

/* TI reference compatible helpers */
int WriteReg(uint8_t bID, uint16_t wAddr, uint64_t dwData, uint8_t bLen, uint8_t bWriteType);
int ReadReg(uint8_t bID, uint16_t wAddr, uint8_t *pData, uint8_t bLen, uint32_t timeout_ms, uint8_t bWriteType);
void ResetAllFaults(uint8_t bID, uint8_t bWriteType);
void MaskAllFaults(uint8_t bID, uint8_t bWriteType);

#endif /* BQ79616_H */
