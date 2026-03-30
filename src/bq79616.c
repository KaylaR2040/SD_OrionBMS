/* ===========================================================================
 * File: bq79616.c
 * Description: Driver for TI BQ79616 battery monitor IC.
 *
 * Notes:
 *   - This driver is designed for a single stacked device; multi-device support
 *     is possible but untested.
 *   - All functions are thread-compatible but not thread-safe; caller must
 *     serialize access to the bus and device.
 * ===========================================================================
 */

#include "master.h"


/* Key register addresses (word addresses from datasheet) */

/* Table 9-10 REQ_TYPE encoding (bits 6:4) */
typedef enum
{
    BQ_REQ_SINGLE_READ          = 0u, /* 000 */
    BQ_REQ_SINGLE_WRITE         = 1u, /* 001 */
    BQ_REQ_STACK_READ           = 2u, /* 010 */
    BQ_REQ_STACK_WRITE          = 3u, /* 011 */
    BQ_REQ_BROADCAST_READ       = 4u, /* 100 */
    BQ_REQ_BROADCAST_WRITE      = 5u, /* 101 */
    BQ_REQ_BROADCAST_WRITE_REV  = 6u  /* 110 */
    /* 111 reserved */
} bq_req_type_t;

/* Frame size bounds (kept conservative) */
#define BQ_TX_MAX   (1u + 1u + 2u + BQ_READ_MAX_BYTES + 2u) /* enough for worst request */
#define BQ_RX_MAX   (1u + 1u + 2u + BQ_READ_MAX_BYTES + 2u) /* worst response */


static uint8_t tx_buf[BQ_TX_MAX];
static uint8_t rx_buf[BQ_RX_MAX];

#define BQ_FIRST_READ_TIMEOUT_MS 50u

// Function Declarations
void bq_pin_tx_to_gpio(void);
void bq_pin_tx_to_uart(void);
void bq_pin_tx_set(GPIO_PinState state);
void bq_delay_us(uint32_t us);
void bq_drive_wake_pulse(uint32_t pulse_us);
void bq_enable_dwt(void);
void bq_log_hex_internal(const char *label, const uint8_t *buf, size_t len);

/* ------------------------------ CRC16-IBM --------------------------------- */
/* Reflected polynomial 0xA001, init 0xFFFF */
static const uint16_t crc16_table[256] = {
    0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
    0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
    0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
    0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
    0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
    0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
    0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
    0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
    0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
    0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
    0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
    0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
    0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
    0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
    0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
    0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};

uint16_t bq_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFu;
    for (uint16_t i = 0; i < len; i++) {
        uint8_t idx = (uint8_t)(crc ^ data[i]);
        crc = (uint16_t)((crc >> 8) ^ crc16_table[idx]);
    }
    return crc;
}

/* Validate per datasheet: CRC computed across entire frame including CRC must be 0 */
int bq_crc_validate_full_frame(const uint8_t *frame, uint16_t frame_len)
{
    if (!frame || frame_len < 2u) {
        return -1;
    }
    if (bq_crc16(frame, frame_len) == 0u) {
        return 0;
    }
    return -2;
}
/* ----------------------------------------------------------------------------- */
/* ----------------------------- WAKE SEQUENCE --------------------------------- */
/* ----------------------------------------------------------------------------- */

/* Wake bridge + stack, return 0 on success */
int BQ_Wake(uint8_t stack_count)
{
    LOG_INFO("\n=== WAKE SEQUENCE TEST ===");

    int wake_bq_status = BQ_WakeSequence(stack_count);
    if (wake_bq_status != 0) {
        LOG_ERROR("Wake sequence failed: bq_status=%ld", (long)wake_bq_status);
        return (int)wake_bq_status;
    }

    // int init_bq_status = bq79616_stack_single_init();
    // if (init_bq_status != 0) {
    //     LOG_ERROR("Stack init failed: bq_status=%d", init_bq_status);
    //     return init_bq_status;
    // }

    HAL_Delay(1000u); 
    return 0;
}

/* ----- Wake-up ping ----- */
void BQ79616_wake_ping(void){
    /* Datasheet wake: hold TX low for ~2.75 ms, then restore UART */
    HAL_UART_DeInit(&uart_bq79616);

    bq_pin_tx_to_gpio();
    bq_pin_tx_set(GPIO_LOW);
    bq_delay_us(BQ_WAKE_PULSE_US);
    bq_pin_tx_set(GPIO_HIGH);

    if (bq_uart_reinit() != 0) {
        Error_Handler();
    }
}

int bq79616_direct_wake(void)
{
    LOG_INFO("\n=== WAKE SEQUENCE TEST ===");
    LOG_INFO("Starting direct UART wake");

    HAL_UART_DeInit(&uart_bq79616);

    bq_pin_tx_to_gpio();
    bq_pin_tx_set(GPIO_HIGH);

    bq_drive_wake_pulse(BQ_WAKE_PULSE_US);

    bq_delay_us(BQ_WAKE_POST_DELAY_US);

    if (bq_uart_reinit() != 0) {
        LOG_ERROR("UART reinit failed after wake");
        return -1;
    }

    LOG_INFO("Direct UART wake complete");
    return 0;
}

/* Full UART wake sequence: two pulses, bridge ACTIVE wait, SEND_WAKE */
int BQ_WakeSequence(uint8_t stack_count)
{

    /*
     * TI SLUUC56C UART wake procedure (BQ79600/BQ79616):
     *  1) Temporarily release UART so the host can directly drive TX.
     *  2) Drive TX low for 2.75 ms twice (separated by tSU(WAKE_SHUT)).
     *  3) Wait tACTIVE so the bridge enters ACTIVE mode before UART traffic.
     *  4) Restore UART alternate function and reinit the peripheral.
     *  5) Issue CONTROL1[SEND_WAKE]=1 so the bridge forwards wake down-stack.
     *  6) Wait 11.6 ms per stacked device for wake propagation.
     */

    LOG_INFO("Driving wake pulse 1");

    HAL_UART_DeInit(&uart_bq79616);
    bq_pin_tx_to_gpio();
    bq_pin_tx_set(GPIO_PIN_SET); /* idle high before manual pulses */

    /* First wake ping */
    bq_drive_wake_pulse(BQ_WAKE_PULSE_US);

    /* Required shutdown interval before second ping */
    bq_delay_us(BQ_WAKE_SHUT_DELAY_US);

    LOG_INFO("Driving wake pulse 2");
    bq_drive_wake_pulse(BQ_WAKE_PULSE_US);

    LOG_INFO("Reinitializing UART");
    if (bq_uart_reinit() != 0) {
        LOG_ERROR("UART reinit failed after wake pulses");
        return -1;
    }

    /* Propagate wake to stacked devices via CONTROL1[SEND_WAKE] */
    uint8_t control1 = BQ_CONTROL1_SEND_WAKE_MASK;
    LOG_INFO("Sending CONTROL1[SEND_WAKE]");
    if (bq79616_write(DEVICE_ADDR, CONTROL1, &control1, 1u) != 0) {
        LOG_ERROR("Failed to write CONTROL1[SEND_WAKE]");
        return -1;
    }

    /* Optional COMM_CLEAR broadcast: clears UART state on all nodes */
    uint8_t comm_clear = 0x00u;
    (void)bq7961x_broadcast_write(CONTROL1, &comm_clear, 1u, 1000u);

    /* Wait 11.6 ms per stacked device for wake propagation */
    uint32_t prop_us = BQ_WAKE_PROP_DELAY_US_PER_DEV * (uint32_t)stack_count;
    LOG_INFO("Waiting for stack wake propagation (%lu us)", (unsigned long)prop_us);
    bq_delay_us(prop_us);

    return 0;
}

/* Single-node stack init: wake tone + address + top-of-stack setup */
int bq79616_stack_single_init(void)
{
    int bq_status;
    uint8_t data;

    /* Step 1: Wake tone + keep SEND_WAKE asserted alongside auto-address (bit0) */
    data = (uint8_t)(BQ_CONTROL1_SEND_WAKE_MASK | 0x01u); /* 0x21: SEND_WAKE + AUTO_ADDR */
    bq_status = bq79616_write(DEVICE_ADDR, CONTROL1, &data, 1u);
    if (bq_status != 0) return bq_status;
    HAL_Delay(12u);  /* Wait for propagation (11.6 ms) */

    /* Step 2: Set device address to 0 */
    data = 0x00u;
    bq_status = bq79616_write(DEVICE_ADDR, DIR0_ADDR, &data, 1u);
    if (bq_status != 0) return bq_status;

    /* Step 3: Set as top-of-stack */
    data = 0x03u;
    bq_status = bq79616_write(DEVICE_ADDR, COMM_CTRL, &data, 1u);
    if (bq_status != 0) return bq_status;

    return 0;
}




// /* ----------------------------------------------------------------------------- */
// /* --------------------------- READ CHANNEL VOLTAGE ---------------------------- */
// /* ----------------------------------------------------------------------------- */
// int BQ_Read_Channel_Voltage( adc_channel)
// {
//     // Implementation for reading channel voltage
//     return 0;
// }



/* ----------------------------- INIT byte helpers -------------------------- */
/*
 * Command INIT (Table 9-10):
 *   bit7 = 1
 *   bits6:4 = REQ_TYPE
 *   bit3 = RSVD (0)
 *   bits2:0 = DATA_SIZE (000->1 byte ... 111->8 bytes)
 *
 * data_bytes:
 *   - writes: number of write bytes (1..8)
 *   - reads:  must be 1 because DATA[0] carries (bytes_to_return - 1)
 */
uint8_t bq_init_cmd(bq_req_type_t req, uint8_t data_bytes)
{
    if (data_bytes < 1u) data_bytes = 1u;
    if (data_bytes > 8u) data_bytes = 8u;

    uint8_t data_size_bits = (uint8_t)((data_bytes - 1u) & 0x07u);
    return (uint8_t)((BQ_CMD_FRAME_TYPE << 7) | (((uint8_t)req & 0x07u) << 4) | data_size_bits);
}

/*
 * Response INIT (Table 9-10 response side):
 *   bit7 = 0
 *   bits6:0 = RESPONSE_BYTE (0x00->1 byte ... 0x7F->128 bytes)
 */
uint8_t bq_rsp_len_from_init(uint8_t init_byte, uint8_t *out_len)
{
    if (!out_len) return 1u;
    if ((init_byte & 0x80u) != 0u) return 2u; /* not a response frame */
    uint8_t n = (uint8_t)(init_byte & 0x7Fu);
    *out_len = (uint8_t)(n + 1u);
    return 0u;
}

/* ---------------------------- UART helpers (HAL) -------------------------- */

int bq_uart_tx(const uint8_t *buf, uint16_t len, uint32_t timeout_ms)
{
    if (!buf || len == 0u) return -1;
    if (HAL_UART_Transmit(&uart_bq79616, (uint8_t *)buf, len, timeout_ms) == HAL_OK) {
        return 0;
    }
    return -2;
}

int bq_uart_rx(uint8_t *buf, uint16_t len, uint32_t timeout_ms)
{
    if (!buf || len == 0u) return -1;
    if (HAL_UART_Receive(&uart_bq79616, buf, len, timeout_ms) == HAL_OK) {
        return 0;
    }
    return -2;
}

int bq_uart_txrx(const uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len, uint32_t timeout_ms)
{
    if (!tx || !rx || tx_len == 0u || rx_len == 0u) {
        return -1;
    }

    /* Sequential TX then RX (half-duplex safe) */
    if (HAL_UART_Transmit(&uart_bq79616, (uint8_t *)tx, tx_len, timeout_ms) != HAL_OK) {
        return -2;
    }
    if (HAL_UART_Receive(&uart_bq79616, rx, rx_len, timeout_ms) != HAL_OK) {
        return -3;
    }
    return 0;
}

/* Public single-device wrappers */
int bq79616_write(uint8_t dev_id,
                  uint16_t reg_addr,
                  const uint8_t *data,
                  uint8_t len)
{
    return bq7961x_single_write(dev_id, reg_addr, data, len, 1000u);
}


/* Optional: explicitly set baudrate on the configured handle */
int bq7961x_uart_set_baud(uint32_t baudrate)
{
    uart_bq79616.Init.BaudRate = baudrate;
    if (HAL_UART_DeInit(&uart_bq79616) != HAL_OK) return -1;
    if (HAL_UART_Init(&uart_bq79616) != HAL_OK) return -2;
    return 0;
}

void bq_pin_tx_to_gpio(void)
{
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_4);
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_4;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &g);
}

void bq_pin_tx_set(GPIO_PinState state)
{
    HAL_GPIO_WritePin(BQ_GPIO_PORT, BQ_GPIO_PIN, state);
}

void bq_pin_tx_to_uart(void)
{
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_4);
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_4;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOC, &g);
}

int bq_uart_reinit(void)
{
    bq_pin_tx_to_uart();
    if (HAL_UART_Init(&uart_bq79616) == HAL_OK) {
        return 0;
    }
    return -1;
}




void bq_drive_wake_pulse(uint32_t pulse_us)
{
    /* Host manually drives TX low to generate UART wake ping per TI spec */
    bq_pin_tx_set(GPIO_LOW);
    bq_delay_us(pulse_us);
    bq_pin_tx_set(GPIO_HIGH);
}


void bq_enable_dwt(void)
{
    /* Enable DWT cycle counter for sub-ms delays if not already on */
    // TODO: Verify this code change to make it compile
    // if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_Tbq_statusENA_Msk)) {
    //     CoreDebug->DEMCR |= CoreDebug_DEMCR_Tbq_statusENA_Msk;
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

void bq_delay_us(uint32_t us)
{
    bq_enable_dwt();
    uint32_t cycles = (SystemCoreClock / 1000000u) * us;
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles) {
        __NOP();
    }
}

void bq_log_hex(const char *label, const uint8_t *buf, size_t len)
{
    const char *tag = (label) ? label : "BUF";
    if (!buf || len == 0u) {
        LOG_INFO("%s: <empty>", tag);
        return;
    }

    char line[192];
    size_t pos = 0u;
    pos += (size_t)snprintf(line + pos, sizeof(line) - pos, "%s (%uB): ", tag, (unsigned)len);

    for (size_t i = 0; i < len; i++) {
        if ((pos + 4u) >= sizeof(line)) {
            pos += (size_t)snprintf(line + pos, sizeof(line) - pos, "...");
            break;
        }
        pos += (size_t)snprintf(line + pos, sizeof(line) - pos, "%02X ", buf[i]);
    }

    LOG_INFO("%s", line);
}

/* ---------------------- Frame builder (command frames) -------------------- */

int bq79616_build_single_read_frame(uint16_t reg_addr,
                                    uint8_t n_minus_1,
                                    uint8_t *out,
                                    size_t out_max,
                                    size_t *out_len)
{
    if (!out || !out_len) {
        return -1;
    }
    if (out_max < BQ_SINGLE_READ_FRAME_LEN) {
        return -2;
    }

    out[0] = 0x80u; /* INIT: Single Device Read, DATA_SIZE=1 byte */
    out[1] = (uint8_t)(DEVICE_ADDR & BQ_DEV_ADDR_MAX);
    out[2] = (uint8_t)((reg_addr >> 8) & 0xFFu);
    out[3] = (uint8_t)(reg_addr & 0xFFu);
    out[4] = n_minus_1; /* number of bytes to read minus one */

    /* CRC-16-IBM across bytes [0..4], appended as [CRC_H][CRC_L] */
    uint16_t crc = bq_crc16_ibm(out, 5u);
    out[5] = (uint8_t)((crc >> 8) & 0xFFu);
    out[6] = (uint8_t)(crc & 0xFFu);

    *out_len = BQ_SINGLE_READ_FRAME_LEN;
    return 0;
}

int bq79616_read_partid_once(uint8_t *partid_out)
{
    uint8_t tx_frame[BQ_SINGLE_READ_FRAME_LEN] = {0};
    uint8_t rx_frame[BQ_SINGLE_READ_FRAME_LEN] = {0};
    size_t tx_len = 0u;

    LOG_INFO("=== BQ79616 bring-up: PARTID read ===");

    int bq_status = bq79616_build_single_read_frame(BQ_PARTID_REG, 0x00u, tx_frame, sizeof(tx_frame), &tx_len);
    if (bq_status != 0) {
        LOG_ERROR("PARTID: frame build failed (bq_status=%d)", bq_status);
        return bq_status;
    }
    LOG_INFO("PARTID: frame built (reg=0x%04X, len=%u)", BQ_PARTID_REG, (unsigned)tx_len);
    bq_log_hex("TX frame", tx_frame, tx_len);

    LOG_INFO("PARTID: expecting RX len=%u bytes (INIT+DEV+REG+DATA+CRC) dev=0x%02X reg=0x%04X data_len=1",
             BQ_SINGLE_READ_FRAME_LEN, DEVICE_ADDR, BQ_PARTID_REG);

    LOG_INFO("PARTID: TX/RX start (one transaction)");
    bq_status = bq_uart_txrx(tx_frame, (uint16_t)tx_len, rx_frame, BQ_SINGLE_READ_FRAME_LEN, BQ_FIRST_READ_TIMEOUT_MS);
    if (bq_status != 0) {
        LOG_ERROR("PARTID: TX/RX failed (bq_status=%d)", bq_status);
        return bq_status;
    }
    LOG_INFO("PARTID: TX/RX complete (%u bytes RX)", BQ_SINGLE_READ_FRAME_LEN);
    bq_log_hex("RX frame", rx_frame, BQ_SINGLE_READ_FRAME_LEN);

    uint16_t crc_calc = bq_crc16_ibm(rx_frame, 5u);
    uint16_t crc_frame = (uint16_t)((rx_frame[5] << 8) | rx_frame[6]);
    if (crc_calc == crc_frame) {
        LOG_INFO("PARTID: CRC PASS (calc=0x%04X frame=0x%04X)", crc_calc, crc_frame);
    } else {
        LOG_ERROR("PARTID: CRC FAIL (calc=0x%04X frame=0x%04X)", crc_calc, crc_frame);
        return -20;
    }

    uint8_t init = rx_frame[0];
    if (init != 0x00u) {
        LOG_WARN("PARTID: unexpected INIT 0x%02X (expected 0x00 for 1-byte response)", init);
    }

    uint8_t rsp_dev = (uint8_t)(rx_frame[1] & BQ_DEV_ADDR_MAX);
    if (rsp_dev != (DEVICE_ADDR & BQ_DEV_ADDR_MAX)) {
        LOG_ERROR("PARTID: DEV_ADDR mismatch (rx=0x%02X exp=0x%02X)", rsp_dev, DEVICE_ADDR);
        return -21;
    }

    uint16_t rsp_reg = (uint16_t)((rx_frame[2] << 8) | rx_frame[3]);
    if (rsp_reg != BQ_PARTID_REG) {
        LOG_ERROR("PARTID: REG mismatch (rx=0x%04X exp=0x%04X)", rsp_reg, BQ_PARTID_REG);
        return -22;
    }

    uint8_t partid = rx_frame[4];
    if (partid_out) {
        *partid_out = partid;
    }

    LOG_INFO("PARTID: expected=0x%02X", BQ_PARTID_EXPECTED);
    LOG_INFO("PARTID: received=0x%02X", partid);

    if (partid == BQ_PARTID_EXPECTED) {
        LOG_INFO("PARTID: PASS");
        return 0;
    }

    LOG_ERROR("PARTID: FAIL");
    return -23;
}

int bq_build_cmd_frame(uint8_t *out,
                              uint16_t out_max,
                              bq_req_type_t req,
                              uint8_t dev_addr,         /* used only for SINGLE commands */
                              uint16_t reg_addr,
                              const uint8_t *data,
                              uint16_t data_len,        /* writes: 1..8; reads: must be 1 */
                              uint16_t *out_len)
{
    if (!out || !out_len) return -1;
    *out_len = 0u;

    bool include_dev = (req == BQ_REQ_SINGLE_READ || req == BQ_REQ_SINGLE_WRITE);

    if (include_dev) {
        if (dev_addr > BQ_DEV_ADDR_MAX) return -2;
    }

    if (data_len < 1u) return -3;

    /* Command DATA_SIZE is 3 bits => max 8 data bytes in a command frame */
    if (data_len > BQ_WRITE_MAX_BYTES) return -4;

    uint16_t include_dev_len = 0u;
    if (include_dev) {
        include_dev_len = 1u;
    }

    uint16_t needed = (uint16_t)(1u + include_dev_len + 2u + data_len + 2u);
    if (needed > out_max) return -5;

    uint16_t idx = 0u;

    out[idx++] = bq_init_cmd(req, (uint8_t)data_len);

    if (include_dev) {
        /* Table 9-11: bits7:6 should be 0, bits5:0 is device address */
        out[idx++] = (uint8_t)(dev_addr & BQ_DEV_ADDR_MAX);
    }

    /* Table 9-12: REG_ADR is 2 bytes (MSB then LSB) */
    out[idx++] = (uint8_t)((reg_addr >> 8) & 0xFFu);
    out[idx++] = (uint8_t)(reg_addr & 0xFFu);

    /* Table 9-13: DATA bytes */
    if (data && data_len) {
        memcpy(&out[idx], data, data_len);
    } else {
        memset(&out[idx], 0x00, data_len);
    }
    idx = (uint16_t)(idx + data_len);

    /*
     * CRC bytes (datasheet):
     *   CRC remainder is appended so receiver CRC over entire frame (incl CRC) becomes 0.
     * We append CRC LSB then CRC MSB (matches TI ref style / common IBM CRC framing).
     */
    uint16_t crc = bq_crc16(out, idx);
    out[idx++] = (uint8_t)(crc & 0xFFu);         /* CRC_L */
    out[idx++] = (uint8_t)((crc >> 8) & 0xFFu);  /* CRC_H */

    *out_len = idx;
    return 0;
}

/* ---------------------- Internal send command APIs ------------------------ */
int bq7961x_single_write(uint8_t dev_addr,
                         uint16_t reg_addr,
                         const uint8_t *data,
                         uint8_t len,
                         uint32_t timeout_ms)
{
    if (len < 1u || len > 8u) return -1;

    uint16_t frame_len = 0u;
    int bq_status = bq_build_cmd_frame(tx_buf, sizeof(tx_buf),
                                BQ_REQ_SINGLE_WRITE,
                                dev_addr,
                                reg_addr,
                                data,
                                len,
                                &frame_len);
    if (bq_status != 0) return bq_status;

    return bq_uart_tx(tx_buf, frame_len, timeout_ms);
}

int bq7961x_single_read(uint8_t dev_addr,
                        uint16_t reg_addr,
                        uint8_t *out,
                        uint8_t out_len,
                        uint32_t timeout_ms)
{
    if (!out) return -1;
    if (out_len < 1u || out_len > BQ_READ_MAX_BYTES) return -2;

    /*
     * Per Table 9-13 (READ):
     *   DATA[0] specifies (bytes_to_return - 1)
     * So read request always has exactly 1 data byte.
     */
    uint8_t req_data = (uint8_t)(out_len - 1u);

    uint16_t cmd_len = 0u; // To be set by bq_build_cmd_frame
    int bq_status = bq_build_cmd_frame(tx_buf, sizeof(tx_buf),
                                BQ_REQ_SINGLE_READ,
                                dev_addr,
                                reg_addr,
                                &req_data,
                                1u,
                                &cmd_len);
    if (bq_status != 0) return bq_status;

    /* TX read request */
    bq_status = bq_uart_tx(tx_buf, cmd_len, timeout_ms);
    if (bq_status != 0) return bq_status;

    /*
     * RX response:
     *   First byte is INIT (bit7 must be 0). It contains (response_bytes-1) in bits6:0.
     */
    uint8_t init = 0u;
    bq_status = bq_uart_rx(&init, sizeof(init), timeout_ms);
    if (bq_status != 0) return -10;

    uint8_t rsp_data_len = 0u;
    if (bq_rsp_len_from_init(init, &rsp_data_len) != 0u) {
        return -11;
    }

    /* Ensure response length matches what you requested */
    if (rsp_data_len != out_len) {
        return -12;
    }

    /*
     * Remaining bytes after INIT:
     *   DEV(1) + REG(2) + DATA(out_len) + Cbq_status(2)
     */
    uint16_t remain = (uint16_t)(1u + 2u + rsp_data_len + 2u);
    if ((1u + remain) > sizeof(rx_buf)) return -13;

    rx_buf[0] = init;
    bq_status = bq_uart_rx(&rx_buf[1], remain, timeout_ms);
    if (bq_status != 0) return -14;

    uint16_t full_len = (uint16_t)(1u + remain);

    /* Cbq_status check across entire response (including Cbq_status) must be 0 */
    bq_status = bq_crc_validate_full_frame(rx_buf, full_len);
    if (bq_status != 0) return -15;

    /* Basic field checks */
    uint8_t rsp_dev = (uint8_t)(rx_buf[1] & BQ_DEV_ADDR_MAX);
    uint16_t rsp_reg = (uint16_t)((rx_buf[2] << 8) | rx_buf[3]);

    if (rsp_dev != (dev_addr & BQ_DEV_ADDR_MAX)) return -16;
    if (rsp_reg != reg_addr) return -17;

    /* Copy payload */
    memcpy(out, &rx_buf[4], rsp_data_len);
    return 0;
}

int bq7961x_stack_read(uint8_t dev_addr,
                              uint16_t reg_addr,
                              uint8_t *out,
                              uint8_t out_len,
                              uint32_t timeout_ms)
{
    /* Stack read here reuses the single-read helper; kept for clarity */
    return bq7961x_single_read(dev_addr, reg_addr, out, out_len, timeout_ms);
}

int bq7961x_stack_write(uint16_t reg_addr,
                        const uint8_t *data,
                        uint8_t len,
                        uint32_t timeout_ms)
{
    if (len < 1u || len > 8u) return -1;

    uint16_t frame_len = 0u;
    int bq_status = bq_build_cmd_frame(tx_buf, sizeof(tx_buf),
                                BQ_REQ_STACK_WRITE,
                                0u,
                                reg_addr,
                                data,
                                len,
                                &frame_len);
    if (bq_status != 0) return bq_status;

    return bq_uart_tx(tx_buf, frame_len, timeout_ms);
}

int bq7961x_stack_read_request(uint16_t reg_addr,
                               uint8_t bytes_to_return,
                               uint32_t timeout_ms)
{
    if (bytes_to_return < 1u || bytes_to_return > BQ_READ_MAX_BYTES) return -1;

    uint8_t req_data = (uint8_t)(bytes_to_return - 1u);

    uint16_t frame_len = 0u;
    int bq_status = bq_build_cmd_frame(tx_buf, sizeof(tx_buf),
                                BQ_REQ_STACK_READ,
                                0u,
                                reg_addr,
                                &req_data,
                                1u,
                                &frame_len);
    if (bq_status != 0) return bq_status;

    return bq_uart_tx(tx_buf, frame_len, timeout_ms);
}

int bq7961x_broadcast_write(uint16_t reg_addr,
                            const uint8_t *data,
                            uint8_t len,
                            uint32_t timeout_ms)
{
    if (len < 1u || len > 8u) return -1;

    uint16_t frame_len = 0u;
    int bq_status = bq_build_cmd_frame(tx_buf, sizeof(tx_buf),
                                BQ_REQ_BROADCAST_WRITE,
                                0u,
                                reg_addr,
                                data,
                                len,
                                &frame_len);
    if (bq_status != 0) return bq_status;

    return bq_uart_tx(tx_buf, frame_len, timeout_ms);
}

int bq7961x_broadcast_write_reverse(uint16_t reg_addr,
                                    const uint8_t *data,
                                    uint8_t len,
                                    uint32_t timeout_ms)
{
    if (len < 1u || len > 8u) return -1;

    uint16_t frame_len = 0u;
    int bq_status = bq_build_cmd_frame(tx_buf, sizeof(tx_buf),
                                BQ_REQ_BROADCAST_WRITE_REV,
                                0u,
                                reg_addr,
                                data,
                                len,
                                &frame_len);
    if (bq_status != 0) return bq_status;

    return bq_uart_tx(tx_buf, frame_len, timeout_ms);
}

int bq7961x_broadcast_write_consecutive(uint16_t reg_addr,
                                               uint8_t stack_count,
                                               uint32_t timeout_ms)
{
    /* Datasheet consecutive auto-address: broadcast starting at 0 */
    (void)stack_count; /* reserved for future per-device verification */
    uint8_t start_addr = 0u;
    return bq7961x_broadcast_write(reg_addr, &start_addr, 1u, timeout_ms);
}

/* ---------------------- Convenience init (optional) ------------------------ */

int bq7961x_uart_init(void)
{
    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0, sizeof(rx_buf));
    return 0;
}

/* Read cell voltage from ADC (keeps device ACTIVE via periodic communication) */
int BQ79616_read_cell_voltage(uint8_t dev_addr, uint8_t cell_channel, uint16_t *voltage_mv)
{
    int bq_status;
    uint8_t data[2] = {0};

    if (!voltage_mv) return -1;

    /* Cell voltage registers (2 bytes each):
       VC1=0x0014, VC2=0x0016, VC3=0x0018, VC4=0x001A, etc.
       Each channel offset by 2 bytes */
    uint16_t reg_addr = 0x0014u + (cell_channel * 2u);

    /* Read 2 bytes from the voltage register */
    bq_status = bq7961x_single_read(dev_addr, reg_addr, data, sizeof(data), BQ_FAST_TIMEOUT_MS);
    if (bq_status != 0) {
        return bq_status;
    }

    /* Convert raw ADC value to millivolts
       BQ79612 ADC resolution: 195.3 µV per LSB
       Formula: (raw_adc * 1953) / 10000 */
    uint16_t raw_adc = (data[0] << 8) | data[1];
    *voltage_mv = (raw_adc * 1953u) / 10000u;

    return 0;
}

//TODO: Fix and use an interrupt not the last time
bool BQ_ServiceTask(void) 
{
    static uint32_t last_keep_alive = 0;
    static uint32_t fault_check_tick = 0;
    uint32_t now = HAL_GetTick();
    
    if ((now - last_keep_alive) >= 20u) {  /* Every 20ms: beat comm timeout */
        last_keep_alive = now;

        uint8_t keep = BQ_CONTROL1_SEND_WAKE_MASK | 0x01u; /* 0x21 */
        int bq_status = bq79616_write(DEVICE_ADDR, CONTROL1, &keep, 1u);

        /* If write completely fails locally */
        if (bq_status != 0) {
            LOG_ERROR("BQ Keep-Alive Write FAILED! Disabling BQ task.");
            return false; 
        }

        /* Every 200 ms, check FAULT using a VERY FAST timeout so we don't freeze */
        if ((now - fault_check_tick) >= 200u) {
            fault_check_tick = now;
            uint8_t fault_sys = 0, fault_comm1 = 0;
            
            int fault_sys_bq_status = bq7961x_single_read(DEVICE_ADDR, FAULT_SYS, &fault_sys, sizeof(fault_sys), BQ_FAST_TIMEOUT_MS);
            int fault_comm1_bq_status = bq7961x_single_read(DEVICE_ADDR, FAULT_COMM1, &fault_comm1, sizeof(fault_comm1), BQ_FAST_TIMEOUT_MS);
            
            /* If the Read times out, the chip died or disconnected -> Disable Task! */
            if (fault_sys_bq_status != 0 || fault_comm1_bq_status != 0) {
                LOG_ERROR("BQ Read FAILED! Comm lost. Disabling BQ task.");
                return false; 
            }

            /* Log actual faults if read succeeded */
            if (fault_sys | fault_comm1) {
                LOG_ERROR("FAULT_SYS=0x%02X FAULT_COMM1=0x%02X", fault_sys, fault_comm1);
            }
        }
    }
    
    return true; /* Communication is still good, keep running */
}


#define RST_SYS_BIT        0x01u
#define RST_PWR_BIT        0x02u

#define RST_COMM1_BIT        0x01u
#define RST_COMM2_BIT        0x02u
#define RST_COMM3_HB_BIT     0x04u
#define RST_COMM3_FTONE_BIT  0x08u
#define RST_COMM3_FCOMM_BIT  0x10u

int bq79616_clear_startup_faults(void)
{
    int status;
    uint8_t val;

    LOG_INFO("Reading FAULT_SUMMARY before clear...");
    status = bq7961x_single_read(DEVICE_ADDR, FAULT_SUMMARY, &val, sizeof(val), BQ_FAST_TIMEOUT_MS);
    if (status != 0) {
        LOG_ERROR("Failed to read FAULT_SUMMARY");
        return status;
    }

    LOG_INFO("FAULT_SUMMARY before clear = 0x%02X", val);

    /* Optional detail reads */
    (void)bq7961x_single_read(DEVICE_ADDR, FAULT_SYS, &val, sizeof(val), BQ_FAST_TIMEOUT_MS);
    LOG_INFO("FAULT_SYS   = 0x%02X", val);

    (void)bq7961x_single_read(DEVICE_ADDR, FAULT_PWR1, &val, sizeof(val), BQ_FAST_TIMEOUT_MS);
    LOG_INFO("FAULT_PWR1  = 0x%02X", val);

    (void)bq7961x_single_read(DEVICE_ADDR, FAULT_COMM1, &val, sizeof(val), BQ_FAST_TIMEOUT_MS);
    LOG_INFO("FAULT_COMM1 = 0x%02X", val);

    (void)bq7961x_single_read(DEVICE_ADDR, FAULT_COMM2, &val, sizeof(val), BQ_FAST_TIMEOUT_MS);
    LOG_INFO("FAULT_COMM2 = 0x%02X", val);

    (void)bq7961x_single_read(DEVICE_ADDR, FAULT_COMM3, &val, sizeof(val), BQ_FAST_TIMEOUT_MS);
    LOG_INFO("FAULT_COMM3 = 0x%02X", val);

    /* Clear system + power faults */
    uint8_t rst1 = (uint8_t)(RST_SYS_BIT | RST_PWR_BIT);
    status = bq79616_write(DEVICE_ADDR, FAULT_RST1, &rst1, 1u);
    if (status != 0) {
        LOG_ERROR("Failed to write FAULT_RST1");
        return status;
    }
    LOG_INFO("Wrote FAULT_RST1 = 0x%02X", rst1);

    /* Clear communication-related faults */
    uint8_t rst2 = (uint8_t)(
        RST_COMM1_BIT |
        RST_COMM2_BIT |
        RST_COMM3_HB_BIT |
        RST_COMM3_FTONE_BIT |
        RST_COMM3_FCOMM_BIT
    );
    status = bq79616_write(DEVICE_ADDR, FAULT_RST2, &rst2, 1u);
    if (status != 0) {
        LOG_ERROR("Failed to write FAULT_RST2");
        return status;
    }
    LOG_INFO("Wrote FAULT_RST2 = 0x%02X", rst2);

    HAL_Delay(2u);

    status = bq7961x_single_read(DEVICE_ADDR, FAULT_SUMMARY, &val, sizeof(val), BQ_FAST_TIMEOUT_MS);
    if (status != 0) {
        LOG_ERROR("Failed to read FAULT_SUMMARY after clear");
        return status;
    }

    LOG_INFO("FAULT_SUMMARY after clear = 0x%02X", val);

    if (val != 0u) {
        LOG_WARN("Fault is still present; underlying condition likely still exists");
        return 1;
    }

    LOG_INFO("Fault clear successful");
    return 0;
}

