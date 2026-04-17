# Documentation Relationship Map

This diagram shows how the documentation is intended to be consumed.

```mermaid
flowchart TD
    A[README.md] --> B[getting_started.md]
    B --> C[hardware_setup.md]
    C --> C2[pinout_and_signals.md]
    C2 --> D[startup_and_flashing.md]

    D --> E[system_overview.md]
    E --> F[firmware_overview.md]
    F --> G[code_map.md]
    G --> H[communication.md]

    D --> I[logging_and_leds.md]
    I --> J[troubleshooting.md]
    J --> K[known_issues_and_resolutions.md]
    K --> L[bringup_checklist.md]

    A --> M[docs_map.md]
    M --> N[reference/hardware_reference.md]
    M --> O[reference/firmware_reference.md]
    M --> P[reference/toolchain_and_programmer_reference.md]
    M --> R[reference/chip_inventory.md]

    E --> Q[diagrams/system_block_diagram.md]
    D --> S[diagrams/startup_flowchart.md]
    D --> T[diagrams/flashing_flowchart.md]
    H --> U[diagrams/communication_flowchart.md]
```

## Reading intent

- Start with onboarding (`getting_started.md`).
- Validate hardware/signal mapping before deep debug.
- Complete flashing/bring-up setup before architecture deep-dive.
- Use troubleshooting and known issues as operational references.
- Use `reference/` files for quick chip/tool/fact lookup.
