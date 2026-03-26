# CAN Timing Issue and Fix

## Summary

The CAN transmit timing bug had two separate causes:

1. The dedicated CAN timer (`TIM7`) was not configured as a true `100 ms` hardware timer.
2. The main loop was being blocked for about `2 seconds` by debug fault reads to the BQ device, which made correctly-timed CAN flags get serviced very late.

Because of that combination, CAN traffic did not appear at the intended rates:

- J1939 claim should be every `200 ms`
- BMS broadcast should be every `100 ms`
- General broadcast should be every `100 ms`

Instead, logs showed intervals such as:

```text
[DEBUG] TX 0x18EEFF80 claim interval=2094ms result=0
[DEBUG] TX 0x1839F380 bms interval=2094ms result=0 ...
[DEBUG] TX 0x1838F380 gen interval=2094ms result=0 ...
```

That `2094 ms` value was not the timer period itself. It was the amount of time the main loop was stalled before it came back and finally serviced the pending CAN flags.

---

## What the Issue Was

### Issue 1: `TIM7` was not a real `100 ms` CAN timer

The code originally treated `TIM7` like this:

- `TIMER7_CLK_HZ = 10000`
- `TIMER7_PERIOD = 100`

That means:

- timer tick = `0.1 ms`
- period = `100 counts`
- interrupt every `10 ms`

So `TIM7` was actually firing every `10 ms`, not every `100 ms`.

The old design then used software counters:

- `CAN_TICKS_FOR_100MS = 10`
- `CAN_TICKS_FOR_200MS = 20`

That can work in principle, but it was not what was intended when the goal was a separate CAN timer that directly supports `100 ms` and `200 ms` scheduling.

### Issue 2: `TIM7` interrupt wiring was incomplete

On the STM32G474, `TIM7` does not use `TIM7_IRQn`.
It shares the interrupt vector with DAC and uses:

- `TIM7_DAC_IRQn`
- `TIM7_DAC_IRQHandler`

The project initially had the wrong interrupt hookup for `TIM7`, so the dedicated CAN timer path was not fully connected to the MCU's actual vector table.

### Issue 3: Main loop was blocked by BQ fault reads

After fixing the timer path, the log still showed about `2094 ms` intervals. That turned out to be a different problem.

In `main.c`, the loop did this every `200 ms`:

- read `FAULT_SYS`
- read `FAULT_COMM1`

Both calls used `bq79616_read()`, and that wrapper used a hard-coded `1000 ms` timeout.

So when the BQ read did not return immediately, the loop could block for:

- `1000 ms` on the first read
- `1000 ms` on the second read
- total about `2000 ms`

That lines up almost exactly with the observed `2094 ms`.

Since `CAN_SendMessages()` runs in the main loop, not inside the interrupt, the CAN timer flags were set on time by the ISR but were not consumed until the main loop came back.

That is why all three message intervals looked late by the same amount.

---

## How We Fixed It

### Fix 1: Make `TIM7` a real `100 ms` hardware timer

`TIM7` was changed to use:

- `TIMER7_CLK_HZ = 10000`
- `TIMER7_PERIOD = 1000`

Math:

- `10000 Hz` timer clock = `0.1 ms` per count
- `1000 counts` = `100 ms` per interrupt

Then the CAN timing flags became:

- `CAN_TICKS_FOR_100MS = 1`
- `CAN_TICKS_FOR_200MS = 2`

So now:

- every `TIM7` interrupt = `100 ms`
- every second `TIM7` interrupt = `200 ms`

This is simpler and matches the intended behavior.

### Fix 2: Use the correct STM32G474 `TIM7` interrupt

The dedicated CAN timer now uses:

- `HAL_NVIC_SetPriority(TIM7_DAC_IRQn, ...)`
- `HAL_NVIC_EnableIRQ(TIM7_DAC_IRQn)`
- `TIM7_DAC_IRQHandler()`

This matches the STM32G474 startup vector and ensures `HAL_TIM_IRQHandler(&g_htim7)` is actually called.

### Fix 3: Sto p the BQ fault reads from stalling the loop

A new helper was added:

```c
int bq79616_read_timeout(..., uint32_t timeout_ms);
```

Then the periodic fault-check reads in `main.c` were changed to use a short timeout:

- `BQ_FAULT_READ_TIMEOUT_MS = 10`

Instead of waiting up to `1000 ms` per read, each debug fault read now gives up quickly if the device does not answer.

This keeps the main loop responsive so `CAN_SendMessages()` can run often enough to service `100 ms` and `200 ms` flags on time.

### Fix 4: Remove noisy keep-alive success spam

The loop was printing:

```text
[DEBUG] Keep-alive write OK (0x21)
```

every `20 ms`.

That output was removed for successful writes and kept only for failures. This reduces UART spam and makes CAN timing logs easier to see.

---

## Why the Fix Works

### Why the timer fix works

The CAN scheduler now has a dedicated timer whose base interrupt period is already `100 ms`.

That means the logic is straightforward:

- one interrupt -> set `100 ms` flag
- two interrupts -> set `200 ms` flag

There is no longer a `10 ms` base period on `TIM7` that must be divided by `10` in software just to reach the intended CAN timing.

### Why the interrupt fix works

Even correct timer register values do nothing if the MCU never enters the interrupt handler.

Using the real STM32G474 vector names fixes that path end-to-end:

- timer update event occurs
- NVIC sees `TIM7_DAC_IRQn`
- `TIM7_DAC_IRQHandler()` runs
- HAL dispatches to `HAL_TIM_PeriodElapsedCallback()`
- CAN flags are set

### Why the BQ timeout fix works

The CAN send path is not interrupt-driven all the way through.

The interrupt only sets flags.
Actual CAN message transmission still happens in the main loop.

So if the main loop blocks for `~2 seconds`, CAN messages will also appear `~2 seconds` late even when the timer ISR is correct.

By shortening the debug fault-read timeout from `1000 ms` to `10 ms`, the loop is no longer monopolized by those reads, and the CAN transmit service can run at the expected cadence.

---

## Files Changed

- `include/timer.h`
- `src/timer.c`
- `src/stm32g4xx_it.c`
- `include/bq79616.h`
- `src/bq79616.c`
- `src/main.c`

---

## Expected Behavior After the Fix

After flashing the updated firmware:

- claim message interval should be about `200 ms`
- BMS broadcast interval should be about `100 ms`
- general broadcast interval should be about `100 ms`

The CAN logs are still rate-limited for readability, so you may not see a log line on every single transmit event, but the `interval=` values should now be near the intended timing instead of `2094 ms`.

---

## Verification

Build verification completed successfully:

```bash
pio run
```

Expected runtime checks:

1. Boot the board and watch the console.
2. Confirm `TIM7 (CAN-DEDICATED)` initialization reports a `100 ms` interrupt period.
3. Confirm CAN debug lines report intervals near `100 ms` and `200 ms`.
4. Confirm the old `2094 ms` pattern does not return unless the main loop becomes blocked again.
