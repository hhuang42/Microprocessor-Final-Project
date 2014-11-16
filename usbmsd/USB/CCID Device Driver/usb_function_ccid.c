/******************************************************************************
  File Information:
          FileName:        usb_function_ccid.c
          Dependencies:    See INCLUDES section below
          Processor:       PIC18, PIC24, or PIC32
          Compiler:        C18, C30, or C32
          Company:         Microchip Technology, Inc.
    
          Software License Agreement
    
          The software supplied herewith by Microchip Technology Incorporated
          (the “Company”) for its PICmicro® Microcontroller is intended and
          supplied to you, the Company’s customer, for use solely and
          exclusively on Microchip PICmicro Microcontroller products. The
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
    
  Summary:
    This file contains functions, macros, definitions, variables,
    datatypes, etc. that are required for use of CCID class function
    drivers. This file should be included in projects that use CCID class
    \function drivers. 
    
    
    
    This file is located in the "\<Install Directory\>\\Microchip\\USB\\CCID Device Driver" directory.
  
    Description:
    USB CCID Class Driver Header File
    
    This file contains functions, macros, definitions, variables,
    datatypes, etc. that are required for use of CCID class function
    drivers. This file should be included in projects that use CCID class
    \function drivers.
    
    This file is located in the "\<Install Directory\>\\Microchip\\USB\\CCID
    Device Driver" directory.
    
    When including this file in a new project, this file can either be
    referenced from the directory in which it was installed or copied
    directly into the user application folder. If the first method is
    chosen to keep the file located in the folder in which it is installed
    then include paths need to be added so that the library and the
    application both know where to reference each others files. If the
    application folder is located in the same folder as the Microchip
    folder (like the current demo folders), then the following include
    paths need to be added to the application's project:
    
    .
    
    ..\\..\\Microchip\\Include
    
    If a different directory structure is used, modify the paths as
    required. An example using absolute paths instead of relative paths
    would be the following:
    
    C:\\Microchip Solutions\\Microchip\\Include
    
    C:\\Microchip Solutions\\My Demo Application                               
  ******************************************************************************/

/********************************************************************
 Change History:
  Rev    Description
  ----   -----------
  2.7    Initial revision

********************************************************************/

/** I N C L U D E S **********************************************************/
#include "USB\usb.h"
#include "USB\usb_function_ccid.h"
#if defined(USB_USE_CCID)

#if defined USB_CCID_SUPPORT_ABORT_REQUEST
    void USB_CCID_ABORT_REQUEST_HANDLER(void);
#endif 

#if defined USB_CCID_SUPPORT_GET_CLOCK_FREQUENCIES_REQUEST
    void USB_CCID_GET_CLOCK_FREQUENCIES_REQUEST_HANDLER(void);
#endif 

#if defined USB_CCID_SUPPORT_GET_DATA_RATES_REQUEST
    void USB_CCID_GET_DATA_RATES_REQUEST_HANDLER(void);
#endif 

/** V A R I A B L E S ********************************************************/

/** P R I V A T E  P R O T O T Y P E S ***************************************/

/** D E C L A R A T I O N S **************************************************/

/** C L A S S  S P E C I F I C  R E Q ****************************************/
/******************************************************************************
 	Function:
 		void USBCheckCCIDRequest(void)
 
 	Description:
 		This routine checks the setup data packet to see if it
 		knows how to handle it
 		
 	PreCondition:
 		None

	Parameters:
		None
		
	Return Values:
		None
		
	Remarks:
		None
		 
  *****************************************************************************/
  void USBCheckCCIDRequest(void)    
  {
      /*
       * If request recipient is not an interface then return
       */
      if(SetupPkt.Recipient != USB_SETUP_RECIPIENT_INTERFACE_BITFIELD) return;
      /*
       * If request type is not class-specific then return
       */
      if(SetupPkt.RequestType != USB_SETUP_TYPE_CLASS_BITFIELD) return;
      /*
       * Interface ID must match interface number associated with
       * CCID class, else return
       */
      if(SetupPkt.bIntfID != CCID_INTERFACE_ID)
          return;
      switch (SetupPkt.bRequest)// checking for the request ID
      {
          #if defined (USB_CCID_SUPPORT_ABORT_REQUEST)
          case ABORT:
              USB_CCID_ABORT_REQUEST_HANDLER();
              break;
          #endif 
          
          #if defined (USB_CCID_SUPPORT_GET_CLOCK_FREQUENCIES_REQUEST)
          case GET_CLOCK_FREQUENCIES:
              USB_CCID_GET_CLOCK_FREQUENCIES_REQUEST_HANDLER();
              break;
          #endif 
          
          #if defined (USB_CCID_SUPPORT_GET_DATA_RATES_REQUEST)
          case GET_DATA_RATES:
              USB_CCID_GET_DATA_RATES_REQUEST_HANDLER();
              break;
          #endif 
          default:
              break;
      }//end switch(SetupPkt.bRequest)
}//end USBCheckCCIDRequest

#endif //def USB_USE_CCID

/** EOF usb_function_ccid.c *************************************************************/
