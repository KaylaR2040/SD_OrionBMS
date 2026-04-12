# Pin Assignments - STM32G474RET6
> Last Updated 4/12/26

Single consolidated map, grouped by peripheral/function. AF values match the HAL MSP setup in `src/stm32g4xx_hal_msp.c`.

| Group   | Signal / Use                 | Pin    | Pin # | AF / Mode          | External Connection / Notes |
|---------|------------------------------|--------|-------|--------------------|-----------------------------|
| UART    | USART1 TX (BQ79616)          | PC4    | 24    | AF7 USART1         | Connect to BQ79616-Q1 RX (Pin 52) |
| UART    | USART1 RX (BQ79616)          | PC5    | 25    | AF7 USART1         | Connect to BQ79616-Q1 TX (Pin 53) |
| UART    | LPUART1 TX (logging)         | PA2    | 14    | AF12 LPUART1       | Connect to ST-LINK `T_VCP_TX` (USB logging) |
| UART    | LPUART1 RX (logging)         | PA3    | 17    | AF12 LPUART1       | Connect to ST-LINK `T_VCP_RX` (USB logging) |
| CAN     | FDCAN1 RX                    | PA11   | 46    | AF9 FDCAN1         | 1 Mbps default |
| CAN     | FDCAN1 TX                    | PA12   | 47    | AF9 FDCAN1         | 1 Mbps default |
| SPI     | SPI1 SCK                     | PA5    | 19    | AF5 SPI1           | MAX17841B / ASCI |
| SPI     | SPI1 MISO                    | PA6    | 20    | AF5 SPI1           |  |
| SPI     | SPI1 MOSI                    | PA7    | 21    | AF5 SPI1           |  |
| SPI     | SPI1 CS (software)           | PA4    | 18    | GPIO output        | MAX17841B chip select (manual) |
| LED     | LED1                         | PA9    | 43    | GPIO output        | Active high |
| LED     | LED2                         | PA8    | 42    | GPIO output        | Active high |
| LED     | LED3                         | PC9    | 40    | GPIO output        | Active high |
| GPIO    | Spare                        | PC6    | 37    | GPIO               | Unused |
| GPIO    | Spare / analog header        | PB15   | 36    | GPIO/Analog        | Unused |
| GPIO    | Spare / analog header        | PB13   | 34    | GPIO/Analog        | Unused |
| ADC1    | IN1 (Thermistor 1)           | PA0    | 12    | Analog             | ADC_CHANNEL_1 |
| ADC1    | IN2 (Thermistor 2)           | PA1    | 13    | Analog             | ADC_CHANNEL_2 |
| ADC1    | IN11 (Thermistor 3)          | PB12   | 35    | Analog             | ADC_CHANNEL_11 |
| ADC1    | IN14 (Thermistor 4)          | PB11   | 33    | Analog             | ADC_CHANNEL_14 |
| ADC1    | IN5 (Thermistor 5)           | PB14   | 36    | Analog             | ADC_CHANNEL_5 |
| ADC1    | IN6 (Thermistor 6)           | PC0    | 8     | Analog             | ADC_CHANNEL_6 |
| ADC1    | IN7 (Thermistor 7)           | PC1    | 9     | Analog             | ADC_CHANNEL_7 |
| ADC1    | IN8 (Thermistor 8)           | PC2    | 10    | Analog             | ADC_CHANNEL_8 |
| ADC1    | IN9 (Thermistor 9)           | PC3    | 11    | Analog             | ADC_CHANNEL_9 |
| ADC1    | IN15 (Thermistor 10)         | PB0    | 23    | Analog             | ADC_CHANNEL_15 |

## Notes
- BQ79616 runs on USART1 (PC4/PC5). Logging defaults to LPUART1 (PA2/PA3) to keep pins separated.
- PA4 is the software CS for MAX17841B (SPI1).
- All LEDs are active high; LED5 is the fault indicator.
- No DMA is used; UARTs/SPI are blocking for bring-up.
- BQ79616 datasheet reference: https://www.ti.com/lit/ds/symlink/bq79612-q1.pdf