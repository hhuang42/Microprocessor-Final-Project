/********************************************************************
 FileName:		SCpic24.h
 Dependencies:	See INCLUDES section
 Processor:		PIC24 Microcontrollers
 Hardware:		This demo is natively intended to be used on Exp 16 board.
 				This demo can be modified for use on other hardware platforms.
 Complier:  	Microchip C30 (for PIC24)
 Company:		Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the “Company”) for its PIC® Microcontroller is intended and
 supplied to you, the Company’s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Description
  ----  -----------------------------------------
  1.0   Initial release
********************************************************************/

#ifndef __SC_DRV24_LIB__
#define __SC_DRV24_LIB__

#include "GenericTypeDefs.h"
#include "p24Fxxxx.h"

extern BOOL GenTxError;         //For Testing Only: Force an Error on Transmit, and recover automatically using 7816-3 protocol
extern BOOL GenRxError;			//For Testing Only: Flag to create error on Data Receive, and recover automatically using 7816-3 protocol
extern BOOL SysErrorDetected;   //I/O data line is permanently stuck low
extern unsigned long long baudRate;	// Baud Rate of Smart Card Transmission/Reception Data

void SCdrv_InitUART(void);
void SCdrv_CloseUART(void);
void SCdrv_SetBRG( BYTE SpeedCode );
BOOL SCdrv_GetRxData( BYTE* pDat, unsigned long nTrys );
void SCdrv_SendTxData( BYTE dat );

#endif



