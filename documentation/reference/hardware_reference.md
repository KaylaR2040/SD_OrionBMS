# Hardware Reference

Compact hardware facts extracted from active source and checked-in documentation.

## MCU and firmware target context

- Target MCU family: STM32G474 (`platformio.ini`, `STM32G474xx`)
- PlatformIO board env: `nucleo_g474re`
- Firmware uses HAL and selected LL modules

## Active pin/interface mappings from code

- UART logging: USART2 `PA2` TX / `PA3` RX (`src/uart.c`, `src/stm32g4xx_hal_msp.c`)
- BQ UART transport: USART1 `PC4` TX / `PC5` RX (`src/uart.c`, `src/stm32g4xx_hal_msp.c`)
- CAN: FDCAN1 `PA11` RX / `PA12` TX (`src/stm32g4xx_hal_msp.c`)
- Thermistor ADC channels (10): `PA0`, `PA1`, `PB12`, `PB11`, `PB14`, `PC0`, `PC1`, `PC2`, `PC3`, `PB0` (`src/therm.c`)
- LEDs: `PA9`, `PA8`, `PC9` (`include/main.h`)

For full signal tables, use `../pinout_and_signals.md`.

## Power and probe notes

- STLINK-V3MINIE supports debug/program and VCP.
- Checked-in ST docs (`Docs/um2910-*.pdf`) state STLINK-V3MINIE does not provide target application power.
- Use external target power for bring-up and runtime testing.

## Boot/reset bring-up notes (project)

- First-time bring-up requires option-byte review in CubeProgrammer.
- Project requires NRST Level 3 during setup/flashing for reliable behavior.
- Treat NRST Level 3 as project-specific unless validated otherwise on your hardware revision.

## Chip/component scope notes

- Active firmware path clearly uses STM32G474 + BQ79616.
- Additional component docs (for example ISO1050, MAX17841/MAX17854) are present in `Docs/` and should be verified against actual board population/BOM before use.

For complete component status, use `chip_inventory.md`.
