# Simple CAN Message Usage

## Overview

Instead of dealing with structs and complex encoders, use these 3 simple functions:

```c
CAN_Msg_SendClaim_Simple()      // J1939 Address Claim
CAN_Msg_SendBMS_Simple()        // BMS Broadcast
CAN_Msg_SendGeneral_Simple()    // General Broadcast (cycle through thermistors)
```

---

## Example Usage in `can.c`

```c
void CAN_App_Send_Transmission(can_app_ctx_t *ctx)
{
    const uint32_t now = HAL_GetTick();
    uint8_t payload[8];
    
    // Read your ADC values (you need to implement this)
    uint16_t adc_values[10];
    ADC_ReadAllThermistors(adc_values);  // Your function
    
    /* ===== MESSAGE 1: J1939 Address Claim (every 200ms) ===== */
    if ((now - ctx->last_claim_ms) >= 200) {
        uint32_t can_id = CAN_Msg_SendClaim_Simple(payload);
        CAN_Comm_SendExt(can_id, payload, 8);
        ctx->last_claim_ms = now;
    }
    
    /* ===== MESSAGE 2: BMS Broadcast (every 100ms) ===== */
    if ((now - ctx->last_bms_ms) >= 100) {
        uint32_t can_id = CAN_Msg_SendBMS_Simple(adc_values, 10, payload);
        CAN_Comm_SendExt(can_id, payload, 8);
        ctx->last_bms_ms = now;
    }
    
    /* ===== MESSAGE 3: General Broadcast (every 100ms, rotating) ===== */
    if ((now - ctx->last_general_ms) >= 100) {
        uint32_t can_id = CAN_Msg_SendGeneral_Simple(adc_values, 10, ctx->thermistor_index, payload);
        CAN_Comm_SendExt(can_id, payload, 8);
        
        // Cycle to next thermistor (0-9, then loop)
        ctx->thermistor_index = (ctx->thermistor_index + 1) % 10;
        ctx->last_general_ms = now;
    }
}
```

---

## What Each Function Does

### 1. `CAN_Msg_SendClaim_Simple(payload)`

**What it sends**: "I'm Module 0, I exist on the bus"

**Frequency**: Every 200ms

**Returns**: CAN ID `0x18EEFF80`

**Payload**:
```
F3 00 80 F3 00 40 1E 90
```

**Usage**:
```c
uint8_t payload[8];
uint32_t can_id = CAN_Msg_SendClaim_Simple(payload);
CAN_Comm_SendExt(can_id, payload, 8);
```

---

### 2. `CAN_Msg_SendBMS_Simple(adc_values, num_thermistors, payload)`

**What it sends**: Summary of all thermistors (count, min, max, average)

**Frequency**: Every 100ms

**Returns**: CAN ID `0x1839F380`

**Payload**:
```
[0] Number of thermistors (10)
[1] Lowest temperature
[2] Highest temperature
[3] Average temperature
[4] Lowest thermistor ID
[5] Highest thermistor ID
[6] Lowest ID (always 0 for Module 0)
[7] Checksum
```

**Usage**:
```c
uint16_t adc_values[10] = { 2048, 2100, 1950, ... };  // Your ADC readings
uint8_t payload[8];
uint32_t can_id = CAN_Msg_SendBMS_Simple(adc_values, 10, payload);
CAN_Comm_SendExt(can_id, payload, 8);
```

---

### 3. `CAN_Msg_SendGeneral_Simple(adc_values, num_thermistors, thermistor_id, payload)`

**What it sends**: Individual thermistor temperature (one at a time)

**Frequency**: Every 100ms (cycle through 0-9)

**Returns**: CAN ID `0x183BF380`

**Payload**:
```
[0] Thermistor ID (0-9)
[1] This thermistor's temperature
[2] Thermistor ID relative to module
[3] Lowest temperature (across all)
[4] Highest temperature (across all)
[5] Highest thermistor ID
[6] Lowest thermistor ID
[7] Checksum
```

**Usage** (in a loop):
```c
static uint8_t current_thermistor = 0;

uint8_t payload[8];
uint32_t can_id = CAN_Msg_SendGeneral_Simple(adc_values, 10, current_thermistor, payload);
CAN_Comm_SendExt(can_id, payload, 8);

// Next thermistor for next time
current_thermistor = (current_thermistor + 1) % 10;
```

---

## Complete Example

```c
#include "can_messages.h"
#include "can.h"
#include "adc.h"

static uint8_t thermistor_index = 0;
static uint32_t last_claim_ms = 0;
static uint32_t last_bms_ms = 0;
static uint32_t last_general_ms = 0;

void CAN_SendAllMessages(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t payload[8];
    
    // Get ADC readings
    uint16_t adc_values[10];
    ADC_ReadAllThermistors(adc_values);
    
    // 1. Address Claim (every 200ms)
    if ((now - last_claim_ms) >= 200) {
        uint32_t id = CAN_Msg_SendClaim_Simple(payload);
        CAN_Comm_SendExt(id, payload, 8);
        last_claim_ms = now;
    }
    
    // 2. BMS Broadcast (every 100ms)
    if ((now - last_bms_ms) >= 100) {
        uint32_t id = CAN_Msg_SendBMS_Simple(adc_values, 10, payload);
        CAN_Comm_SendExt(id, payload, 8);
        last_bms_ms = now;
    }
    
    // 3. General Broadcast (every 100ms, rotating)
    if ((now - last_general_ms) >= 100) {
        uint32_t id = CAN_Msg_SendGeneral_Simple(adc_values, 10, thermistor_index, payload);
        CAN_Comm_SendExt(id, payload, 8);
        
        thermistor_index = (thermistor_index + 1) % 10;
        last_general_ms = now;
    }
}
```

---

## That's It!

**No structs. No complexity. Just 3 functions.**

The old complex functions still exist for testing, but you don't need to use them.
