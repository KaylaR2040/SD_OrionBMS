/*
 * ===========================================================================
 * BQ79616 WAKE-UP SEQUENCE: REGISTER GUIDE
 * ===========================================================================
 *
 * This explains which registers are involved in waking up the BQ79616 from
 * SHUTDOWN mode and what they do.
 *
 * ===========================================================================
 * OVERVIEW OF DEVICE POWER STATES
 * ===========================================================================
 *
 * The BQ79616 has two main power modes:
 *
 * 1. SHUTDOWN MODE (Default after power-up)
 *    ├─ Minimal current draw
 *    ├─ No analog power to battery monitoring circuits
 *    ├─ No UART communications possible
 *    └─ Woken by: Two TX-low pulses on UART RXD line
 *
 * 2. ACTIVE MODE (After successful wake)
 *    ├─ Full power to analog circuits
 *    ├─ UART communication enabled
 *    ├─ Battery monitoring active
 *    └─ Returns to SHUTDOWN after timeout (if enabled)
 *
 *
 * ===========================================================================
 * REGISTER: CONTROL1 (Address 0x0001)
 * ===========================================================================
 *
 * This is the KEY register for wake control!
 *
 * BIT LAYOUT:
 * ───────────────────────────────────────────────────────────────────
 * BIT#  | NAME              | PURPOSE
 * ───────────────────────────────────────────────────────────────────
 *  7-6  | (reserved)        | Must be 0
 *  5    | SEND_WAKE         | ← THIS WAKES STACKED DEVICES
 *  4-2  | (reserved)        | Must be 0
 *  1-0  | (reserved)        | Must be 0
 * ───────────────────────────────────────────────────────────────────
 *
 * SEND_WAKE BIT (Bit 5):
 * ├─ Write 1: Device sends wake signal down the stack
 * ├─ Write 0: Normal operation
 * └─ Used in: bq79616_stack_single_init()
 *
 * YOUR CODE SENDS:
 * ────────────────
 *   uint8_t control1 = BQ_CONTROL1_SEND_WAKE_MASK;  // 0x20 = 0b0010_0000
 *   bq79616_write(0u, BQ_REG_CONTROL1_ADDR, &control1, 1u);
 *
 * This sets SEND_WAKE=1 which tells the bridge to pass the wake signal to
 * any stacked BQ79616 devices below it.
 *
 *
 * ===========================================================================
 * REGISTER: STATUS (Address 0x0000) - READ ONLY
 * ===========================================================================
 *
 * This tells you what mode the device is currently in.
 *
 * BIT LAYOUT (typical):
 * ─────────────────────────────────────────────────────
 * BIT#  | NAME                    | MEANING
 * ─────────────────────────────────────────────────────
 *  7    | No Cell Detected        | If 1: No cells detected
 *  6    | Cell Balance Mode       | If 1: Cell balancing active
 *  5    | Thermistor Mode         | If 1: Thermistor connected
 *  4    | ACTIVE                  | If 1: Device is ACTIVE ← CHECK THIS
 *  3    | (other)                 |
 *  2    | (other)                 |
 *  1    | (other)                 |
 *  0    | (other)                 |
 * ─────────────────────────────────────────────────────
 *
 * ⚠️  NOTE: Bit definitions vary by BQ79616 revision!
 *    Consult your specific datasheet for exact STATUS layout.
 *
 * TO CHECK IF DEVICE IS ACTIVE:
 * ─────────────────────────────
 *   uint8_t status = 0;
 *   bq79616_read(0u, 0x0000u, &status, 1u);
 *   
 *   if (status & 0x10) {  // Check bit 4 (example - verify your datasheet!)
 *       // Device is ACTIVE - analog power should be ON
 *       LOG_INFO("SUCCESS: Device is ACTIVE, analog power enabled");
 *   } else {
 *       // Device is still SHUTDOWN - wake sequence failed
 *       LOG_ERROR("FAIL: Device is SHUTDOWN, analog power OFF");
 *       LOG_ERROR("Status register: 0x%02X", status);
 *   }
 *
 *
 * ===========================================================================
 * INITIALIZATION SEQUENCE (Your Code Flow)
 * ===========================================================================
 *
 * in main.c:
 *
 *   // Step 1: Apply the two TX-low wake pulses
 *   // This wakes the device FROM SHUTDOWN TO ACTIVE
 *   BQ_WakeSequence(1u);
 *     └─ Generates two 2.75 ms TX-low pulses
 *     └─ Sends CONTROL1[SEND_WAKE]=1 via UART
 *     └─ Waits for propagation
 *
 *   // Step 2: Perform stack initialization  
 *   // This configures the device for operation
 *   bq79616_stack_single_init();
 *     └─ Write CONTROL1 = 0x20 (SEND_WAKE)
 *     └─ Enable auto-addressing (CONTROL1 = 0x01)
 *     └─ Set device address (DIR0 = 0x00)
 *     └─ Set stack configuration (COMM_CTRL = 0x03)
 *
 *
 * ===========================================================================
 * REGISTER: COMM_CTRL (Address 0x0003)
 * ===========================================================================
 *
 * Configures how the device communicates in the stack.
 *
 * BIT LAYOUT:
 * ────────────────────────────────────────────────────
 * BIT#  | NAME                    | PURPOSE
 * ────────────────────────────────────────────────────
 *  7-4  | (reserved)              | Must be 0
 *  3    | STACK_MODE              | If 1: Stack mode enabled
 *  2    | (reserved)              |
 *  1    | TOP_OF_STACK            | If 1: This device is top of stack
 *  0    | (reserved)              |
 * ────────────────────────────────────────────────────
 *
 * YOUR CODE SENDS:
 * ───────────────
 *   uint8_t control = 0x03;  // Binary: 0b0000_0011
 *   bq79616_write(0u, BQ_REG_COMM_CTRL, &control, 1u);
 *
 * This sets:
 *   - STACK_MODE = 1 (bit 3 set via 0x08, but you're writing 0x03...)
 *   - TOP_OF_STACK = 1 (bit 1 set via 0x02)
 *
 * ⚠️  NOTE: 0x03 may not be correct! Check datasheet for proper COMM_CTRL.
 *    Verify what bits actually need to be set for your specific BQ79616.
 *
 *
 * ===========================================================================
 * REGISTER: DIR0 (Address 0x0002)
 * ===========================================================================
 *
 * Device Identification Register - sets the device address in the stack.
 *
 * BIT LAYOUT:
 * ────────────────────────────────────────────────────
 * BIT#  | NAME                    | PURPOSE
 * ────────────────────────────────────────────────────
 *  7-6  | (reserved)              | Must be 0
 *  5-0  | DEVICE_ADDRESS          | Sets address (0-63 typical)
 * ────────────────────────────────────────────────────
 *
 * YOUR CODE SENDS:
 * ───────────────
 *   uint8_t addr = 0x00;
 *   bq79616_write(0u, BQ_REG_DIR0_ADDR, &addr, 1u);
 *
 * This sets the device address to 0x00.
 *
 * IMPORTANT for stacking:
 * ├─ Each device in a stack needs a unique address
 * ├─ Auto-addressing can assign these automatically
 * └─ Master device is usually address 0x00
 *
 *
 * ===========================================================================
 * DEBUGGING: REGISTER READ-BACK
 * ===========================================================================
 *
 * After wake sequence completes, verify register contents:
 *
 *    // Check STATUS
 *    uint8_t status = 0;
 *    bq79616_read(0u, 0x0000u, &status, 1u);
 *    LOG_INFO("STATUS: 0x%02X (expect ACTIVE bit set)", status);
 *    
 *    // Check CONTROL1
 *    uint8_t ctrl1 = 0;
 *    bq79616_read(0u, 0x0001u, &ctrl1, 1u);
 *    LOG_INFO("CONTROL1: 0x%02X (expect 0x20 after wake)", ctrl1);
 *    
 *    // Check COMM_CTRL
 *    uint8_t comm = 0;
 *    bq79616_read(0u, 0x0003u, &comm, 1u);
 *    LOG_INFO("COMM_CTRL: 0x%02X", comm);
 *    
 *    // Check DIR0 (address)
 *    uint8_t addr = 0;
 *    bq79616_read(0u, 0x0002u, &addr, 1u);
 *    LOG_INFO("DIR0: 0x%02X (expect 0x00)", addr);
 *
 * If reads succeed but STATUS doesn't show ACTIVE:
 *   → Device woke temporarily but fell back to SHUTDOWN
 *   → May need longer wait after wake, or additional configuration
 *   → Check BQ79616 datasheet for idle timeout settings
 *
 * If reads fail (CRC errors, no response):
 *   → Device is NOT responding to UART
 *   → Wake pulses likely didn't work
 *   → Go back to oscilloscope verification
 *
 *
 * ===========================================================================
 * REGISTER ADDRESSES REFERENCE
 * ===========================================================================
 *
 * Address  | Register Name              | R/W | Purpose
 * ---------|----------------------------|-----|----------------------------------
 * 0x0000   | STATUS                     | R   | Device status (ACTIVE bit, etc)
 * 0x0001   | CONTROL1                   | R/W | SEND_WAKE, wake configuration
 * 0x0002   | DIR0                       | R/W | Device address in stack
 * 0x0003   | COMM_CTRL                  | R/W | Stack mode, top-of-stack flag
 * 0x0004   | (depends on revision)      | -   | -
 * ...      | (consult datasheet)        | -   | -
 * 0x0080   | OTP_ECC_DATAIN1            | R   | OTP/ECC test register
 * 
 * ⚠️  Register layout varies by BQ79616 revision!
 *    Always consult the specific datasheet for your part number.
 *
 * ===========================================================================
 */
