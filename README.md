# EV Active Sensor Adapter

A compact, HV-aware sensing module for an FSAE EV accumulator. It acquires per-cell voltages and thermistor temperatures, validates each sample batch, and publishes telemetry on Classical CAN for the logger/BMS. The firmware uses an RTOS framework with timer-triggered ADC + DMA, deterministic validation/packetization, and an isolated CAN transceiver. The repository is structured so any developer can rebuild, verify, and extend the system quickly.

---

## Table of Contents
- [Project Purpose](#project-purpose)
- [Core Functionality](#core-functionality)
- [System Architecture](#system-architecture)
- [RTOS Design (Actors & IPC)](#rtos-design-actors--ipc)
- [Repository Structure](#repository-structure)
- [Build & Run](#build--run)
- [Configuration](#configuration)
- [Data & CAN](#data--can)
- [Documentation (Doxygen)](#documentation-doxygen)
- [File/Library Origins & Licenses](#filelibrary-origins--licenses)
- [Issue Tracking & Versioning](#issue-tracking--versioning)
- [Keywords / SEO](#keywords--seo)
- [Target Audience](#target-audience)
- [Roadmap](#roadmap)
- [Acknowledgments](#acknowledgments)

---

## Project Purpose
Design and validate a pass-through “active sensor adapter” that reads per-cell voltages and thermistors and relays them to the team’s logger/BMS over CAN. The goals are to minimize wiring and board count, meet the accumulator envelope and safety requirements, and keep the system straightforward to integrate and debug during scrutineering.

## Core Functionality
- **Acquisition:** Differential cell-tap sensing per segment; scaling to MCU ADC range (0–3.3 V) via analog front-end; thermistor sense and linearization.
- **Validation:** Range checks, monotonicity/slew checks, optional moving average/median filtering; configurable limits.
- **Publication:** Periodic CAN frames with per-cell voltages, temperatures, and health flags; heartbeat and diagnostics frames.
- **Timing:** ADC triggered by hardware timer; DMA → processing → CAN TX within fixed RTOS time budgets.
- **Safety & EMC:** Isolated CAN PHY; partitioned HV/LV domains; input protection and layout rules for creepage/clearance.

---

## System Architecture
**High level**
```
[Cell Taps & Thermistors] → [Analog Front-End] → [MCU ADC + DMA] 
    → [RTOS Tasks: Validate, Packetize] → [CAN Controller] → [Isolated CAN Transceiver] → [Logger / BMS]
```

**Major blocks**
- **Analog Front-End:** Differential measurement per cell segment; scale/offset to ADC domain; RC anti-alias/filtering; ESD/OVP where applicable.
- **MCU + Firmware:** ADC + DMA; timer-driven sampling; RTOS tasks for validation/packing; CAN driver; non-volatile config.
- **CAN Subsystem:** On-chip CAN controller (Classical CAN; 11/29-bit); isolated transceiver; configurable bitrate and filters.

---

## RTOS Design (Actors & IPC)
| Actor (Type)              | Purpose                                   | Period / Trigger          | IPC / Resources            |
|---------------------------|-------------------------------------------|---------------------------|----------------------------|
| `adc_sampler` (ISR/DMA CB)| Kick conversions; DMA completion handling | Timer @ e.g., 100–200 Hz  | Queue → `validate_task`    |
| `validate_task` (task)    | Range/slew checks; filtering              | On queue receive          | Queue → `can_tx_task`      |
| `can_tx_task` (task)      | Pack frames; rate-limit; send via CAN     | On queue / 10–50 Hz slots | CAN driver; TX mailbox     |
| `diag_task` (task)        | Heartbeat; error counters; self-tests     | 1 Hz                      | CAN driver                 |
| `config_task` (task)      | Apply params from NVM/debug console       | On command                | NVM / CLI / UART (optional)|

IPC: queues for sample batches, mutex for config access, event flags for errors/heartbeat, semaphore for CAN TX credit.  
RTOS: FreeRTOS or CMSIS-RTOS2 shim. The framework is present even if minimal features are used.

---

## Repository Structure
```
.
├─ firmware/
│  ├─ app/
│  │  ├─ main.c
│  │  ├─ tasks_can.c
│  │  ├─ tasks_validate.c
│  │  ├─ tasks_diag.c
│  │  └─ app_config.h
│  ├─ drivers/
│  │  ├─ adc_drv.c/.h
│  │  ├─ can_drv.c/.h
│  │  ├─ timer_drv.c/.h
│  │  └─ board_pins.h
│  ├─ rtos/
│  │  ├─ rtos_port.c/.h         # CMSIS-RTOS2 or FreeRTOS glue
│  │  └─ freertos/              # (if vendored; else via submodule)
│  ├─ utils/
│  │  ├─ filters.c/.h
│  │  ├─ crc.c/.h
│  │  └─ log.c/.h
│  └─ CMakeLists.txt / *.project (if CCS)
├─ hardware/
│  ├─ schematics/
│  └─ pcb/
├─ dbc/
│  └─ ev_active_sensor.dbc
├─ tools/
│  ├─ can_dump_example.py
│  └─ dbc_gen.py
├─ docs/
│  ├─ doxygen/
│  │  └─ Doxyfile
│  └─ images/
├─ LICENSE
└─ README.md
```

---

## Build & Run

You can recreate the system using one of the paths below. All code elements should build directly in GitHub CI and locally.

### Option A: TI Code Composer Studio (CCS)
1. Install CCS and the vendor SDK/driver pack for TMS320F23897xD.
2. `File → Import… → Existing CCS Project`, select `firmware/`.
3. Select the correct target and connection.
4. Build and flash. Use the “Release” profile for timing-accurate tests.

### Option B: CMake + ARM GCC (or vendor GCC)
```bash
cd firmware
cmake -B build -DCMAKE_TOOLCHAIN_FILE=toolchains/arm-none-eabi.cmake -DMCU=<TMS320F23897x>
cmake --build build -j
# Flash with preferred programmer, e.g.:
# openocd -f interface/<probe>.cfg -f target/TMS320F23897x.cfg -c "program build/firmware.elf verify reset exit"
```

### Bring-up checklist
- Confirm ADC reference and pin mapping in `board_pins.h` and `app_config.h`.
- Set CAN bitrate/filters in `app_config.h` (500 kbps, 1megps).
- Connect isolated CAN transceiver to the vehicle bus; verify TX/RX loopback on bench first.
- Start the logger; verify heartbeat and telemetry frames appear at the expected rates.

---

## Configuration
Edit `app_config.h` (or use CLI/UART if enabled):
- `SAMPLE_RATE_HZ`, `CAN_BITRATE`, `FRAME_PERIOD_MS`
- Channel enable masks; number of cells; thermistor Beta constants
- DBC frame IDs and scaling

---

## Data & CAN
- **CAN Type:** Classical CAN (11-bit or 29-bit).
- **Frames:**
  - `CELL_VOLTAGES_n`: packed per-cell voltages (scaled in mV)
  - `CELL_TEMPS_n`: packed temperatures (0.1 °C or °C)

---

## Documentation (Doxygen)
- In-code comments follow Doxygen style; functions link back to project requirements via `@req EV-REQ-###`.
- Generate docs locally:
  ```bash
  doxygen docs/doxygen/Doxyfile
  ```
- Publish: upload `docs/html/` via GitHub Pages or attach as a CI artifact.

---

## File/Library Origins & Licenses
- **Our code:** `firmware/app`, `firmware/utils`, and project glue — Apache-2.0 (see `LICENSE`).
- **RTOS:** FreeRTOS (MIT) via submodule or vendor package; see submodule `LICENSE`.
- **Drivers:** Either written in-house (Apache-2.0) or vendor HAL (vendor license). Each file header states origin and license.
- **DBC & tools:** Team-authored, Apache-2.0 unless otherwise noted.

> Every third-party directory includes a `NOTICE` with origin URL, version, and license.

---

## Issue Tracking & Versioning
- Use GitHub Issues for bugs/features. Each issue should include: owner, date, code version (git SHA), description, and resolution.
- Commits must include *what changed and why* in the message body.
- Releases are tagged (`vMAJOR.MINOR.PATCH`), with one marked **Final** for hand-off at semester end.

---

## Keywords / SEO
FSAE EV, battery monitoring, cell voltage sensing, thermistor sensing, differential measurement, ADC DMA, Classical CAN, isolated CAN transceiver, RTOS, FreeRTOS, CMSIS-RTOS2, embedded firmware, automotive EMC, accumulator.

## Target Audience
FSAE teams, embedded/firmware students, and engineers building HV-aware sensing for battery telemetry.

---

## Roadmap
- [ ] Bench characterization: ADC INL/DNL, noise, and filter step response
- [ ] Fault injection: wire-off, short, out-of-range, thermistor open/short
- [ ] CAN filter/ID finalization with logger/BMS DBC
- [ ] Production test mode & golden-frame generator
- [ ] CI: static analysis and unit tests for filters and validators

---

## Acknowledgments
Thanks to faculty mentors, the electrical subteam, and sponsors supporting test equipment and PCB fab.
