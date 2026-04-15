# Pinout and Signals

This is the consolidated pin and interface map for bring-up/debug, derived from active code first and then cross-checked against existing project notes.

## Source-of-truth priority

1. Active code in `src/` and `include/`
2. Board/vendor docs in `Docs/`
3. Older notes only when they agree with code

## MCU pin map used by current firmware

### UART

| Function | Peripheral | MCU Pin | Source |
|---|---|---|---|
| Log TX | USART2 TX | `PA2` | `src/uart.c`, `src/stm32g4xx_hal_msp.c` |
| Log RX | USART2 RX | `PA3` | `src/uart.c`, `src/stm32g4xx_hal_msp.c` |
| BQ TX | USART1 TX | `PC4` | `src/uart.c`, `src/stm32g4xx_hal_msp.c` |
| BQ RX | USART1 RX | `PC5` | `src/uart.c`, `src/stm32g4xx_hal_msp.c` |

### CAN

| Function | Peripheral | MCU Pin | Source |
|---|---|---|---|
| CAN RX | FDCAN1 RX | `PA11` | `src/stm32g4xx_hal_msp.c` |
| CAN TX | FDCAN1 TX | `PA12` | `src/stm32g4xx_hal_msp.c` |

### ADC thermistor channels

| Therm Channel | ADC Channel | MCU Pin | Source |
|---|---|---|---|
| 1 | ADC1_IN1 | `PA0` | `src/therm.c` |
| 2 | ADC1_IN2 | `PA1` | `src/therm.c` |
| 3 | ADC1_IN11 | `PB12` | `src/therm.c` |
| 4 | ADC1_IN14 | `PB11` | `src/therm.c` |
| 5 | ADC1_IN5 | `PB14` | `src/therm.c` |
| 6 | ADC1_IN6 | `PC0` | `src/therm.c` |
| 7 | ADC1_IN7 | `PC1` | `src/therm.c` |
| 8 | ADC1_IN8 | `PC2` | `src/therm.c` |
| 9 | ADC1_IN9 | `PC3` | `src/therm.c` |
| 10 | ADC1_IN15 | `PB0` | `src/therm.c` |

### LEDs

| LED | MCU Pin | Source |
|---|---|---|
| LED1 | `PA9` | `include/main.h` |
| LED2 | `PA8` | `include/main.h` |
| LED3 | `PC9` | `include/main.h` |

## SWD / programming signals

The firmware docs assume standard SWD wiring is present and stable:

- SWDIO
- SWCLK
- NRST
- GND
- VREF

Project bring-up requirement:

- Use **NRST Level 3** in programmer setup for reliable flashing/debug behavior in this project.

See `startup_and_flashing.md`.

## Known pinout note conflict in older docs

`Docs/PINOUT.md` contains a logging UART mapping that references LPUART1/AF12 on `PA2/PA3`, while active code configures USART2/AF7 for logging. Current firmware source (`src/uart.c`, `src/stm32g4xx_hal_msp.c`) is the authoritative mapping.

## Quick bench validation

1. Confirm UART logs at `115200` over STLINK VCP.
2. Confirm CAN traffic on FDCAN1 (`PA11`/`PA12`).
3. Confirm therm channels produce changing ADC values in logs.
4. Confirm BQ transport path uses `PC4`/`PC5` at `1000000` baud.

See also:

- `hardware_setup.md`
- `communication.md`
- `logging_and_leds.md`
- `reference/chip_inventory.md`
