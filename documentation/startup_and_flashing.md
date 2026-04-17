# Startup and Flashing

This is the primary bring-up and recovery guide for this repository.

## Preconditions

- Target board externally powered
- STLINK-V3MINIE connected over SWD (including NRST)
- Firmware built (`pio run -e nucleo_g474re`)

## Standard flash flow

1. Connect STLINK and target power.
2. Start STM32CubeProgrammer and confirm SWD connection.
3. Verify project bring-up settings (option bytes + NRST level; details below).
4. Flash firmware (CubeProgrammer or PlatformIO).
5. Reset/power-cycle and validate UART + CAN behavior.

## Fresh Chip / Brand-New Board Bring-Up

This step is mandatory for first-time chips/boards in this project.

1. Open STM32CubeProgrammer and connect with SWD.
2. Read and review option bytes related to boot source:
   - `nSWBOOT0`
   - `nBOOT0`
   - any BOOT0-related project boot fields shown by tool version
3. Apply the project's known-good boot profile so the MCU consistently boots user firmware from Flash on reset/power-cycle.
4. Set reset mode to **NRST Level 3** for this project during setup/flashing.
5. Program option bytes, then perform a full reset/power cycle.

> Important (project-specific): This repository has observed bring-up failures when boot option-byte configuration is wrong and when NRST mode is not set to the project's known-good value.

> Important (scope): NRST Level 3 is documented here as a project requirement, not a universal STM32 rule.

## Boot option-byte configuration issue (SWBOOT / nSWBOOT0 / nBOOT0)

Observed project behavior:

- Board may not boot application reliably after power cycle
- Board may look stuck in bootloader-like behavior
- Programming appears to work, but runtime behavior is inconsistent
- Board may appear to recover only after reprogramming

What to do:

- Use CubeProgrammer option-byte view and confirm boot-source configuration matches the team's known-good settings.
- Re-apply and cycle power after option-byte changes.
- Verify by cold boot, not only debugger reset.

Evidence notes in repo:

- STM32 Nucleo G4 board docs in `Docs/dm00556337-*.pdf` mention `nSWBOOT0`/`nBOOT0` BOOT0 behavior.
- Project code itself does not hard-code option-byte values; bring-up setup must be done with programmer tools.

## Project-specific NRST Level 3 requirement

For this project, use NRST Level 3 during flashing/setup because this was required to achieve reliable bring-up behavior.

- Use this in CubeProgrammer/programmer reset settings during fresh-chip setup and recovery attempts.
- If the exact electrical cause is needed, verify against schematic and lab measurements.

## Flashing with PlatformIO

```bash
pio run -e nucleo_g474re
pio run -e nucleo_g474re -t upload
```

## Flashing with CubeProgrammer

1. Connect via STLINK/SWD.
2. Load generated `.elf` or `.bin`.
3. Program and verify.
4. Reset/power cycle.

## Verify flashing success

- UART monitor shows normal startup logs (`115200` on STLINK VCP)
- Boot LED pulse occurs once
- CAN traffic includes IDs:
  - `0x18EEFF80`
  - `0x1839F380`
  - `0x1838F380`

## If the board only works after reprogramming

Likely causes in this project:

- Boot option-byte configuration mismatch (`nSWBOOT0`/`nBOOT0` path)
- Reset/debug configuration mismatch (NRST not set to project-known-good Level 3)
- Power-cycle path differs from debugger reset path

Recovery sequence:

1. Reconnect in CubeProgrammer.
2. Re-check and re-apply boot option-byte profile.
3. Set NRST to Level 3 (project requirement).
4. Reflash firmware.
5. Do a full power cycle and retest without debugger-assisted reset.

## If flashing fails

- Check SWD wiring and target power first.
- Lower SWD speed and retry.
- Use mass erase if device is in a bad state.
- Re-apply fresh-chip bring-up sequence above.

See also:

- `hardware_setup.md`
- `troubleshooting.md`
- `known_issues_and_resolutions.md`
- `bringup_checklist.md`
