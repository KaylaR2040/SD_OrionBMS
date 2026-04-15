# Code Map

This file is a navigation map to the current repository layout and the modules you usually touch first.

## Top-level layout (current)

- `src/` - firmware implementation
- `include/` - headers and shared APIs
- `Docs/` - legacy docs, vendor PDFs, hardware references
- `Other/` - logs and analysis scripts
- `platformio.ini` - build target, upload protocol, monitor speed

## Important source files

| Topic | Where to look |
|---|---|
| Main / entry point | `src/main.c` |
| System init / startup order | `src/init.c` |
| Clock setup | `src/init.c`, `src/clocks.c` |
| Hardware MSP pin setup | `src/stm32g4xx_hal_msp.c` |
| UART (logs + BQ transport) | `src/uart.c`, `include/uart.h` |
| CAN transport | `src/can.c`, `include/can.h` |
| CAN message encoding | `src/can_messages.c`, `include/can_messages.h` |
| ADC thermistor sampling | `src/therm.c`, `include/therm.h` |
| Thermistor conversion table | `src/thermistor_table.c` |
| Battery monitor interface | `src/bq79616.c`, `src/volt.c`, `include/bq79616.h`, `include/volt.h` |
| Logging macros | `include/uart.h` (`LOG_INFO/WARN/ERROR/DEBUG`) |
| LED control | `src/led.c`, `include/led.h` |
| Error handling | `src/error_handling.c`, `include/error_handling.h` |
| Timer scheduling/flags | `src/timer.c`, `include/timer.h` |

## Where to look for bring-up/debug topics

- Pin and signal mapping: `pinout_and_signals.md`
- Chip/component context: `reference/chip_inventory.md`
- CAN timing issue history: `Docs/CAN_TIMING_ISSUE_AND_FIX.md`
- BQ wake/startup behavior: `src/bq79616.c`, `include/bq79616.h`, `QUICK_START_GUIDE.md`, `REGISTER_REFERENCE.md`
- Captured CAN logs: `Other/can_log.txt`, `Other/can_diag_campaign_20260412_234404.log`

## Notes on older docs

Some root and `Docs/` markdown files describe older architectures. Treat active code under `src/` and `include/` as source of truth first.
