# Chip Inventory and Board Components

This file captures the chips and major hardware components referenced in this repository and their status in the current firmware.

## A) Active in current firmware path

| Component | Role in project | Evidence in repo |
|---|---|---|
| STM32G474 (STM32G474xx family) | Main MCU running firmware | `platformio.ini`, `src/*.c`, `include/*.h` |
| BQ79616 | Battery monitor UART chain interface | `src/bq79616.c`, `include/bq79616.h`, `REGISTER_REFERENCE.md`, `QUICK_START_GUIDE.md` |
| STLINK-V3MINIE | SWD programming/debug + UART VCP bridge | `Docs/um2910-*.pdf`, `Docs/stlink-v3minie.pdf` |

## B) Present in checked-in hardware docs (verify board population)

| Component / document | Typical purpose | Status note |
|---|---|---|
| ISO1050 (`Docs/iso1050.pdf`) | CAN transceiver/isolation reference | Not directly referenced in active source; verify against actual board BOM/schematic |
| MAX17841B / MAX17854 docs (`Docs/MAX17841B.pdf`, `Docs/MAX17854.pdf`, EV kit docs) | Battery-monitor family references | Not active in current STM32 source path; treat as reference material |
| Nucleo G474 board docs/schematic (`Docs/dm00556337-*.pdf`, `Docs/mb1367-g474re-c05_schematic.pdf`) | Board-level pin/boot/debug behavior reference | Useful for bring-up, especially BOOT0/option-byte context |

## C) Legacy/reference content in repo

| Component family | Where found | Status note |
|---|---|---|
| TMS570 sample environment | `TI_Ref_Code/bq79616_sample_code/*` | Legacy reference/sample code; not current target firmware |
| MSP430 docs (`Docs/msp430fr2355.pdf`, `Docs/slau680.pdf`) | Reference docs | Not part of active STM32 firmware path |

## Project setup implications

- Build/flash/debug workflow is STM32G474 + STLINK-V3MINIE based.
- BQ79616 bring-up behavior (wake, keep-alive, fault handling) is implemented and required for full voltage path operation.
- CAN transceiver hardware details should be verified against your board-level schematic/BOM before making electrical assumptions.

## Required bring-up checks tied to these components

1. STM32 option-byte and boot setup in CubeProgrammer (`nSWBOOT0` / `nBOOT0` path).
2. Project reset requirement: NRST Level 3.
3. External target power present (STLINK does not power the target application).
4. BQ UART wiring and wake path validated.

See also:

- `../startup_and_flashing.md`
- `../pinout_and_signals.md`
- `hardware_reference.md`
