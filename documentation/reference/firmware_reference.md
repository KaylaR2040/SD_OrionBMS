# Firmware Reference

Compact firmware facts and file locations.

## Build/profile

- Build system: PlatformIO (`platformio.ini`)
- Target env: `nucleo_g474re`
- Upload protocol: `stlink`

## Core runtime flow

- Entry: `src/main.c`
- Startup orchestration: `src/init.c` (`System_AppInit`)
- Main task loop order:
  1. `Therm_ServiceTask`
  2. `CAN_ServiceTask`
  3. `Volt_ServiceTask`

## Key modules

- Thermistor acquisition: `src/therm.c`, `include/therm.h`
- Therm conversion table: `src/thermistor_table.c`, `include/thermistor_table.h`
- CAN transport: `src/can.c`, `include/can.h`
- CAN message packing/scheduling: `src/can_messages.c`, `include/can_messages.h`
- BQ interface: `src/bq79616.c`, `include/bq79616.h`
- BQ service wrapper/state: `src/volt.c`, `include/volt.h`
- Timers and flags: `src/timer.c`, `include/timer.h`
- Logging/UART: `src/uart.c`, `include/uart.h`
- LED control: `src/led.c`, `include/led.h`
- Error path: `src/error_handling.c`, `include/error_handling.h`

## Important runtime constants/IDs

- BQ UART baud: `1000000` (`BQ_BAUDRATE`)
- Log UART baud: `115200`
- Default CAN bitrate: `1000 kbps` (`CAN_APP_DEFAULT_KBPS`)
- Main CAN IDs:
  - `0x18EEFF80` claim
  - `0x1839F380` BMS summary
  - `0x1838F380` general thermistor
  - `0x18FF3000..003` external voltage segments

## Known architectural behavior

- BQ startup is blocking during init.
- If BQ fails, thermistor + CAN path remains active with fault-reporting behavior.
- CAN scheduling uses TIM7 flags and non-ISR send operations.
