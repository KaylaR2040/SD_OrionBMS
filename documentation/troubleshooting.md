# Troubleshooting

Use this by symptom. Each section gives likely cause, verification method, and fix.

## Cannot flash firmware

- Symptom: Programmer fails to erase/program or verify.
- Likely cause: SWD wiring/power issue, target held in bad boot/reset state.
- How to verify: CubeProgrammer cannot read device info reliably.
- Fix: Check power and SWD pins, use mass erase, then run fresh-chip bring-up steps.

## Debugger cannot connect

- Symptom: STLINK sees no target or unstable connection.
- Likely cause: Missing VREF/GND/NRST, wrong reset mode, unstable power.
- How to verify: intermittent connect/disconnect or ID read failures.
- Fix: Rewire SWD fully, set project reset mode to NRST Level 3, power-cycle target.

## No logs on serial console

- Symptom: `pio device monitor` shows no startup output.
- Likely cause: wrong COM port/baud, no target power, firmware not reaching UART init.
- How to verify: check `115200`, STLINK VCP enumeration, and power rails.
- Fix: correct monitor settings, power target, then reflash and retry.

## Code only works after reprogramming

- Symptom: runs after flash/debug reset, fails after cold power cycle.
- Likely cause: boot option-byte mismatch (`nSWBOOT0`/`nBOOT0`) or reset setup mismatch.
- How to verify: compare behavior between debug reset and cold boot.
- Fix: re-apply CubeProgrammer boot option-byte profile and set NRST Level 3.

## Power issue

- Symptom: random resets, unstable comms, no consistent boot.
- Likely cause: STLINK assumed to power target, or bench supply instability.
- How to verify: measure target rails during flash and runtime.
- Fix: provide stable external power; do not rely on STLINK for target application power.

## Communication issue (CAN)

- Symptom: no expected CAN IDs on bus.
- Likely cause: bus wiring/termination, scheduling disabled, or FDCAN init/runtime fault.
- How to verify: check for IDs `0x18EEFF80`, `0x1839F380`, `0x1838F380`; inspect logs.
- Fix: validate wiring/termination, verify startup reached CAN enabled state.

## Battery monitor chain not responding

- Symptom: BQ keep-alive/fault poll errors and voltage path disabled.
- Likely cause: UART wiring (`PC4/PC5`), wake sequence failure, BQ power/path issue.
- How to verify: check BQ logs and fault register read outcomes.
- Fix: verify BQ UART wiring/power, rerun startup, inspect `src/bq79616.c` wake path.

## Boot/reset misconfiguration

- Symptom: inconsistent startup, bootloader-like behavior, or poor debug reset behavior.
- Likely cause: option-byte boot config not matching project needs; reset mode mismatch.
- How to verify: read option bytes in CubeProgrammer and compare against known-good profile.
- Fix: set boot option bytes for user-flash boot path and set NRST Level 3 for this project.

See also:

- `startup_and_flashing.md`
- `known_issues_and_resolutions.md`
- `bringup_checklist.md`
