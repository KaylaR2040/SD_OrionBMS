# Getting Started

This is the shortest path to get a new board from unknown state to running firmware with logs and CAN traffic.

## 1) Install tools

- PlatformIO CLI/IDE (build + upload)
- STM32CubeProgrammer (option bytes, low-level flash recovery)
- Serial terminal (`pio device monitor` is enough)
- CAN capture tool used by the team (`candump`, Vector, PCAN, etc.)

See: `reference/toolchain_and_programmer_reference.md`

## 2) Gather hardware

- Target board with STM32G474 MCU
- STLINK-V3MINIE probe
- USB cable for STLINK
- External target power supply
- CAN bus connection to your logger/BMS bench network

> STLINK-V3MINIE is a debug/programming probe and VCP bridge; it does not power the target application.

## 3) Verify pinout and wiring

Before first flash, validate wiring against:

- `pinout_and_signals.md`
- `hardware_setup.md`

## 4) Fresh-chip bring-up first

For brand-new chips/boards, complete programmer setup before normal flashing:

- Review boot option bytes (`nSWBOOT0` / `nBOOT0`)
- Apply project reset/debug setting: **NRST Level 3**

See: `startup_and_flashing.md#fresh-chip--brand-new-board-bring-up`

## 5) Build and flash

```bash
pio run -e nucleo_g474re
pio run -e nucleo_g474re -t upload
```

## 6) Verify first boot

```bash
pio device monitor -b 115200
```

Expected checks:

- One-time startup LED pulse
- UART logs on STLINK VCP
- CAN IDs `0x18EEFF80`, `0x1839F380`, `0x1838F380`

## 7) If anything fails

Go directly to:

- `troubleshooting.md`
- `known_issues_and_resolutions.md`
- `bringup_checklist.md`
- `reference/chip_inventory.md`
