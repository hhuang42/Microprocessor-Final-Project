/*    Mouse_demo.c
    Class-compliant USB mouse driver for PIC32
    A trimmed-down version of the "USB Host - HID Mouse" demo program
    from the Microchip Application Libraries, retreived June 2010 from
    the Microchip website.
    
*/
/*  Matthew Watkins
    Fall 2010
      Simplified 25 October 2011 David_Harris & Karl_Wang@hmc.edu
*/

/******************************************************************************

    USB Mouse Host Application Demo

Description:
    This file contains the basic USB Mouse application demo. Purpose of the demo
    is to demonstrate the capability of HID host . Any Low speed/Full Speed
    USB Mouse can be connected to the PICtail USB adapter along with 
    Explorer 16 demo board. This file schedules the HID ransfers, and interprets
    the report received from the mouse. X & Y axis coordinates, Left & Right Click
    received from the mouse are diaplayed on the the LCD display mounted on the
    Explorer 16 board. Demo gives a fair idea of the HID host and user should be
    able to incorporate necessary changes for the required application.

* File Name:       Mouse_demo.c
* Dependencies:    None
* Processor:       PIC24FJ256GB110
* Compiler:        C30 v2.01
* Company:         Microchip Technology, Inc.

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

*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"
#include "usb_config.h"
#include "USB\usb.h"
#include "USB/usb_host_msd.h"
#include "USB/usb_host_msd_scsi.h"
#include "MDD File System/FSIO.h"
#include <plib.h>
#include <P32xxxx.h>




// *****************************************************************************
// *****************************************************************************
// Macros
// *****************************************************************************
// *****************************************************************************
#define MAX_ALLOWED_CURRENT             (500)         // Maximum power we can supply in mA
#define MINIMUM_POLL_INTERVAL           (0x0A)        // Minimum Polling rate for HID reports is 10ms

#define USAGE_PAGE_BUTTONS              (0x09)

#define USAGE_PAGE_GEN_DESKTOP          (0x01)


#define MAX_ERROR_COUNTER               (10)

// Use SPI Channel 2
#define SPI_CHANNEL                     (2)

// Divide source clock by 2
#define SRC_CLK_DIV                     (2)



// *****************************************************************************
// *****************************************************************************
// Global Variables
// *****************************************************************************
// *****************************************************************************

FSFILE * myFile;
FSFILE * myFile2;
BYTE myData[512];
size_t numBytes;
volatile BOOL deviceAttached;
//******************************************************************************
//******************************************************************************
// Main
//******************************************************************************
//******************************************************************************
typedef struct riff_header
{
    char chunk_id[4];
    unsigned int chunk_size;
    char format[4];
    char subchunk1_id[4];
    unsigned int subchunk1_size;
    unsigned short int audio_format;
    unsigned short int num_channels;
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short int block_align;
    unsigned short int bits_per_sample;
    char subchunk2_id[4];
    unsigned int subchunk2_size;

} header;

short whendoublechannel(myFile, myFile2){
		FSfread(myData, 1, 4, myFile);
		short* samples = myData;
		return samples[0];
	}

int main (void)
{
        BYTE i;
        int  value;
    	int j = 0;
        value = SYSTEMConfigWaitStatesAndPB( GetSystemClock() );
    
        // Enable the cache for the best performance
        CheKseg0CacheOn();
    
        INTEnableSystemMultiVectoredInt();
            
        TRISF = 0xFFFF;
        TRISE = 0xFFFF;
        TRISB = 0xFFFF;
        TRISG = 0xFFFF;
        
        // Initialize USB layers
        USBInitialize( 0 );

        // Initialize the SPI Channel 2 to be a master, reverse
        // clock, have 32 bits, enabled frame mode, and
        SpiChnOpen(SPI_CHANNEL, 
                   SPI_OPEN_MSTEN|SPI_OPEN_CKE_REV|
                   SPI_OPEN_MODE32|SPI_OPEN_FRMEN, SRC_CLK_DIV);
        while(1)
        {
            USBTasks();
            //if thumbdrive is plugged in
            if(USBHostMSDSCSIMediaDetect())
            {
                deviceAttached = TRUE;

                //now a device is attached
                //See if the device is attached and in the right format
                if(FSInit())
                {
                    //Opening a file in mode "w" will create the file if it doesn't
                    //  exist.  If the file does exist it will delete the old file
                    //  and create a new one that is blank.
                    myFile = FSfopen("440.wav","r");
                    myFile2 = FSfopen("test.txt", "w");

                    //Read the data form testread.txt (myFile) and put into array myData
                    FSfread(myData, 1, 44, myFile);

                    header *rheader_ptr = &myData;
                    header my_header = *rheader_ptr;
					
					FSfwrite(myData, 1, 4, myFile2);
                    itoa(myData, my_header.chunk_size, 10);
                    FSfwrite(myData, 1, strlen(myData), myFile2);
					itoa(myData, my_header.num_channels, 10);
					FSfwrite(myData, 1, strlen(myData), myFile2);
					itoa(myData,my_header.bits_per_sample, 10);
					FSfwrite(myData, 1, strlen(myData), myFile2);

					if(my_header.num_channels ==1)
						{ sprintf(myData, "This wave file is a mono type");
						//FSfread(myData, 1, 2, myFile);
						//FSfwrite(myData, 1, 2, myFile2); 
						}
					else if (my_header.num_channels ==2)
						{	while (j<10){
							short sample = whendoublechannel(myFile, myFile2);
							myData[0] = sample;
							FSfwrite(myData, 1, 2, myFile2);
							j++;
						}
						}
					else { sprintf(myData, "Cannot identify the wave file type");
						 }
					
					//strlen(my_header.chunk_size)-44
					//sprintf(myData, "\nTest: %d \n",my_header.num_channels);

					//FSfwrite(myData, 1, strlen(myData), myFile2);
				
					//FSfread (myData, 1, 50, myFile);
					//FSfwrite(myData, 1, 50, myFile2);

                    //should come up with a way to store the things in myData to somewhere else.
                    //before reusing myData....
                    //FSfread(myData, 1, chunk_size-44, myFile2);
                    //FSfwrite(myData, 1, 4, myFile2);

                    //Always make sure to close the file so that the data gets
                    //  written to the drive.
                    FSfclose(myFile);
                    FSfclose(myFile2);

                    //Just sit here until the device is removed.
                    while(deviceAttached == TRUE)
                    {
                        USBTasks();
                    }
                }
            }
        }
}

//******************************************************************************
//******************************************************************************
// USB Support Functions
//******************************************************************************
//******************************************************************************

BOOL USB_ApplicationEventHandler( BYTE address, USB_EVENT event, void *data, DWORD size )
{
    switch( event )
    {
        case EVENT_VBUS_REQUEST_POWER:
            // The data pointer points to a byte that represents the amount of power
            // requested in mA, divided by two.  If the device wants too much power,
            // we reject it.
            if (((USB_VBUS_POWER_EVENT_DATA*)data)->current <= (MAX_ALLOWED_CURRENT / 2))
            {
                return TRUE;
            }
            else
            {
				return FALSE;
              //  UART2PrintString( "\r\n***** USB Error - device requires too much current *****\r\n" );
            }
            break;

        case EVENT_VBUS_RELEASE_POWER:
            // Turn off Vbus power.
            // The PIC24F with the Explorer 16 cannot turn off Vbus through software.

            //This means that the device was removed
            deviceAttached = FALSE;
            return TRUE;
            break;

        case EVENT_HUB_ATTACH:
        //    UART2PrintString( "\r\n***** USB Error - hubs are not supported *****\r\n" );
            return TRUE;
            break;

        case EVENT_UNSUPPORTED_DEVICE:
        //    UART2PrintString( "\r\n***** USB Error - device is not supported *****\r\n" );
            return TRUE;
            break;

        case EVENT_CANNOT_ENUMERATE:
        //   UART2PrintString( "\r\n***** USB Error - cannot enumerate device *****\r\n" );
            return TRUE;
            break;

        case EVENT_CLIENT_INIT_ERROR:
        //    UART2PrintString( "\r\n***** USB Error - client driver initialization error *****\r\n" );
            return TRUE;
            break;

        case EVENT_OUT_OF_MEMORY:
        //    UART2PrintString( "\r\n***** USB Error - out of heap memory *****\r\n" );
            return TRUE;
            break;

        case EVENT_UNSPECIFIED_ERROR:   // This should never be generated.
        //    UART2PrintString( "\r\n***** USB Error - unspecified *****\r\n" );
            return TRUE;
            break;

        default:
            break;
    }
    return FALSE;
}

