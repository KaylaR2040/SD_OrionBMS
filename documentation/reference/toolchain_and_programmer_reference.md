# Toolchain and Programmer Reference

## Build and flash tools

| Tool | Use |
|---|---|
| PlatformIO (`pio`) | Build and upload firmware |
| STM32CubeProgrammer | Option-byte configuration, direct flash, recovery |
| STLINK-V3MINIE | Physical debug/program probe and VCP bridge |

## Typical commands

Build:

```bash
pio run -e nucleo_g474re
```

Upload:

```bash
pio run -e nucleo_g474re -t upload
```

Open serial monitor:

```bash
pio device monitor -b 115200
```

## Programmer setup notes (project)

- For fresh chips/new boards, do CubeProgrammer setup first.
- Review and configure boot option bytes (`nSWBOOT0` / `nBOOT0` path) per project bring-up profile.
- Set reset mode to NRST Level 3 for this project during setup/flashing.

## STLINK-V3MINIE notes from checked-in docs

- Provides SWD debug/program functionality.
- Provides VCP for UART logs.
- Does not provide target application power (use external power source).

## CAN and bench validation tools

- Captured logs in repo:
  - `Other/can_log.txt`
  - `Other/mycan_log.txt`
  - `Other/can_diag_campaign_20260412_234404.log`
- Data scripts:
  - `Other/convertingtoexcel.py`
  - `Other/comparing.py`

## Recovery-first sequence

1. Connect SWD + external power.
2. Use CubeProgrammer to verify connection and option bytes.
3. Apply project boot/reset settings.
4. Flash firmware.
5. Power-cycle and verify UART + CAN behavior.

## Related references

- `../pinout_and_signals.md`
- `hardware_reference.md`
- `chip_inventory.md`
