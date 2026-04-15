# Flashing Flowchart

```mermaid
flowchart TD
    A[Connect STLINK SWD and external target power] --> B[Open STM32CubeProgrammer]
    B --> C[Connect to target]
    C --> D{Fresh chip or first bring-up?}

    D -->|Yes| E[Review option bytes]
    E --> F[Check boot fields nSWBOOT0 and nBOOT0]
    F --> G[Apply project-known-good boot option-byte profile]
    G --> H[Set reset mode: NRST Level 3 project requirement]
    H --> I[Program option bytes and reset/power cycle]

    D -->|No| J[Keep verified settings]
    I --> K[Flash firmware image]
    J --> K

    K --> L[Verify programming success]
    L --> M[Power cycle and run]
    M --> N{Board stable after cold boot?}

    N -->|Yes| O[Open UART logs and CAN monitor; validate runtime]
    N -->|No| P[Run recovery: re-check boot option bytes + NRST Level 3 and reflash]
    P --> M
```

## Required project checks

- CubeProgrammer option-byte review is mandatory on fresh chips.
- `nSWBOOT0` / `nBOOT0` boot behavior must be verified for this project.
- Use NRST Level 3 during flashing/setup for reliable bring-up in this project.
