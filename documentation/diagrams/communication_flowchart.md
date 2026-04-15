# Communication Flowchart

```mermaid
flowchart LR
    A[Thermistor ADC samples] --> B[Temperature conversion + fault mask]
    B --> C[CAN cache update]

    D[BQ keep-alive + fault polls + cell reads] --> E[External voltage cache update]
    E --> C

    F[TIM7 schedule flags] --> G[CAN_SendMessages]
    C --> G

    G --> H1[0x18EEFF80 claim]
    G --> H2[0x1839F380 BMS summary]
    G --> H3[0x1838F380 general thermistor]
    G --> H4[0x18FF3000..003 external volt segments]

    H1 --> I[FDCAN1 TX]
    H2 --> I
    H3 --> I
    H4 --> I

    I --> J[CAN bus logger / BMS]

    K[Runtime events/faults] --> L[LOG_INFO/WARN/ERROR/DEBUG]
    L --> M[USART2 STLINK VCP 115200]
```

## Notes

- CAN transmission scheduling is interrupt-flag driven but sent in task context.
- BQ comm loss transitions voltage path to failed state while thermistor CAN path continues.
