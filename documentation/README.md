# SD Orion BMS Documentation

This folder is the maintainable documentation set for the STM32G474-based Orion thermistor and battery-monitor interface firmware in this repository.

Content is derived from active source (`src/`, `include/`, `platformio.ini`) and checked-in project docs/vendor references in `Docs/`.

## Who this is for

- New engineers bringing up a fresh board or MCU
- Firmware developers navigating architecture and modules
- Debug/operations engineers handling flashing, boot, logging, CAN, and battery-monitor issues

## What this documentation covers

- Fast onboarding and first flash
- Hardware setup and signal/pin mapping
- Firmware architecture and code map
- Communication behavior (USART2 logs, USART1 BQ transport, FDCAN)
- Troubleshooting, known issues, and bring-up recovery
- Chip inventory and board component references

## Start here

1. `getting_started.md`
2. `hardware_setup.md`
3. `pinout_and_signals.md`
4. `startup_and_flashing.md`

Then continue with:

- `system_overview.md`
- `firmware_overview.md`
- `code_map.md`
- `communication.md`

For debugging:

- `logging_and_leds.md`
- `troubleshooting.md`
- `known_issues_and_resolutions.md`
- `bringup_checklist.md`

Reference and component details:

- `reference/hardware_reference.md`
- `reference/firmware_reference.md`
- `reference/toolchain_and_programmer_reference.md`
- `reference/chip_inventory.md`

## Critical bring-up note

For this project, first-time MCU bring-up must include STM32CubeProgrammer option-byte review and reset/debug configuration before trusting normal flashing/debug behavior. See `startup_and_flashing.md`.
