# Bringup Checklist

Use this linearly for first-time board bring-up and after major hardware/flash changes.

- [ ] Hardware connected (SWD, UART, CAN, BQ links)
- [ ] Board powered from external supply (not probe-only)
- [ ] Debugger connected and stable (STLINK SWD + NRST)
- [ ] Option bytes checked in CubeProgrammer (`nSWBOOT0` / `nBOOT0` path)
- [ ] NRST Level 3 checked/applied for this project
- [ ] Firmware built and flashed successfully
- [ ] Full power cycle performed after flashing/option-byte update
- [ ] UART logs verified at `115200`
- [ ] LEDs checked for expected startup and no persistent fault indication
- [ ] CAN communication verified (`0x18EEFF80`, `0x1839F380`, `0x1838F380`)
- [ ] BQ chain behavior verified (no persistent keep-alive/fault-poll failures)
