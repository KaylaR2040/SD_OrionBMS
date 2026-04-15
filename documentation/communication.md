# Communication

This project currently uses three active communication paths: UART logs, UART battery-monitor transport, and CAN.

## Communication interfaces in use

| Interface | Role | Firmware endpoints | Key settings |
|---|---|---|---|
| USART2 (STLINK VCP) | Console/log output | `UART_Stlink_Init`, `Log_Printf` | 115200, 8N1 |
| USART1 (BQ transport) | BQ79616 protocol path | `UART_BQ79616_Init`, `bq79616_*` | 1000000, 8N1 |
| FDCAN1 | Telemetry output to BMS/logger | `CAN_Comm_SendExt`, `CAN_SendMessages` | 1 Mbps nominal default |

## CAN frame set (current)

| Frame | ID | Typical cadence | Source |
|---|---|---|---|
| J1939 claim | `0x18EEFF80` | 200 ms | `CAN_EncodeJ1939ClaimForModule` |
| BMS therm summary | `0x1839F380` | 100 ms | `CAN_EncodeBmsForCache` |
| General therm round-robin | `0x1838F380` | 100 ms | `CAN_EncodeGeneralForCache` |
| External voltage segments | `0x18FF3000` to `0x18FF3003` | scheduled event | `CAN_EncodeExternalADCVoltage` |

## Data/message path

1. ADC thermistor values are sampled and converted.
2. Fault and thermal summary are computed.
3. BQ service reads cell voltages and updates external-voltage payload source.
4. TIM7 flags trigger message scheduling in `CAN_SendMessages()`.
5. FDCAN TX queue sends extended IDs; drops are rate-limited in logs.

See diagram: `diagrams/communication_flowchart.md`

## Hardware-side communication notes

- UART log path and BQ UART are separate and independently configured.
- If BQ path fails, CAN still runs in degraded fault-reporting mode.
- CAN pin mapping is `PA11` RX / `PA12` TX in MSP init.
- `Docs/iso1050.pdf` is present as CAN transceiver reference material; verify actual transceiver population and polarity against your board schematic/BOM before assuming hardware behavior.

## Common checks when communication fails

- No logs: verify USART2 VCP path and terminal baud (`115200`).
- No BQ response: verify `PC4/PC5` wiring, power, and wake sequence behavior.
- No CAN frames: verify scheduling enabled after startup, CAN bus wiring, transceiver presence, and bench termination.

See also:

- `pinout_and_signals.md`
- `logging_and_leds.md`
- `troubleshooting.md`
- `reference/chip_inventory.md`
