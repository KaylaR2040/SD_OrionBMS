# System Overview

## What this project does

This firmware runs on STM32G474 and publishes battery/thermal telemetry to CAN. It:

- Samples up to 10 thermistor channels on MCU ADC1
- Talks to a BQ79616 battery monitor chain over USART1
- Publishes Orion-style thermistor and external-voltage CAN frames over FDCAN1
- Emits debug/status logs over STLINK VCP (USART2)

## Main system components

- STM32G474 firmware core (`src/main.c`, `src/init.c`)
- Thermistor acquisition path (`src/therm.c`, `src/thermistor_table.c`)
- BQ79616 interface path (`src/bq79616.c`, `src/volt.c`)
- CAN transport and message packing (`src/can.c`, `src/can_messages.c`)
- Timing/scheduling (`src/timer.c`)

## High-level data flow

1. ADC samples thermistors.
2. Temperatures/fault masks are computed.
3. BQ path keeps comms alive and reads external cell voltages.
4. CAN scheduler transmits claim, BMS summary, general thermistor frames, and external-voltage segments.
5. Logs and LEDs expose runtime state.

See diagram: `diagrams/system_block_diagram.md`

## Startup/connection overview

- `System_AppInit()` initializes clocks, GPIO, UARTs, ADC, CAN, and timers.
- Startup blocks in BQ bring-up (`Volt_RunBlockingStartup()`), then CAN scheduling is released.
- If BQ fails, thermistor/CAN path still runs in degraded fault-reporting mode.

See diagram: `diagrams/startup_flowchart.md`

## Inputs and outputs

Inputs:

- Thermistor analog channels (ADC1)
- BQ UART response data
- Power/reset/debug configuration state

Outputs:

- UART logs on USART2 at 115200
- CAN frames on FDCAN1 at 1 Mbps default
- LED status indication for subsystem failures
