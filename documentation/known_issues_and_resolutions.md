# Known Issues and Resolutions

This section tracks observed project issues from repository notes, code comments, and bring-up behavior.

## 1) SWBOOT / boot option-byte bring-up issue

- Title: Boot option-byte mismatch causes inconsistent boot behavior
- Where it appeared: Bring-up workflow and programmer setup
- Symptoms:
  - Firmware behavior differs between reflash/reset and cold power cycle
  - Board may appear stuck in bootloader-like behavior
  - Runtime reliability improves only after reprogramming
- Root cause:
  - Project evidence indicates boot option-byte configuration (`nSWBOOT0` / `nBOOT0` BOOT path) must be set correctly during first-time setup
  - Exact board-specific profile must be verified in programmer option-byte view
- Solution:
  1. Connect with CubeProgrammer
  2. Read and set boot option-byte profile to project known-good values
  3. Reprogram option bytes and power-cycle
- Note for future developers:
  - Treat this as a required fresh-chip setup step
  - Verify against schematic and board docs in `Docs/` when uncertain

## 2) NRST Level 3 required for reliable bring-up (project-specific)

- Title: Reset/programming reliability depends on NRST Level 3 in this project
- Where it appeared: Flash/debug setup during bring-up
- Symptoms:
  - Unreliable programming/debug resets until reset mode changed
- Root cause:
  - Exact electrical reason is not proven in-code; observed behavior is repeatable in team bring-up
- Solution:
  - Use NRST Level 3 during flashing/setup for this project
- Note for future developers:
  - Keep this as a project setting, not a universal STM32 recommendation

## 3) CAN timing drift due to timer/loop interaction

- Title: TIM7 and blocking reads caused CAN transmit interval errors
- Where it appeared: `Docs/CAN_TIMING_ISSUE_AND_FIX.md`
- Symptoms:
  - Messages expected at 100/200 ms appeared around ~2 s cadence
- Root cause:
  - TIM7 setup/IRQ wiring mismatch plus long blocking BQ fault reads in loop
- Solution:
  - Correct TIM7 period/IRQ path and reduce blocking read impact
- Note for future developers:
  - Keep ISR lightweight and avoid long blocking operations in service loop paths that gate CAN send timing

## 4) Documentation drift in legacy notes

- Title: Some older markdown does not match current code
- Where it appeared: root `README.md`, portions of `Docs/*.md`
- Symptoms:
  - references to old architecture/IDs/peripherals
- Root cause:
  - docs evolved separately from current implementation
- Solution:
  - use `src/` + `include/` as source of truth and maintain this `documentation/` set
- Note for future developers:
  - update docs together with firmware changes that alter startup, comms, IDs, or pins

See also:

- `startup_and_flashing.md`
- `troubleshooting.md`
