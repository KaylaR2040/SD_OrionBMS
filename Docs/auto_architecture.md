# EV Active Sensor Adapter - Firmware System Architecture

## System Overview

```mermaid
flowchart LR
    subgraph SCHED["Execution and Scheduling"]
        direction TB
        BOOT["Boot and Init"]
        TICK["1ms Timebase"]
        LOOP["Main Service Loop"]
        BOOT --> TICK --> LOOP
    end

    subgraph VOLT["Voltage Lane"]
        direction TB
        V_IN["Cell Taps x14"]
        V_AFE["Analog Front-End"]
        V_ADC["External HV ADC"]
        V_TRS["UART TO SPI TRANSLATOR"]
        V_SPI["SPI Driver"]
        V_PROC["Voltage Processing"]
        V_IN --> V_AFE --> V_ADC <--> V_TRS <-->V_SPI --> V_PROC
    end

    subgraph TEMP["Temperature Lane"]
        direction TB
        T_IN["Thermistors x10"]
        T_ADC["STM32 ADC"]
        T_CONV["Lookup Table Conversion"]
        T_PROC["Temp Processing"]
        T_IN --> T_ADC --> T_CONV --> T_PROC
    end

    subgraph MODEL["Data Model"]
        direction TB
        BUF["Measurement Buffers"]
        META["Timestamps"]
        FAULTS["Fault State"]
        BUF --- META
        META --- FAULTS
    end

    subgraph FRAME["Message Assembly"]
        direction TB
        PACK["Frame Builder"]
        CRC["Checksum"]
        RATE["Rate Control"]
        PACK --> CRC --> RATE
    end

    subgraph CANNET["CAN Transport"]
        direction TB
        CAN_API["CAN API"]
        CAN_HAL["FDCAN HAL"]
        BUS["CAN Bus"]
        CAN_API --> CAN_HAL --> BUS
        BUS --> CAN_HAL --> CAN_API
    end

    subgraph DBG["Debug Interface"]
        direction TB
        LEDS["Status LEDs"]
        UART["UART Console"]
    end

    subgraph CONSUMERS["External Consumers"]
        direction TB
        BMS["Orion BMS"]
        LOGGER["Data Logger"]
    end

    LOOP --> V_SPI
    LOOP --> T_ADC
    V_PROC --> BUF
    T_PROC --> BUF
    BUF --> PACK
    META --> PACK
    FAULTS --> PACK
    RATE --> CAN_API
    CAN_API --> LEDS
    CAN_API --> UART
    FAULTS --> LEDS
    FAULTS --> UART
    BUS --> BMS
    BUS --> LOGGER
```

## Timing Constraints

```mermaid
flowchart TB
    subgraph TIMING["Message Timing Requirements"]
        direction LR
        T1["J1939 Address Claim"]
        T2["BMS Broadcast"]
        T3["General Broadcast"]
        T4["ADC Sampling"]
        T5["SPI Polling"]
    end

    T1 --- PERIOD1["200 ms period"]
    T2 --- PERIOD2["100 ms period"]
    T3 --- PERIOD3["100 ms period, round-robin"]
    T4 --- PERIOD4["Configurable, typical 100 ms"]
    T5 --- PERIOD5["Configurable, typical 200 ms"]
```

## Data Flow Detail

```mermaid
flowchart TB
    subgraph ADC_ACQUIRE["ADC Acquisition"]
        ADC_TASK["ADC_ServiceTask()"]
        ADC_SAMPLE["Sample 10 channels"]
        ADC_RAW["Raw ADC counts 0-4095"]
    end

    subgraph CONVERT["Temperature Conversion"]
        LOOKUP["Lookup Table"]
        CLAMP["Clamp to int8 range"]
        TEMPS["s_temps array"]
    end

    subgraph ENCODE["CAN Encoding"]
        BMS_ENC["EncodeBMSBroadcast()"]
        GEN_ENC["EncodeGeneralBroadcast()"]
        PAYLOAD["8-byte payload"]
    end

    subgraph TRANSMIT["CAN Transmit"]
        SEND_EXT["CAN_Comm_SendExt()"]
        FDCAN["FDCAN Peripheral"]
        WIRE["Physical Bus"]
    end

    ADC_TASK --> ADC_SAMPLE --> ADC_RAW
    ADC_RAW --> LOOKUP --> CLAMP --> TEMPS
    TEMPS --> BMS_ENC --> PAYLOAD
    TEMPS --> GEN_ENC --> PAYLOAD
    PAYLOAD --> SEND_EXT --> FDCAN --> WIRE
```

## Module Responsibilities

| Module | File | Purpose | Timing |
|--------|------|---------|--------|
| Main | main.c | Init and main loop | Continuous |
| ADC | adc.c | Sample thermistor channels | Periodic |
| SPI | spi.c | Poll MAX17841 for cell data | Periodic |
| Thermistor | thermistor_table.c | ADC to temperature lookup | On demand |
| CAN Messages | can_messages.c | Encode BMS and General frames | On demand |
| CAN Core | can.c | FDCAN init, send, receive | Periodic |
| Timer | timer.c | HAL_GetTick based timing | 1 ms tick |
| Log | log.c | UART debug output | On demand |
| LED | led.c | Status indication | Periodic |
| Error | error_handling.c | Fault handling | On event |

## CAN Message Schedule

| Message | CAN ID | Period | Payload |
|---------|--------|--------|---------|
| J1939 Address Claim | 0x18EEFF80 | 200 ms | 8 bytes fixed |
| BMS Broadcast | 0x1839F380 | 100 ms | Module summary |
| General Broadcast | 0x183BF380 | 100 ms | Single thermistor, round-robin |

## Temperature Data Path

```mermaid
flowchart LR
    THERM["Thermistor"] --> DIVIDER["Voltage Divider"] --> PIN["ADC Pin"]
    PIN --> SAMPLE["ADC Sample"] --> COUNT["0-4095 count"]
    COUNT --> TABLE["Lookup Table"] --> SIGNED["int8_t temp"]
    SIGNED --> CAST["cast to uint8_t"] --> WIRE["CAN payload byte"]
    WIRE --> BMS["Orion BMS interprets as int8_t"]
```

## Initialization Sequence

```mermaid
flowchart TB
    START["Power On"] --> HAL["HAL_Init()"]
    HAL --> CLK["SystemClock_Config()"]
    CLK --> GPIO["GPIO_App_Init()"]
    GPIO --> UART["UART_Log_Init()"]
    UART --> ADC["ADC_App_Init()"]
    ADC --> SPI["SPI_App_Init()"]
    SPI --> CAN["CAN_App_Init()"]
    CAN --> LED["LED_Init()"]
    LED --> LOOP["Enter Main Loop"]
```

## Main Loop Execution

```mermaid
flowchart TB
    LOOP["Main Loop Start"] --> CHECK_ADC{"ADC period elapsed?"}
    CHECK_ADC -- yes --> RUN_ADC["ADC_ServiceTask()"]
    CHECK_ADC -- no --> CHECK_SPI
    RUN_ADC --> CHECK_SPI{"SPI period elapsed?"}
    CHECK_SPI -- yes --> RUN_SPI["SPI_ServiceTask()"]
    CHECK_SPI -- no --> CHECK_CAN
    RUN_SPI --> CHECK_CAN{"CAN period elapsed?"}
    CHECK_CAN -- yes --> RUN_CAN["CAN_ServiceTask()"]
    CHECK_CAN -- no --> LOOP
    RUN_CAN --> LOOP
```