# Pin Assignments — STM32G474RET6 (Current)

Single consolidated map, grouped by peripheral/function. AF values match the HAL MSP setup in `src/stm32g4xx_hal_msp.c`.

| Group   | Signal / Use                 | Pin    | AF / Mode          | External Connection / Notes |
|---------|------------------------------|--------|--------------------|-----------------------------|
| UART    | USART1 TX (BQ79616)          | PC4    | AF7 USART1         | Connect to BQ79616-Q1 RX (Pin 52) |
| UART    | USART1 RX (BQ79616)          | PC5    | AF7 USART1         | Connect to BQ79616-Q1 TX (Pin 53) |
| UART    | LPUART1 TX (logging)         | PA2    | AF12 LPUART1       | Connect to ST-LINK `T_VCP_TX` (USB logging) |
| UART    | LPUART1 RX (logging)         | PA3    | AF12 LPUART1       | Connect to ST-LINK `T_VCP_RX` (USB logging) |
| CAN     | FDCAN1 RX                    | PA11   | AF9 FDCAN1         | 1 Mbps default |
| CAN     | FDCAN1 TX                    | PA12   | AF9 FDCAN1         | 1 Mbps default |
| SPI     | SPI1 SCK                     | PA5    | AF5 SPI1           | MAX17841B / ASCI |
| SPI     | SPI1 MISO                    | PA6    | AF5 SPI1           |  |
| SPI     | SPI1 MOSI                    | PA7    | AF5 SPI1           |  |
| SPI     | SPI1 CS (software)           | PA4    | GPIO output        | MAX17841B chip select (manual) |
| LED     | LED1                         | PA9    | GPIO output        | Active high |
| LED     | LED2                         | PA8    | GPIO output        | Active high |
| LED     | LED3                         | PC9    | GPIO output        | Active high |
| LED     | LED4                         | PC8    | GPIO output        | Active high |
| LED     | LED5                         | PC7    | GPIO output        | Fault / Error_Handler indicator |
| GPIO    | Spare                        | PC6    | GPIO               | Unused |
| GPIO    | Spare / analog header        | PB15   | GPIO/Analog        | Unused |
| GPIO    | Spare / analog header        | PB13   | GPIO/Analog        | Unused |
| ADC1    | IN1 (Thermistor 1)           | PA0    | Analog             | ADC_CHANNEL_1 |
| ADC1    | IN2 (Thermistor 2)           | PA1    | Analog             | ADC_CHANNEL_2 |
| ADC1    | IN11 (Thermistor 3)          | PB12   | Analog             | ADC_CHANNEL_11 |
| ADC1    | IN14 (Thermistor 4)          | PB11   | Analog             | ADC_CHANNEL_14 |
| ADC1    | IN5 (Thermistor 5)           | PB14   | Analog             | ADC_CHANNEL_5 |
| ADC1    | IN6 (Thermistor 6)           | PC0    | Analog             | ADC_CHANNEL_6 |
| ADC1    | IN7 (Thermistor 7)           | PC1    | Analog             | ADC_CHANNEL_7 |
| ADC1    | IN8 (Thermistor 8)           | PC2    | Analog             | ADC_CHANNEL_8 |
| ADC1    | IN9 (Thermistor 9)           | PC3    | Analog             | ADC_CHANNEL_9 |
| ADC1    | IN15 (Thermistor 10)         | PB0    | Analog             | ADC_CHANNEL_15 |

Notes
- BQ79616 runs on USART1 (PC4/PC5). Logging defaults to LPUART1 (PA2/PA3) to keep pins separated.
- PA4 is the software CS for MAX17841B (SPI1).
- All LEDs are active high; LED5 is the fault indicator.
- No DMA is used; UARTs/SPI are blocking for bring-up.
- BQ79616 datasheet reference: https://www.ti.com/lit/ds/symlink/bq79612-q1.pdf
