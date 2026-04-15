# Logging and LEDs

## UART logging

- Log channel: USART2 via STLINK VCP (`PA2` TX / `PA3` RX)
- Default monitor speed: `115200` (`platformio.ini`)
- Log macros: `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`, `LOG_DEBUG`

Open monitor:

```bash
pio device monitor -b 115200
```

## Typical startup/runtime logs

Examples from code paths:

- `BQ startup state: READY` or `FAILED`
- `FDCAN nominal timing: ...`
- `Thermistor fault mask=...`
- `BQ Keep-Alive Write FAILED repeatedly...`
- `PACK TOO HOT: ...`

## LED meanings (current code behavior)

| LED logical role | LED ID | Behavior |
|---|---|---|
| Therm subsystem indicator | `THERM_LED` (`LD1`) | Lit when `therm_status == FAILED` |
| CAN subsystem indicator | `CAN_LED` (`LD2`) | Lit when CAN status failed or scheduling disabled |
| Voltage/BQ subsystem indicator | `VOLT_LED` (`LD3`) | Lit when BQ state is not READY |

Additional behavior:

- Startup pulse: all LEDs pulse for 1 second in `main()`.
- Fatal error path (`Error_Handler`): all LEDs toggle indefinitely.

## Normal startup indicators

- One-time LED all-pulse at boot
- UART logs active shortly after init
- CAN periodic frames present once scheduling is enabled

## Abnormal indicators

- No UART output at all
- One subsystem LED stays lit continuously
- Repeating BQ keep-alive/fault poll failures
- CAN IDs missing despite successful build/flash

## No log output quick checks

1. Confirm terminal baud is `115200`.
2. Confirm STLINK VCP device enumerated on host.
3. Confirm target board is externally powered.
4. Confirm firmware reached `UART_Stlink_Init()` (not trapped early in error path).
5. Confirm PA2/PA3 mapping and board routing against schematic.

See also:

- `hardware_setup.md`
- `troubleshooting.md`
