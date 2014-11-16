/********************************************************************
 FileName:		SClib.h
 Dependencies:	See INCLUDES section
 Processor:		PIC18, PIC24 Microcontrollers
 Hardware:		This demo is natively intended to be used on Exp 16, LPC
 				& HPC Exp board. This demo can be modified for use on other hardware
 				platforms.
 Complier:  	Microchip C18 (for PIC18), C30 (for PIC24)
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

#ifndef __SC_MCP_LIB__
#define __SC_MCP_LIB__

#include "GenericTypeDefs.h"

typedef enum
{
	SC_ERR_NONE,	// No Error
	SC_ERR_CARD_NOT_SUPPORTED,	// Card Not Supported
	SC_ERR_BAR_OR_NO_ATR_RESPONSE,	// No ATR Response from the card
	SC_ERR_RSV1,	// Smart Card Error 1 (Reserved) - can be used based upon the Application 
	SC_ERR_RSV2,	// Smart Card Error 2 (Reserved) - can be used based upon the Application
	SC_ERR_RSV3,	// Smart Card Error 3 (Reserved) - can be used based upon the Application
	SC_ERR_RSV4,	// Smart Card Error 4 (Reserved) - can be used based upon the Application
	SC_ERR_RSV5,	// Smart Card Error 5 (Reserved) - can be used based upon the Application
	SC_ERR_RSV6,	// Smart Card Error 6 (Reserved) - can be used based upon the Application
	SC_ERR_RSV7		// Smart Card Error 7 (Reserved) - can be used based upon the Application
} SC_ERROR;

// Smart Card Commands

// Start Session Command code to the Smart Card
#define SC_START_SESSION	0x84

// Authenticate Command code to the Smart Card
#define SC_AUTHENTICATE		0x82

// Get Response Command code to the Smart Card
#define SC_GET_RESPONSE		0xC0

// Submit Code Command to the Smart Card
#define SC_SUBMIT_CODE		0x20

// Clear Card Command code to the Smart Card
#define SC_CLEAR_CARD		0x30

// Select File Command code to the Smart Card
#define SC_SELECT_FILE		0xA4

// Read Record Command code to the Smart Card
#define SC_READ_RECORD		0xB2

// Write Record Command code to the Smart Card
#define SC_WRITE_RECORD		0xD2

// Credit Command code to the Smart Card
#define SC_CREDIT			0xE2

// Debit Command code to the Smart Card
#define SC_DEBIT			0xE6

// Revoke Command code to the Smart Card
#define SC_REVOKE			0xE8

// Inquire Account Command code to the Smart Card
#define SC_INQUIRE_ACCT		0xE4

// Change Pin Command code to the Smart Card
#define SC_CHANGE_PIN		0x24

// ATR data sent by smartcard.
extern BYTE SC_CardATR[];

// length of ATR data sent by smart card
extern WORD SC_ATRLen;

// Smart Card Error type is stored in this variable
extern SC_ERROR SC_LastError;

// TA1 determines the clock-rate conversion factor F & bit-rate-adjustment factor D
extern BYTE SC_TA1;

// TA2 determines whether the smart card will operate in specific mode or negotiable mode
// following the ATR
extern BYTE SC_TA2;

// TA3 conveys the Information Field Size Integer (IFSI) for the smart card.
extern BYTE SC_TA3;

// TB1 conveys information on the smart card's programming voltage requirements.
extern BYTE SC_TB1;

// TB2 conveys PI2, which determines the value of programming voltage required by
// the smart card. The value of PI1 in TB1 is superceded when TB2 is present
extern BYTE SC_TB2;

// TB3 indicates the value of the Character Waiting Time Integer (CWI) and 
// Block Waiting Time Integer (BWI) used to compute the Character Waiting Time (CWT)
// and Block Waiting Time (BWT).
extern BYTE SC_TB3;

// TC1 determines the extra guard time to be added between consecutive characters
// sent to the smart card from the terminal.
extern BYTE SC_TC1;

// TC2 is specific to protocol type T=0. TC2 conveys work waiting-time integer (WI)
// that determines the maximum interval between the leading edge of the start bit
// of any character sent by the smart card and the leading edge of the start bit
// of the previous character sent either by the card or the reader
extern BYTE SC_TC2;

// When TC3 is present, it indicates the type of block-error detection to be used.
// When TC3 is not present, the default longitudinal redundancy check (LRC) is used.
extern BYTE SC_TC3;

// TD1 indicates if any further interface bytes are to be transmitted, and if so,
// which protocol will be used.
extern BYTE SC_TD1;

// The TD2 character has the same function as the TD1 character.
extern BYTE SC_TD2;

// TD3 indicates interface bytes similar to that of TD1 & TD2
extern BYTE SC_TD3;

// Historical bytes sent by Smart Card
extern BYTE* SC_ATR_HistBfr;

// Number of Historical bytes present
extern BYTE  SC_ATR_HistLen;

////////// SmartCard APDU Command 7816-4
typedef struct
{
  BYTE CLA;  			// Command class
  BYTE INS;  			// Operation code 
  BYTE P1;   			// Selection Mode 
  BYTE P2;   			// Selection Option 
  BYTE LC;				// Data length
  BYTE LE;         		// Expected length of data to be returned
} SC_APDU_Cmd;

/////////// SmartCard APDU Response structure 7816-4
typedef struct
{
  BYTE SW1;          	// Trailer Response status 1
  BYTE SW2;          	// Trailer Response status 2
} SC_APDU_Resp;


// SmartCard States
#define	SC_STATE_CARD_NOT_PRESENT	10	// No Card Detected
#define	SC_STATE_CARD_ACTIVE		20	// Card is powered and ATR received
#define	SC_STATE_CARD_INACTIVE		30	// Card present but not powered


// Smart Card Lib API //////////////////

/*******************************************************************************
  Function:
    void SC_Initialize(void)
	
  Description:
    This function initializes the smart card library

  Precondition:
    None

  Parameters:
    None

  Return Values:
    None
	
  Remarks:
    None
  *****************************************************************************/
void SC_Initialize(void);


/*******************************************************************************
  Function:
	BOOL SC_CardPresent(void)	
  
  Description:
    This macro checks if card is inserted in the socket

  Precondition:
    SC_Initialize() is called

  Parameters:
    None

  Return Values:
    TRUE if Card is inserted, otherwise return FALSE
	
  Remarks:
    None
  *****************************************************************************/
BOOL SC_CardPresent(void);


/*******************************************************************************
  Function:
    int SC_GetCardState(void)
	
  Description:
    This function returns the current state of SmartCard

  Precondition:
    SC_Initialize is called.

  Parameters:
    None

  Return Values:
    SC_STATE_CARD_NOT_PRESENT:		No Card Detected
    SC_STATE_CARD_ACTIVE:			Card is powered and ATR received
    SC_STATE_CARD_INACTIVE:			Card present but not powered
	
  Remarks:
    None
  *****************************************************************************/
int SC_GetCardState(void);

/*******************************************************************************
  Function:
	BOOL SC_PowerOnATR(void)	
  
  Description:
    This function performs the power on sequence of the SmartCard and 
	interprets the Answer-to-Reset data received from the card.

  Precondition:
    SC_Initialize() is called, and card is present

  Parameters:
    None

  Return Values:
    TRUE if Answer to Reset (ATR) was successfuly received and processed
	
  Remarks:
    None
  *****************************************************************************/
BOOL SC_PowerOnATR(void);

/*******************************************************************************
  Function:
	BOOL SC_DoPPS(void)	
  
  Description:
    This function configures the baud rate of the PIC uart to match with
	Answer-to-Reset data.

  Precondition:
    SC_PowerOnATR was success

  Parameters:
    None

  Return Values:
    TRUE if Baud rate is supported by the PIC
	
  Remarks:
    This function is called when SC_PowerOnATR() returns TRUE.  If the Baud 
	rate configration file inside the card is changed, these function should 
	be called again for the new baud to take effect.
  *****************************************************************************/
BOOL SC_DoPPS(void);


/*******************************************************************************
  Function:
	BOOL SC_Transact(SC_APDU_Cmd* pAPDU, SC_APDU_Resp* pAPDUResp, BYTE* pAPDUData)	
  
  Description:
    This function Sends the ISO 7816-4 compaliant APDU commands to the card.  
	It also receive the expected response from the card as defined by the 
	command data.

  Precondition:
    SC_DoPPS was success

  Parameters:
    SC_APDU_Cmd* pAPDU	- Pointer to APDU Command Structure 
	SC_APDU_Resp* pResp - Pointer to APDU Response structure
			BYTE* pResp - Pointer to the Command/Response Data buffer

  Return Values:
    TRUE if transaction was success, and followed the ISO 7816-4 protocol. 
	
  Remarks:
    In the APDU command structure, the LC field defines the number of bytes to 
	transmit from the APDUdat array.  This array can hold max 256 bytes, which 
	can be redefined by the user.  The LE field in APDU command defines the number
	of bytes to receive from the card.  This array can hold max 256 bytes, which 
	can be redefined by the user.
	
  *****************************************************************************/
BOOL SC_Transact(SC_APDU_Cmd* pAPDU, SC_APDU_Resp* pAPDUResp, BYTE* pAPDUData);


/*******************************************************************************
  Function:
    void SC_Shutdown(void)
	
  Description:
    This function Performs the Power Down sequence of the SmartCard

  Precondition:
    SC_Initialize is called.

  Parameters:
    None

  Return Values:
    None
	
  Remarks:
    None
  *****************************************************************************/
void SC_Shutdown(void);

#endif



