/*
 *  @file bq79616.c
 *
 *  @author Vince Toledo - Texas Instruments Inc.
 *  @date February 2020
 *  @version 1.2
 *  @note Built with CCS for Hercules Version: 8.1.0.00011
 *  @note Built for TMS570LS1224 (LAUNCH XL2)
 */

/*****************************************************************************
 **
 **  Copyright (c) 2011-2019 Texas Instruments
 **
 ******************************************************************************/

#include <bq79616.h>
#include "string.h"
#include "sci.h"
#include "rti.h"
#include "gio.h"
#include "datatypes.h"


//GLOBAL VARIABLES (use these to avoid stack overflows by creating too many function variables)
//avoid creating variables/arrays in functions, or you will run out of stack space quickly
BYTE response_frame2[(MAXBYTES+6)*TOTALBOARDS]; //response frame to be used by every read
BYTE fault_frame[39*TOTALBOARDS]; //hold fault frame if faults are printed
int currentBoard = 0;
int currentCell = 0;
BYTE bReturn = 0;
int bRes = 0;
int count = 10000;
BYTE bBuf[8];
uint8 pFrame[64];
uint16 wCRC = 0;
uint16 wCRC16 = 0;
int crc_i = 0;
static volatile unsigned int delayval = 0; //for delayms and delayus functions

extern int UART_RX_RDY;
extern int RTI_TIMEOUT;

//******
//PINGS
//******
void Wake79616(void) {
    sciREG->GCR1 &= ~(1U << 7U); // put SCI into reset
    sciREG->PIO0 &= ~(1U << 2U); // disable transmit function - now a GPIO
    sciREG->PIO3 &= ~(1U << 2U); // set output to low

    delayus(2500); // WAKE ping = 2ms to 2.5ms
    sciInit();
    sciSetBaudrate(sciREG, BAUDRATE);
}

void SD79616(void) {
    sciREG->GCR1 &= ~(1U << 7U); // put SCI into reset
    sciREG->PIO0 &= ~(1U << 2U); // disable transmit function - now a GPIO
    sciREG->PIO3 &= ~(1U << 2U); // set output to low

    delayus(9000); // SD ping = 7ms to 10ms
    sciInit();
    sciSetBaudrate(sciREG, BAUDRATE);
}

void StA79616(void) {
    sciREG->GCR1 &= ~(1U << 7U); // put SCI into reset
    sciREG->PIO0 &= ~(1U << 2U); // disable transmit function - now a GPIO
    sciREG->PIO3 &= ~(1U << 2U); // set output to low

    delayus(250); // StA ping = 250us to 300us
    sciInit();
    sciSetBaudrate(sciREG, BAUDRATE);
}

void HWRST79616(void) {
    sciREG->GCR1 &= ~(1U << 7U); // put SCI into reset
    sciREG->PIO0 &= ~(1U << 2U); // disable transmit function - now a GPIO
    sciREG->PIO3 &= ~(1U << 2U); // set output to low

    delayus(36000); // StA ping = 36ms
    sciInit();
    sciSetBaudrate(sciREG, BAUDRATE);
}
//**********
//END PINGS
//**********

//**********************
//AUTO ADDRESS SEQUENCE
//**********************
void AutoAddress()
{
    //DUMMY WRITE TO SNCHRONIZE ALL DAISY CHAIN DEVICES DLL (IF A DEVICE RESET OCCURED PRIOR TO THIS)
    WriteReg(0, OTP_ECC_DATAIN1, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN2, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN3, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN4, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN5, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN6, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN7, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN8, 0X00, 1, FRMWRT_ALL_W);

    //ENABLE AUTO ADDRESSING MODE
    WriteReg(0, CONTROL1, 0X01, 1, FRMWRT_ALL_W);

    //SET ADDRESSES FOR EVERY BOARD
    for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
    {
        WriteReg(0, DIR0_ADDR, currentBoard, 1, FRMWRT_ALL_W);
    }

    WriteReg(0, COMM_CTRL, 0x02, 1, FRMWRT_ALL_W); //set everything as a stack device first

    if(TOTALBOARDS==1) //if there's only 1 board, it's the base AND top of stack, so change it to those
    {
        WriteReg(0, COMM_CTRL, 0x01, 1, FRMWRT_SGL_W);
    }
    else //otherwise set the base and top of stack individually
    {
        WriteReg(0, COMM_CTRL, 0x00, 1, FRMWRT_SGL_W);
        WriteReg(TOTALBOARDS-1, COMM_CTRL, 0x03, 1, FRMWRT_SGL_W);
    }

    //SYNCRHONIZE THE DLL WITH A THROW-AWAY READ
    ReadReg(0, OTP_ECC_DATAIN1, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN2, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN3, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN4, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN5, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN6, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN7, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN8, response_frame2, 1, 0, FRMWRT_ALL_R);

    /*//OPTIONAL: read back all device addresses
    for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
    {
        ReadReg(currentBoard, DIR0_ADDR, response_frame2, 1, 0, FRMWRT_SGL_R);
        printf("board %d\n",response_frame2[4]);
    }*/

    //RESET ANY COMM FAULT CONDITIONS FROM STARTUP
    WriteReg(0, FAULT_RST2, 0x03, 1, FRMWRT_ALL_W);

    return;
}
//**************************
//END AUTO ADDRESS SEQUENCE
//**************************


//************************
//WRITE AND READ FUNCTIONS
//************************

//FORMAT WRITE DATA, SEND TO
//BE COMBINED WITH REST OF FRAME
int WriteReg(BYTE bID, uint16 wAddr, uint64 dwData, BYTE bLen, BYTE bWriteType) {
	// device address, register start address, data bytes, data length, write type (single, broadcast, stack)
	bRes = 0;
	memset(bBuf,0,sizeof(bBuf));
	switch (bLen) {
	case 1:
		bBuf[0] = dwData & 0x00000000000000FF;
		bRes = WriteFrame(bID, wAddr, bBuf, 1, bWriteType);
		break;
	case 2:
		bBuf[0] = (dwData & 0x000000000000FF00) >> 8;
		bBuf[1] = dwData & 0x00000000000000FF;
		bRes = WriteFrame(bID, wAddr, bBuf, 2, bWriteType);
		break;
	case 3:
		bBuf[0] = (dwData & 0x0000000000FF0000) >> 16;
		bBuf[1] = (dwData & 0x000000000000FF00) >> 8;
		bBuf[2] = dwData & 0x00000000000000FF;
		bRes = WriteFrame(bID, wAddr, bBuf, 3, bWriteType);
		break;
	case 4:
		bBuf[0] = (dwData & 0x00000000FF000000) >> 24;
		bBuf[1] = (dwData & 0x0000000000FF0000) >> 16;
		bBuf[2] = (dwData & 0x000000000000FF00) >> 8;
		bBuf[3] = dwData & 0x00000000000000FF;
		bRes = WriteFrame(bID, wAddr, bBuf, 4, bWriteType);
		break;
	case 5:
		bBuf[0] = (dwData & 0x000000FF00000000) >> 32;
		bBuf[1] = (dwData & 0x00000000FF000000) >> 24;
		bBuf[2] = (dwData & 0x0000000000FF0000) >> 16;
		bBuf[3] = (dwData & 0x000000000000FF00) >> 8;
		bBuf[4] = dwData & 0x00000000000000FF;
		bRes = WriteFrame(bID, wAddr, bBuf, 5, bWriteType);
		break;
	case 6:
		bBuf[0] = (dwData & 0x0000FF0000000000) >> 40;
		bBuf[1] = (dwData & 0x000000FF00000000) >> 32;
		bBuf[2] = (dwData & 0x00000000FF000000) >> 24;
		bBuf[3] = (dwData & 0x0000000000FF0000) >> 16;
		bBuf[4] = (dwData & 0x000000000000FF00) >> 8;
		bBuf[5] = dwData & 0x00000000000000FF;
		bRes = WriteFrame(bID, wAddr, bBuf, 6, bWriteType);
		break;
	case 7:
		bBuf[0] = (dwData & 0x00FF000000000000) >> 48;
		bBuf[1] = (dwData & 0x0000FF0000000000) >> 40;
		bBuf[2] = (dwData & 0x000000FF00000000) >> 32;
		bBuf[3] = (dwData & 0x00000000FF000000) >> 24;
		bBuf[4] = (dwData & 0x0000000000FF0000) >> 16;
		bBuf[5] = (dwData & 0x000000000000FF00) >> 8;
		bBuf[6] = dwData & 0x00000000000000FF;
		bRes = WriteFrame(bID, wAddr, bBuf, 7, bWriteType);
		break;
	case 8:
		bBuf[0] = (dwData & 0xFF00000000000000) >> 56;
		bBuf[1] = (dwData & 0x00FF000000000000) >> 48;
		bBuf[2] = (dwData & 0x0000FF0000000000) >> 40;
		bBuf[3] = (dwData & 0x000000FF00000000) >> 32;
		bBuf[4] = (dwData & 0x00000000FF000000) >> 24;
		bBuf[5] = (dwData & 0x0000000000FF0000) >> 16;
		bBuf[6] = (dwData & 0x000000000000FF00) >> 8;
		bBuf[7] = dwData & 0x00000000000000FF;
		bRes = WriteFrame(bID, wAddr, bBuf, 8, bWriteType);
		break;
	default:
		break;
	}
	return bRes;
}

//GENERATE COMMAND FRAME
int WriteFrame(BYTE bID, uint16 wAddr, BYTE * pData, BYTE bLen, BYTE bWriteType) {
	int bPktLen = 0;
	uint8 * pBuf = pFrame;
	memset(pFrame, 0x7F, sizeof(pFrame));
	*pBuf++ = 0x80 | (bWriteType) | ((bWriteType & 0x10) ? bLen - 0x01 : 0x00); //Only include blen if it is a write; Writes are 0x90, 0xB0, 0xD0
	if (bWriteType == FRMWRT_SGL_R || bWriteType == FRMWRT_SGL_W)
	{
		*pBuf++ = (bID & 0x00FF);
	}
	*pBuf++ = (wAddr & 0xFF00) >> 8;
	*pBuf++ = wAddr & 0x00FF;

	while (bLen--)
		*pBuf++ = *pData++;

	bPktLen = pBuf - pFrame;

	wCRC = CRC16(pFrame, bPktLen);
	*pBuf++ = wCRC & 0x00FF;
	*pBuf++ = (wCRC & 0xFF00) >> 8;
	bPktLen += 2;
	//THIS SEEMS to occasionally drop bytes from the frame. Sometimes is not sending the last frame of the CRC.
	//(Seems to be caused by stack overflow, so take precautions to reduce stack usage in function calls)
	sciSend(sciREG, bPktLen, pFrame);

	return bPktLen;
}

//GENERATE READ COMMAND FRAME AND THEN WAIT FOR RESPONSE DATA (INTERRUPT MODE FOR SCIRX)
int ReadReg(BYTE bID, uint16 wAddr, BYTE * pData, BYTE bLen, uint32 dwTimeOut,
        BYTE bWriteType) {
    // device address, register start address, byte frame pointer to store data, data length, read type (single, broadcast, stack)
    bRes = 0;
    count = 1000000; //timeout after this many attempts
    if (bWriteType == FRMWRT_SGL_R) {
        ReadFrameReq(bID, wAddr, bLen, bWriteType);
        memset(pData, 0, sizeof(pData));
        sciEnableNotification(sciREG, SCI_RX_INT);
        sciReceive(sciREG, bLen + 6, pData);
        while(UART_RX_RDY == 0U && count>0) count--; /* Wait */
        UART_RX_RDY = 0;
        bRes = bLen + 6;
    } else if (bWriteType == FRMWRT_STK_R) {
        bRes = ReadFrameReq(bID, wAddr, bLen, bWriteType);
        memset(pData, 0, sizeof(pData));
        sciEnableNotification(sciREG, SCI_RX_INT);
        sciReceive(sciREG, (bLen + 6) * (TOTALBOARDS - 1), pData);
        while(UART_RX_RDY == 0U && count>0) count--; /* Wait */
        UART_RX_RDY = 0;
        bRes = (bLen + 6) * (TOTALBOARDS - 1);
    } else if (bWriteType == FRMWRT_ALL_R) {
        bRes = ReadFrameReq(bID, wAddr, bLen, bWriteType);
        memset(pData, 0, sizeof(pData));
        sciEnableNotification(sciREG, SCI_RX_INT);
        sciReceive(sciREG, (bLen + 6) * TOTALBOARDS, pData);
        while(UART_RX_RDY == 0U && count>0) count--; /* Wait */
        UART_RX_RDY = 0;
        bRes = (bLen + 6) * TOTALBOARDS;
    } else {
        bRes = 0;
    }

//    //CHECK IF CRC IS CORRECT
//    for(crc_i=0; crc_i<bRes; crc_i+=(bLen+6))
//    {
//        if(CRC16(&pData[crc_i], bLen+6)!=0)
//        {
//            printf("BAD CRC\n");
//        }
//    }

    return bRes;
}

int ReadFrameReq(BYTE bID, uint16 wAddr, BYTE bByteToReturn, BYTE bWriteType) {
	bReturn = bByteToReturn - 1;

	if (bReturn > 127)
		return 0;

	return WriteFrame(bID, wAddr, &bReturn, 1, bWriteType);
}

// CRC16 TABLE
// ITU_T polynomial: x^16 + x^15 + x^2 + 1
const uint16 crc16_table[256] = { 0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301,
		0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1,
		0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81,
		0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
		0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00,
		0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1,
		0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380,
		0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0, 0x3180, 0xF141,
		0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501,
		0x35C0, 0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0,
		0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881,
		0x3840, 0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
		0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401,
		0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1,
		0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 0xA001, 0x60C0, 0x6180,
		0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740,
		0xA501, 0x65C0, 0x6480, 0xA441, 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01,
		0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1,
		0xA881, 0x6840, 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80,
		0xBA41, 0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
		0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200,
		0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1,
		0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780,
		0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0, 0x5D80, 0x9D41,
		0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901,
		0x59C0, 0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1,
		0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80,
		0x8C41, 0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
		0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040 };

uint16 CRC16(BYTE *pBuf, int nLen) {
    uint16 wCRC = 0xFFFF;
    int i;

    for (i = 0; i < nLen; i++) {
        wCRC ^= (*pBuf++) & 0x00FF;
        wCRC = crc16_table[wCRC & 0x00FF] ^ (wCRC >> 8);
    }

    return wCRC;
}
//****************************
//END WRITE AND READ FUNCTIONS
//****************************

//************************
//MISCELLANEOUS FUNCTIONS
//************************

BOOL GetFaultStat() {

    if (!gioGetBit(gioPORTA, 0))
        return 0;
    return 1;
}

void delayus(uint16 us) {
    if (us == 0)
       return;
    else
    {
        //CHANGE THE INTERRUPT COMPARE VALUES (PERIOD OF INTERRUPT)
        //Setup compare 0 value.
        rtiREG1->CMP[0U].COMPx = 10*us; //10 ticks of clock per microsecond, so multiply by 10
        //Setup update compare 0 value.
        rtiREG1->CMP[0U].UDCPx = 10*us;

        //ENABLE THE NOTIFICATION FOR THE PERIOD WE SET
        rtiEnableNotification(rtiNOTIFICATION_COMPARE0);

        //START THE COUNTER
        rtiStartCounter(rtiCOUNTER_BLOCK0);

        //WAIT IN LOOP UNTIL THE INTERRUPT HAPPENS (HAPPENS AFTER THE PERIOD WE SET)
        //WHEN INTERRUPT HAPPENS, RTI_NOTIFICATION GETS SET TO 1 IN THAT INTERRUPT
        //GO TO notification.c -> rtiNotification() to see where RTI_TIMEOUT is set to 1
        while(RTI_TIMEOUT==0);

        //RESET THE VARIABLE TO 0, FOR THE NEXT TIME WE DO A DELAY
        RTI_TIMEOUT = 0;

        //DISABLE THE INTERRUPT NOTIFICATION
        rtiDisableNotification(rtiNOTIFICATION_COMPARE0);

        //STOP THE COUNTER
        rtiStopCounter(rtiCOUNTER_BLOCK0);

        //RESET COUNTER FOR THE NEXT TIME WE DO A DELAY
        rtiResetCounter(rtiCOUNTER_BLOCK0);
    }
}

void delayms(uint16 ms) {
    if (ms == 0)
       return;
    else
    {
        rtiREG1->CMP[0U].COMPx = 10000*ms;
        rtiREG1->CMP[0U].UDCPx = 10000*ms;
        rtiEnableNotification(rtiNOTIFICATION_COMPARE0);
        rtiStartCounter(rtiCOUNTER_BLOCK0);
        while(RTI_TIMEOUT==0);
        RTI_TIMEOUT = 0;
        rtiDisableNotification(rtiNOTIFICATION_COMPARE0);
        rtiStopCounter(rtiCOUNTER_BLOCK0);
        rtiResetCounter(rtiCOUNTER_BLOCK0);
    }
}

void ResetAllFaults(BYTE bID, BYTE bWriteType)
{
    //BROADCAST INCLUDES EXTRA FUNCTIONALITY TO OVERWRITE THE CUST_CRC WITH THE CURRENT SETTINGS
    if(bWriteType==FRMWRT_ALL_W)
    {
        //READ THE CALCULATED CUSTOMER CRC VALUES
        ReadReg(0, CUST_CRC_RSLT_HI, fault_frame, 2, 0, FRMWRT_ALL_R);
        //OVERWRITE THE CRC OF EVERY BOARD IN THE STACK WITH THE CORRECT CRC
        for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
        {
            //THE RETURN FRAME STARTS WITH THE HIGHEST BOARD FIRST, SO THIS WILL WRITE THE HIGHEST BOARD FIRST
            WriteReg(TOTALBOARDS-currentBoard-1, CUST_CRC_HI, fault_frame[currentBoard*8+4] << 8 | fault_frame[currentBoard*8+5], 2, FRMWRT_SGL_W);
        }
        //NOW CLEAR EVERY FAULT
        WriteReg(0, FAULT_RST1, 0xFFFF, 2, FRMWRT_ALL_W);
    }
    else if(bWriteType==FRMWRT_SGL_W)
    {
        WriteReg(bID, FAULT_RST1, 0xFFFF, 2, FRMWRT_SGL_W);
    }
    else if(bWriteType==FRMWRT_STK_W)
    {
        WriteReg(0, FAULT_RST1, 0xFFFF, 2, FRMWRT_STK_W);
    }
    else
    {
        printf("ERROR: ResetAllFaults bWriteType incorrect\n");
    }
}

void MaskAllFaults(BYTE bID, BYTE bWriteType)
{
    if(bWriteType==FRMWRT_ALL_W)
    {
        WriteReg(0, FAULT_MSK1, 0xFFFF, 2, FRMWRT_ALL_W);
    }
    else if(bWriteType==FRMWRT_SGL_W)
    {
        WriteReg(bID, FAULT_MSK1, 0xFFFF, 2, FRMWRT_SGL_W);
    }
    else if(bWriteType==FRMWRT_STK_W)
    {
        WriteReg(0, FAULT_MSK1, 0xFFFF, 2, FRMWRT_STK_W);
    }
    else
    {
        printf("ERROR: MaskAllFaults bWriteType incorrect\n");
    }
}

void PrintAllFaults(BYTE bID, BYTE bWriteType)
{
    //PRINT 39 REGISTERS STARTING FROM FAULT_SUMMARY (INCLUDES RESERVED REGISTERS)
    printf("\n");
    currentBoard = 0;
    currentCell = 0;
    memset(fault_frame,0,sizeof(fault_frame));
    if(bWriteType==FRMWRT_ALL_R)
    {
        ReadReg(0, FAULT_SUMMARY, fault_frame, 39, 0, FRMWRT_ALL_R);
        for(currentBoard = 0; currentBoard<TOTALBOARDS; currentBoard++)
        {
            printf("BOARD %d FAULTS:\t",TOTALBOARDS-currentBoard);
            for(currentCell = 0; currentCell<39; currentCell++)
            {
                printf("%02x ",fault_frame[(currentBoard*(39+6))+4+currentCell]);
            }
            printf("\n");
        }
    }
    else if(bWriteType==FRMWRT_SGL_R)
    {
        ReadReg(bID, FAULT_SUMMARY, fault_frame, 39, 0, FRMWRT_SGL_R);
        printf("BOARD %d FAULTS:\t",bID);
        for(currentCell = 0; currentCell<39; currentCell++)
        {
            printf("%02x ",fault_frame[4+currentCell]);
        }
        printf("\n");
    }
    else if(bWriteType==FRMWRT_STK_R)
    {
        ReadReg(0, FAULT_SUMMARY, fault_frame, 39, 0, FRMWRT_STK_R);
        for(currentBoard = 0; currentBoard<(TOTALBOARDS-1); currentBoard++)
        {
            printf("BOARD %d FAULTS:\t",TOTALBOARDS-currentBoard);
            for(currentCell = 0; currentCell<39; currentCell++)
            {
                printf("%02x ",fault_frame[(currentBoard*(39+6))+4+currentCell]);
            }
            printf("\n");
        }
    }
    else
    {
        printf("ERROR: PrintAllFaults bWriteType incorrect\n");
    }
    printf("\n");
}

//RUN BASIC CELL BALANCING FOR ALL DEVICES
void RunCB()
{
    //SET BALANCING TIMERS TO 30 s
    WriteReg(0, CB_CELL16_CTRL, 0x0202020202020202, 8, FRMWRT_ALL_W);   //cell 16-9 (8 byte max write)
    WriteReg(0, CB_CELL8_CTRL, 0x0202020202020202, 8, FRMWRT_ALL_W);    //cell 8-1

    //SET DUTY CYCLE TO 10 s (default)
    WriteReg(0, BAL_CTRL1, 0x01, 1, FRMWRT_ALL_W);   //10s duty cycle

    //OPTIONAL: SET VCBDONE THRESH TO 3V, AND OVUV_GO
    WriteReg(0, VCB_DONE_THRESH, 0x08, 1, FRMWRT_ALL_W);    //3V threshold (8*25mV + 2.8V)
    WriteReg(0, OVUV_CTRL, 0x05, 1, FRMWRT_ALL_W);          //round-robin and OVUV_GO

    //START BALANCING
    WriteReg(0, BAL_CTRL2, 0x03, 1, FRMWRT_ALL_W); //auto balance and BAL_GO
}

//RUN BASIC REVERSE ADDRESSING SEQUENCE
void ReverseAddressing()
{
    //CHANGE BASE DEVICE DIRECTION
    WriteReg(0, CONTROL1, 0x80, 1, FRMWRT_SGL_W);

    //CHANGE REST OF STACK DIRECTION
    WriteReg(0, CONTROL1, 0x80, 1, FRMWRT_REV_ALL_W);

    //DO NORMAL AUTO ADDRESS SEQUENCE, BUT FOR DIR1_ADDR
    //DUMMY WRITE TO SNCHRONIZE ALL DAISY CHAIN DEVICES DLL (IF A DEVICE RESET OCCURED PRIOR TO THIS)
    WriteReg(0, OTP_ECC_DATAIN1, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN2, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN3, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN4, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN5, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN6, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN7, 0X00, 1, FRMWRT_ALL_W);
    WriteReg(0, OTP_ECC_DATAIN8, 0X00, 1, FRMWRT_ALL_W);

    //ENABLE AUTO ADDRESSING MODE, WHILE KEEPING REVERSE DIRECTION
    WriteReg(0, CONTROL1, 0X81, 1, FRMWRT_ALL_W);

    //SET ADDRESSES FOR EVERY BOARD (REVERSE DIRECTION)
    for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
    {
        WriteReg(0, DIR1_ADDR, currentBoard, 1, FRMWRT_ALL_W);
    }

    WriteReg(0, COMM_CTRL, 0x02, 1, FRMWRT_ALL_W); //set everything as a stack device first

    if(TOTALBOARDS==1) //if there's only 1 board, it's the base AND top of stack, so change it to those
    {
        WriteReg(0, COMM_CTRL, 0x01, 1, FRMWRT_SGL_W);
    }
    else //otherwise set the base and top of stack individually
    {
        WriteReg(0, COMM_CTRL, 0x00, 1, FRMWRT_SGL_W);
        WriteReg(TOTALBOARDS-1, COMM_CTRL, 0x03, 1, FRMWRT_SGL_W);
    }

    //SYNCRHONIZE THE DLL WITH A THROW-AWAY READ
    ReadReg(0, OTP_ECC_DATAIN1, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN2, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN3, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN4, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN5, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN6, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN7, response_frame2, 1, 0, FRMWRT_ALL_R);
    ReadReg(0, OTP_ECC_DATAIN8, response_frame2, 1, 0, FRMWRT_ALL_R);

    ////OPTIONAL: read back all device addresses
    //for(currentBoard=0; currentBoard<TOTALBOARDS; currentBoard++)
    //{
    //    ReadReg(currentBoard, DIR0_ADDR, response_frame2, 1, 0, FRMWRT_SGL_R);
    //    printf("board %d\n",response_frame2[4]);
    //}

    //RESET ANY COMM FAULT CONDITIONS FROM STARTUP
    WriteReg(0, FAULT_RST2, 0x03, 1, FRMWRT_ALL_W);
}
//***************************
//END MISCELLANEOUS FUNCTIONS
//***************************

//EOF

