# BQ79616-Q1 Sample Code for TMS570LS1224 (LAUNCH XL2)
This repository contains the BQ79616-Q1 sample code for the TMS570LS1224. This includes many useful functions that can be used for future development for the BQ79616-Q1.
## Pinout
CONNECTIONS BETWEEN BQ79616EVM AND LAUNCH XL2 (TMS570LS1224): 

* bq79616EVM J17 pin 1 (NC)     -> FLOAT
* bq79616EVM J17 pin 2 (NC)     -> FLOAT
* bq79616EVM J17 pin 3 (NFAULT) -> LAUNCH XL2 J4 pin 10 (PA0)
* bq79616EVM J17 pin 4 (NC)     -> FLOAT
* bq79616EVM J17 pin 5 (GND)    -> LAUNCH XL2 J3 pin 2  (GND)
* bq79616EVM J17 pin 6 (3V3)    -> LAUNCH XL2 J2 pin 1  (3V3)
* bq79616EVM J17 pin 7 (RX)     -> LAUNCH XL2 J2 pin 4  (UATX)
* bq79616EVM J17 pin 8 (TX)     -> LAUNCH XL2 J2 pin 3  (UARX)
* bq79616EVM J17 pin 9 (NC)     -> FLOAT
* bq79616EVM J17 pin 10 (NC)    -> FLOAT

RELEVANT MODIFIED FILES:

* bq79616.h        must change TOTALBOARDS and MAXBYTES here for code to function
* bq79616.c        contains all relevant functions used in the sample code
* notification.c   sets UART_RX_RDY and RTI_TIMEOUT when their respective interrupts happen
* .dil/.hcg        used for generating the basic TMS570LS1224 code files, can be used to make changes to the microcontroller