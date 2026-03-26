# CAN Message Testing - Clean & Focused

## Summary
✅ **ALL 7 TESTS PASSING**

Tests focus on the **simple helper functions** - no complexity, just testing what matters!

---

## Test Coverage

### ✅ TEST 1: ADC Conversion with Proper Rounding
- **Tests**: `CAN_Msg_ADCToTemp()`
- **What it checks**:
  - Rounding works correctly (ADC 2088 → 51°C, not 50°C)
  - Boundary values (ADC 0 = 0°C, ADC 4095 = 100°C)
- **Result**: PASS ✓

### ✅ TEST 2: Address Claim
- **Tests**: `CAN_Msg_SendClaim_Simple()`
- **What it checks**:
  - CAN ID is 0x18EEFF80
  - All 8 payload bytes match specification
- **Result**: PASS ✓

### ✅ TEST 3: BMS Broadcast - Min/Max/Average
- **Tests**: `CAN_Msg_SendBMS_Simple()`
- **What it checks**:
  - Correctly finds minimum temperature (10°C at ID 9)
  - Correctly finds maximum temperature (100°C at ID 8)
  - Correctly calculates average (55°C)
  - CAN ID is 0x1839F380
  - Payload has 10 thermistors
- **Result**: PASS ✓

### ✅ TEST 4: General Broadcast - Thermistor 0
- **Tests**: `CAN_Msg_SendGeneral_Simple()`
- **What it checks**:
  - Thermistor ID 0 (FIRST thermistor, not 1!)
  - Reports correct temperature
  - CAN ID is 0x183BF380
- **Result**: PASS ✓

### ✅ TEST 5: General Broadcast - Thermistor 9
- **Tests**: `CAN_Msg_SendGeneral_Simple()`
- **What it checks**:
  - Thermistor ID 9 (LAST thermistor, not 10!)
  - Reports correct temperature (100°C)
  - System min/max calculated across all thermistors
- **Result**: PASS ✓

### ✅ TEST 6: BMS Broadcast - All Same Temperature
- **Tests**: `CAN_Msg_SendBMS_Simple()`
- **What it checks**:
  - Edge case: all 10 thermistors at 25°C
  - Min = Max = Avg = 25°C
- **Result**: PASS ✓

### ✅ TEST 7: Checksum Calculation
- **Tests**: Checksum in `CAN_Msg_SendBMS_Simple()`
- **What it checks**:
  - Manual calculation matches function output
  - Formula: PGN (0x39) + DLC (8) + sum(payload[0..6])
- **Result**: PASS ✓

---

## Key Clarifications

### Thermistor IDs are 0-9, NOT 1-10!
- You have 10 thermistors numbered **0 through 9**
- Thermistor 0 = **first** thermistor
- Thermistor 9 = **last** thermistor

### What Each Function Tests

**`CAN_Msg_SendClaim_Simple()`**
- Returns: CAN ID 0x18EEFF80
- Payload: Fixed 8 bytes (F3 00 80 F3 00 40 1E 90)
- Purpose: Announces Module 0 presence

**`CAN_Msg_SendBMS_Simple()`**
- Returns: CAN ID 0x1839F380
- Payload: Count, Min, Max, Avg, Min ID, Max ID, Lowest ID, Checksum
- Purpose: Summary of all thermistors

**`CAN_Msg_SendGeneral_Simple()`**
- Returns: CAN ID 0x183BF380
- Payload: Thermistor ID (0-9), This temp, Relative ID, Min, Max, Max ID, Min ID, Checksum
- Purpose: Individual thermistor report (cycle through 0-9)

---

## Running the Tests

```bash
# Run all tests
pio test -e native -v

# Expected output:
# [  PASSED  ] 7 tests
```

---

## What's NOT Tested (Yet)

1. ❌ Error handling (disconnected thermistor, short circuit)
2. ❌ Fault byte (0x80) when sensor fails
3. ❌ Steinhart-Hart equation (still using placeholder linear conversion)
4. ❌ Integration with actual can.c code
5. ❌ Timing (200ms claim, 100ms broadcasts)

---

## Test Output Example

```
TEST 3: BMS Broadcast - Min/Max/Avg
  CAN ID: 0x1839F380 (expected 0x1839F380)
  Count: 10 thermistors
  Min:   10°C (ID 9)
  Max:   100°C (ID 8)
  Avg:   55°C
  PASS - Min/Max/Avg correct
```

Clean, simple, to-the-point. ✓
