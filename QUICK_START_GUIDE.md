/*
 * ===========================================================================
 * BQ79616 WAKE-UP SEQUENCE DEBUGGING: QUICK START GUIDE
 * ===========================================================================
 *
 * Your problem: UART works but analog power won't turn on.
 * Root cause: The TX-low wake pulses likely aren't generating correctly.
 *
 * We've created comprehensive debugging tools and documentation.
 * Start here!
 *
 * ===========================================================================
 * FILES WE CREATED FOR YOU
 * ===========================================================================
 *
 * 1. src/bq79616_debug.c + include/bq79616_debug.h
 *    └─ Debug version of wake sequence with detailed logging
 *    └─ Tracks timing, GPIO states, UART status
 *    └─ Provides real-time feedback on what's failing
 *
 * 2. include/WAKE_DEBUGGING_GUIDE.h
 *    └─ Detailed step-by-step debugging procedure
 *    └─ Common failure modes and how to fix them
 *    └─ Hardware verification checklist
 *
 * 3. DEBUG_ACTION_PLAN.md (THIS FOLDER)
 *    └─ Concise 5-step action plan with expected values
 *    └─ What to look for in log output
 *    └─ How to interpret results
 *
 * 4. WAKE_TIMING_DIAGRAM.txt (THIS FOLDER)
 *    └─ ASCII timing diagram showing exactly what should happen
 *    └─ Millisecond-by-millisecond timeline
 *    └─ Visual reference to understand the sequence
 *
 * 5. REGISTER_REFERENCE.md (THIS FOLDER)
 *    └─ Explanation of BQ79616 registers involved in wake-up
 *    └─ Register bit layouts and what each bit does
 *    └─ How to read STATUS register to verify wake success
 *
 * ===========================================================================
 * WHAT WE MODIFIED IN YOUR CODE
 * ===========================================================================
 *
 * ✓ include/master.h
 *   └─ Added #include "bq79616_debug.h"
 *
 * ✓ src/main.c
 *   └─ Changed BQ_WakeSequence(1u) → BQ_WakeSequence_Debug(1u, &debug_info)
 *   └─ Added LOG_INFO calls to output debug data
 *
 * No other functional changes! All original code is preserved.
 *
 *
 * ===========================================================================
 * QUICK START: 5 MINUTES
 * ===========================================================================
 *
 * 1. BUILD & UPLOAD
 *    └─ Use PlatformIO: Build
 *    └─ Upload to board
 *    └─ Should compile with NO ERRORS
 *
 * 2. OPEN SERIAL MONITOR
 *    └─ Speed: 1,000,000 baud (or check your UART config)
 *    └─ Watch for log output starting with:
 *        "[INFO] === BQ79616 WAKE DEBUG START ==="
 *
 * 3. COPY OUTPUT TO DEBUG_ACTION_PLAN.md
 *    └─ Read STEP 1-5 in DEBUG_ACTION_PLAN.md
 *    └─ Compare your output to expected values
 *    └─ Find which step fails
 *
 * 4. APPLY FIX FOR FAILING STEP
 *    └─ See "FIX" section in that step
 *    └─ Make correction to code
 *    └─ Re-test
 *
 * 5. VERIFY SUCCESS
 *    └─ Look for: "Wake sequence RC: 0"
 *    └─ Look for: "SEND_WAKE RC: 0"
 *    └─ Analog power should now be ON!
 *
 *
 * ===========================================================================
 * EXPECTED LOG OUTPUT (HEALTHY WAKE SEQUENCE)
 * ===========================================================================
 *
 * [INFO] === BQ79616 WAKE DEBUG START ===
 * [INFO] TX pin initial state: 1
 * [INFO] Step 1: Disabling UART and converting TX to GPIO output
 * [INFO] Step 2: Sending WAKE pulse 1 (2.75 ms)
 * [INFO]   Pulse 1 width: 2750 us (expected ~2750 us)
 * [INFO]   TX pin state after pulse 1: 1
 * [INFO] Step 3: Waiting shutdown interval (3.5 ms)
 * [INFO] Step 4: Sending WAKE pulse 2 (2.75 ms)
 * [INFO]   Pulse 2 width: 2750 us (expected ~2750 us)
 * [INFO]   TX pin state after pulse 2: 1
 * [INFO] Step 5: Waiting for bridge ACTIVE state (3.5 ms)
 * [INFO] Step 6: Restoring UART functionality
 * [INFO]   UART reinitialized successfully
 * [INFO] Step 7: Sending CONTROL1[SEND_WAKE] command via UART
 * [INFO]   SEND_WAKE transmitted successfully
 * [INFO] Step 8: Waiting for stack propagation (11600 us for 1 device(s))
 * [INFO] TX pin final state: 1
 * [INFO] === BQ79616 WAKE DEBUG COMPLETE (rc=0) ===
 * [INFO] Wake sequence RC: 0
 * [INFO] Pulse 1 width: 2750 us
 * [INFO] Pulse 2 width: 2750 us
 * [INFO] SEND_WAKE RC: 0
 *
 * If your output matches this exactly → SUCCESS!
 * Analog power should be ON. Device is woken.
 *
 *
 * ===========================================================================
 * COMMON ISSUES & QUICK FIXES
 * ===========================================================================
 *
 * ISSUE #1: "Pulse width: 0 us" or "Pulse width: 1 us"
 * ─────────────────────────────────────────────────────
 * Cause: GPIO not working, pin not configured as output
 * Fix: Check GPIOC PIN 4 configuration in your GPIO setup
 *      Ensure it's set to GPIO_MODE_OUTPUT_PP (push-pull)
 * Test: Use oscilloscope on GPIOC PIN 4, should see pulses go LOW then HIGH
 *
 *
 * ISSUE #2: "TX pin state after pulse: 0" (stuck LOW)
 * ─────────────────────────────────────────────────────
 * Cause: GPIO.PIN_SET not working, pin stuck LOW
 * Fix: Check GPIO pull configuration, might have pull-down
 *      Verify GPIOC PIN 4 physically returns to HIGH
 * Test: Restart device, check if pin recovers in next cycle
 *       If never recovers, hardware problem (damaged pin, short to ground)
 *
 *
 * ISSUE #3: "Pulse width: 5500 us" (twice expected)
 * ─────────────────────────────────────────────────
 * Cause: SystemCoreClock value is wrong in code
 * Fix: Check actual MCU clock speed
 *      In SystemInit() or system file, verify:
 *      SystemCoreClock = 170000000  // STM32G4 typical
 * Test: Re-calibrate delay function using oscilloscope
 *
 *
 * ISSUE #4: "SEND_WAKE RC: -X" (write failed)
 * ──────────────────────────────────────────
 * Cause: Device not responding to UART (still SHUTDOWN)
 * Fix: Make sure ISSUE #1 and #2 are PASS first
 *      If pulses look good, check UART baud rate
 *      Default is 1,000,000 baud - verify your UART config
 * Test: Use logic analyzer on TX/RX lines during wake sequence
 *       Should see TX-low pulses, then UART traffic ~13ms later
 *
 *
 * ISSUE #5: All tests pass but analog power still OFF
 * ─────────────────────────────────────────────────────
 * Cause: Different BQ79616 revision, different control bit
 * Fix: Check your BQ79616 datasheet for:
 *      - CONTROL1 register bit layout (may not be bit 5 for SEND_WAKE)
 *      - STATUS register ACTIVE bit location
 *      - Any additional setup registers needed
 * Test: Read STATUS register after wake, check if ACTIVE bit is set
 *       Look at register values via DEBUG_ACTION_PLAN.md Step 5
 *
 *
 * ===========================================================================
 * WHERE TO GET HELP
 * ===========================================================================
 *
 * 1. Read WAKE_DEBUGGING_GUIDE.h for detailed step-by-step procedure
 *
 * 2. Check WAKE_TIMING_DIAGRAM.txt for visual reference
 *
 * 3. Consult REGISTER_REFERENCE.md for register details
 *
 * 4. Review DEBUG_ACTION_PLAN.md for checklist approach
 *
 * 5. Compare log output to EXPECTED LOG OUTPUT above
 *
 * 6. Use oscilloscope on GPIOC PIN 4 and TX/RX lines to verify
 *    hardware-level behavior
 *
 * 7. Check BQ79616 datasheet for:
 *    - UART protocol details
 *    - Register definitions for your specific revision
 *    - Wake sequence timing requirements
 *    - Power state transition diagrams
 *
 *
 * ===========================================================================
 * HARDWARE VERIFICATION FIRST!
 * ===========================================================================
 *
 * Before debugging code, verify hardware is correct:
 *
 * CHECKLIST:
 * ☐ GPIOC PIN 4 is physically connected to BQ79616 RXD pin
 * ☐ GPIOC PIN 5 (or your RX) connected to BQ79616 TXD pin  
 * ☐ Common GND between MCU and BQ79616
 * ☐ BQ79616 power supply is connected and stable (check voltage)
 * ☐ UART baud rate matches BQ79616 (1,000,000 typical)
 * ☐ No level shifters OR they're configured correctly
 * ☐ Oscilloscope probes connected to verify signals
 *
 * If any of these are wrong, no amount of code debugging will help!
 *
 *
 * ===========================================================================
 * NEXT STEPS
 * ===========================================================================
 *
 * 1. Build your code now (no changes needed, already updated)
 * 2. Watch serial monitor for debug output
 * 3. Open DEBUG_ACTION_PLAN.md and follow Step 1-5
 * 4. Report back with the exact log output you see
 * 5. I'll help you interpret and fix any failures
 *
 * ===========================================================================
 */
