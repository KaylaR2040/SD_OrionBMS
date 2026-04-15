# Hardware Setup

This section focuses on practical bench bring-up for the current firmware and pin mappings in code.

## Required boards and tools

| Item | Purpose | Source in repo |
|---|---|---|
| STM32G474 target board | Runs firmware | `platformio.ini`, `src/*` |
| STLINK-V3MINIE | SWD programming/debug + VCP logs | `Docs/um2910-stlinkv3minie-*.pdf` |
| External target power | Powers the target board/application | STLINK note in `Docs/um2910-*.pdf` |
| CAN interface + bus wiring | Observe CAN traffic | `src/can.c`, `src/can_messages.c` |
| BQ chain hardware | External voltage monitor path | `src/bq79616.c` |

For full component context (including CAN transceiver and legacy/reference chips), see `reference/chip_inventory.md`.

## Wiring and interface overview

- SWD/programmer: SWDIO, SWCLK, NRST, GND, target reference voltage
- Logging UART: USART2 `PA2/PA3` via STLINK VCP, 115200 baud
- BQ transport UART: USART1 `PC4/PC5` at 1,000,000 baud
- CAN: FDCAN1 `PA11` (RX) and `PA12` (TX)
- Thermistors: ADC1 channels mapped in `src/therm.c`

Detailed signal tables are in `pinout_and_signals.md`.

## Power and connection order

1. Connect STLINK-V3MINIE SWD to target.
2. Connect USB to STLINK (debug + VCP).
3. Apply external target power.
4. Open CubeProgrammer or PlatformIO tools.

> STLINK-V3MINIE does not power the target application. Always power the board separately.

## ST-Link / debugger usage notes

- Probe: STLINK-V3MINIE
- Interface: SWD
- Keep NRST connected for reliable reset/programming sequences
- For first-time bring-up, apply project setup in `startup_and_flashing.md`

## Before you power on (checklist)

- [ ] SWD wiring correct (including NRST and GND)
- [ ] External target power connected and stable
- [ ] CAN wiring and termination match your bench setup
- [ ] BQ UART lines (`PC4` TX, `PC5` RX) connected to battery-monitor chain
- [ ] No accidental shorts on ADC thermistor inputs

## First power-on expected behavior

- LEDs initialize and all pulse once (`LED_All_Pulse(1000u)`)
- UART log channel becomes active (USART2)
- Firmware performs blocking BQ startup gate
- CAN scheduling starts after BQ startup reaches READY or FAILED terminal state

See also:

- `pinout_and_signals.md`
- `startup_and_flashing.md`
- `logging_and_leds.md`
- `bringup_checklist.md`
