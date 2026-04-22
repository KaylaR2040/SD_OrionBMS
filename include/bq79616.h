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

/* --- Interface mapping --- */
#define BQ_BAUDRATE 1000000U /* BQ79616 supports baudrate of 1 Mbps */
#define BQ_GPIO_PORT GPIOC
#define BQ_GPIO_PIN GPIO_PIN_4

/* --- GPIO and state constants --- */
#define GPIO_LOW GPIO_PIN_RESET
#define GPIO_HIGH GPIO_PIN_SET
#define BQ_TURN_OFF 0
#define BQ_TURN_ON  1

/* --- Core timing constants --- */
#define BQ_WAKE_PULSE_US             2500u
#define BQ_STARTUP_SETTLE_US         4000u
#define BQ_SHUTDOWN_IDLE_HIGH_US     100u
#define BQ_SHUTDOWN_PING_LOW_US   9000U   /* Datasheet tHLD_SD: 7 ms to 10 ms */
#define BQ_SHUTDOWN_ENTRY_MS      25U     /* Datasheet tSHUTDN: 20 ms, use margin */
#define BQ_LPF_SETTLE_BASE_US        38000u
#define BQ_LPF_SETTLE_PER_STACK_US   5u

/* --- Register bit masks and fixed values --- */
#define BQ_CONTROL1_SEND_WAKE_MASK    (1u << 5)   /* CONTROL1[SEND_WAKE] */
#define BQ_CONTROL1_KEEP_ALIVE_MASK   0x01u
#define BQ_FAULT_SUMMARY_CUST_CRC_BIT 0x20u
#define BQ_FAULT_MASK2_CUST_CRC_MASK  0x40u
#define BQ_FAULT_RESET_ALL_VALUE      0xFFFFu

/* ------------------------- Datasheet-driven constants ---------------------- */

#define BQ_DEV_ADDR_MAX          (0x3Fu)
#define BQ_READ_MAX_BYTES        (128u)   /* 0x7F => 128 bytes */
#define BQ_WRITE_MAX_BYTES       (8u)     /* DATA_SIZE field is 3 bits => 1..8 bytes */
#define BQ_CMD_DATA_BYTES_MIN    1u
#define BQ_CMD_DATA_BYTES_MAX    BQ_WRITE_MAX_BYTES
#define BQ_CMD_FRAME_TYPE (1u)
#define BQ_BYTE_MASK          0xFFu
#define BQ_CMD_REQ_TYPE_MASK      0x07u
#define BQ_INIT_RESPONSE_FLAG_MASK 0x80u
#define BQ_INIT_RESPONSE_LEN_MASK  0x7Fu
#define BQ_SINGLE_READ_INIT_VALUE  0x80u

#define BQ_STACK_COUNT   1u
#define BQ_FAST_TIMEOUT_MS 1000u 
#define BQ_FIRST_READ_TIMEOUT_MS 200u
#define BQ_SERVICE_TIMEOUT_MS    10u
#define BQ_FAULT_SNAPSHOT_LOG_MS 2000u

/* TI reference code compatibility (frame write/read types) */
#define FRMWRT_SGL_W 0x10u
#define FRMWRT_ALL_R 0x40u
#define FRMWRT_ALL_W 0x50u

/* --- Frame bounds and helpers --- */
#define BQ_TX_MAX   (1u + 1u + 2u + BQ_READ_MAX_BYTES + 2u)
#define BQ_RX_MAX   (1u + 1u + 2u + BQ_READ_MAX_BYTES + 2u)
#define BQ_RESPONSE_PREFIX_BYTES         4u
#define BQ_RESPONSE_FRAME_OVERHEAD_BYTES 6u
#define BQ_FRAME_CRC_MIN_BYTES          2u
#define BQ_CRC16_INIT_VALUE             0xFFFFu
#define BQ_HEX_LOG_LINE_MAX_CHARS       192u
#define BQ_HEX_LOG_ELLIPSIS_RESERVE     4u

/* --- Auto-address and communication setup --- */
#define BQ_AUTO_ADDR_DUMMY_WRITE_VALUE   0x00u
#define BQ_AUTO_ADDR_ENABLE_VALUE        0x01u
#define BQ_AUTO_ADDR_DEVICE_ADDR_VALUE   0x00u
#define BQ_COMM_CTRL_STACK_MODE_VALUE    0x02u
#define BQ_COMM_CTRL_BASE_TOP_VALUE      0x01u
#define BQ_FAULT_RST2_COMM_CLEAR_VALUE   0x03u
#define BQ_ADC_CONF1_VALUE               0x02u
#define BQ_ADC_CTRL1_CONTINUOUS_MAIN_GO  0x0Eu

/* --- Device addressing --- */
#define DEVICE_ADDR 0x01u  /* Use factory default address */

/* --- Direct bring-up helpers --- */
#define BQ_PARTID_EXPECTED 0x21u
#define BQ_PARTID_REG      PARTID
#define BQ_SINGLE_READ_FRAME_LEN 7u

/* --- Cell conversion and calibration --- */
#define BQ_ACTIVE_CELL_MIN                6u
#define BQ_ACTIVE_CELL_MAX                16u
#define BQ_ACTIVE_CELL_ENCODING_OFFSET    6u
#define BQ_CELL_REG_BASE                  0x0014u
#define BQ_CELL_REG_STRIDE_BYTES          2u
#define BQ_SINGLE_CELL_READ_BYTES         2u
#define BQ_READ_ALL_CELLS_COUNT           16u
#define BQ_READ_ALL_CELLS_RAW_BYTES       32u
#define BQ_CELL_MV_NUMERATOR              1953u
#define BQ_CELL_MV_DENOMINATOR            10000u
#define BQ_ALL_CELLS_MV_NUMERATOR         19073u
#define BQ_ALL_CELLS_MV_DENOMINATOR       100000u
#define BQ_CELL1_INDEX                    0u
#define BQ_CELL14_INDEX                   13u
#define BQ_CELL1_CAL_OFFSET_MV            1000u
#define BQ_CELL14_CAL_OFFSET_MV           1300u

/* --- Service-loop timing and limits --- */
#define BQ_KEEP_ALIVE_INTERVAL_MS         20u
#define BQ_FAULT_POLL_INTERVAL_MS         200u
#define BQ_CELL_SNAPSHOT_INTERVAL_MS      100u
#define BQ_FAULT_REG_LOG_INTERVAL_MS      1000u
#define BQ_STARTUP_FAULT_SETTLE_MS        10u
#define BQ_STARTUP_FAULT_RETRY_DELAY_MS   5u
#define BQ_POST_CLEAR_DELAY_MS            2u
#define BQ_CUST_CRC_LOAD_DELAY_MS         20u
#define BQ_COMM_FAILURE_THRESHOLD         3u
#define BQ_FAULT_POLL_PHASE_COUNT         4u

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
void bq79616_shutdown(void);
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
