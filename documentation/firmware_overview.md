# Firmware Overview

## Runtime architecture

The firmware is organized as a simple service loop with timer-driven scheduling flags:

- Main loop (`src/main.c`):
  - `Therm_ServiceTask()`
  - `CAN_ServiceTask()`
  - `Volt_ServiceTask()`

- System startup (`src/init.c`):
  - HAL/clocks/GPIO/LED
  - UART init (STLINK + BQ)
  - ADC + CAN + timers
  - blocking BQ bring-up state gate

## Startup sequence

`System_AppInit()` executes this order:

1. `HAL_Init()`
2. `SystemClock_Config()`
3. `LED_Init()`
4. `clocks_configure_all()`
5. `UART_Stlink_Init()` (USART2)
6. `UART_BQ79616_Init()` (USART1)
7. `Therm_App_Init()`
8. `CAN_App_Init(1000 kbps)`
9. `Timers_Init()`
10. `Volt_RunBlockingStartup()`
11. Release CAN scheduling based on BQ state

## Major modules and responsibilities

- `src/therm.c`: ADC1 channel sampling, thermistor snapshot logging
- `src/thermistor_table.c`: ADC-to-temperature conversion
- `src/can.c`: FDCAN init/transport and service entry
- `src/can_messages.c`: message encoding, fault handling, transmission scheduling hooks
- `src/bq79616.c`: wake, keep-alive, fault polling, cell reads
- `src/volt.c`: BQ state machine wrapper and degraded-mode behavior
- `src/timer.c`: TIM6/TIM7 scheduling and staggered log flags
- `src/uart.c`: logging layer and BQ UART transport handles

## Firmware data flow

1. Therm ADC values captured into `g_can_ctx.thermistors`.
2. Temperature/fault cache computed.
3. BQ task updates external voltage buffer into `g_can_ctx.voltages`.
4. CAN task packages and sends periodic frames.
5. UART logging and LEDs expose state transitions/failures.

## Init/loop/driver relationships

- Initialization is centralized in `System_AppInit()`.
- Driver and protocol code stay in dedicated modules.
- ISRs stay short; periodic work is done in service tasks in non-ISR context.

See also:

- `code_map.md`
- `communication.md`
- `diagrams/startup_flowchart.md`
