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
(the ìCompanyÅE for its PICmicroÆ Microcontroller is intended and
supplied to you, the Companyís customer, for use solely and
exclusively on Microchip PICmicro Microcontroller products. The
software is owned by the Company and/or its supplier, and is
protected under applicable copyright laws. All rights are reserved.
Any use in violation of the foregoing restrictions may subject the
user to criminal sanctions under applicable laws, as well as to
civil liability for the breach of the terms and conditions of this
license.

THIS SOFTWARE IS PROVIDED IN AN ìAS ISÅECONDITION. NO WARRANTIES,
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

#define CLOCK_RATE (40000000)

#define SOUND_RATE (44100)

#define SAMPLE_LENGTH (169)

#define MIN_SAMPLE_VALUE (-32768)

#define MAX_SAMPLE_VALUE (32767)

#define PWM_PERIOD (CLOCK_RATE/SOUND_RATE)

#define BUFFER_SIZE (512)


// *****************************************************************************
// *****************************************************************************
// Global Variables
// *****************************************************************************
// *****************************************************************************

FSFILE * myFile;
FSFILE * myFile2;
BYTE myData[BUFFER_SIZE];
volatile BYTE* myDataPtr = myData;
size_t numBytes;
volatile BOOL deviceAttached;
BYTE myData2[BUFFER_SIZE];
volatile BYTE* myData2Ptr = myData2;

BYTE buffer[BUFFER_SIZE];
volatile BYTE* bufferPtr = buffer;
volatile BOOL bufferFilled;
size_t bufferOffset;
BOOL sampleAvailable;
size_t remainingBytes;
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

short next_sample(void){
        short* samples = myDataPtr;
        short sample = samples[bufferOffset];
        bufferOffset = (bufferOffset + 1) % (BUFFER_SIZE/2);
        if (bufferOffset == 0){
            if (bufferFilled) {
                BYTE* temp = bufferPtr;
                bufferPtr = myDataPtr;
                myDataPtr = temp;
                bufferFilled = FALSE;
            } else {
                return 0;
            }
        }
        return sample;
    }
void pwm_setup(void){
    bufferOffset = 0;
    sampleAvailable = FALSE;
    bufferFilled = FALSE;
    myDataPtr = myData;
    bufferPtr = buffer;
	
        // set up the timer 2 interrupt with a prioirty of 2 and zero sub-priority
    INTSetVectorPriority(_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_2);
    INTSetVectorSubPriority(_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);

    // configure for multi-vectored mode
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    // enable interrupts
    INTEnableInterrupts();

    /* Open Timer2 with Period register value */
    OpenTimer2(T2_ON, CLOCK_RATE/SOUND_RATE);

    /* Enable OC | 32 bit Mode  | Timer2 is selected | Continuous O/P   | OC Pin High , S Compare value, Compare value*/
    
    OC2CON = 0x0000;
    OC2R = CLOCK_RATE/SOUND_RATE;
    OC2RS = CLOCK_RATE/SOUND_RATE;
    OC2CON = 0x0006;
    PR2 = CLOCK_RATE/SOUND_RATE;

    IFS0CLR = 0x00000100;
    IEC0SET = 0x00000100;
    IPC2SET = 0x0000001C;

    T2CONSET = 0x8000;
    OC2CONSET = 0x8000;
}


void read_octagondata(char* file_name)
{   
    int octagondata;
    myFile2 = FSfopen(file_name, "r");
    //Read the data from octagondata.txt (myFile) and put into array myData2

    FSfread(myData2, 1, myFile2.size, myFile2);

    char * pch;

    pch = strtok(myData2, " ");
    octagondata = atoi(pch);
    pch = strtok(NULL, " ");
    octagondata = atoi(pch) | (octagondata<<9)
    octagondata = (octagondata << 15)
    
    // send the first set of data over (beat + # of octagons)
    SpiChnPutC(SPI_CHANNEL, octagondata);
    
    // clear out the receiving side
    SpiChnGetC(SPI_CHANNEL);

        while (pch !=NULL)
    {       

        // before sending the data over, change the data from ascii to binary
            octagondata = atoi(pch);
            pch = strtok(NULL, " ");
            octagondata = atoi(pch) | (octagondata<<10);
            pch = strtok(NULL, " ");
            octagondata = atoi(pch) | (octagondata<<11);
            pch = strtok(NULL, " ");
            octagondata = atoi(pch) | (octagondata<<1);
        // send data over
            SpiChnPutC(SPI_CHANNEL, octagondata);

        // clear out the receiving side
            SpiChnGetC(SPI_CHANNEL);
            pch = strtok(NULL, " ");
    }

    FSfclose(myFile2);
    //not sure about this function....
   

    //send through the SPI to PIC2

    /* strtok example */
    /*#include <stdio.h>
    #include <string.h>

    int main ()
    {
    char str[] ="- This, a sample string.";
    char * pch;
    printf ("Splitting string \"%s\" into tokens:\n",str);
    pch = strtok (str," ,.-");
    while (pch != NULL)
    {
        printf ("%s\n",pch);
        pch = strtok (NULL, " ,.-");
     }
    return 0;
    }*/


}

void play_file(char* file_name){

    myFile = FSfopen(file_name,"r");

    //Read the data form testread.txt (myFile) and put into array myData
    FSfread(myData, 1, 44, myFile);

    header *rheader_ptr = &myData;
    header my_header = *rheader_ptr;
    
    pwm_setup();
    remainingBytes = my_header.subchunk2_size;
    FSfread(myData, 1, BUFFER_SIZE, myFile);
    remainingBytes -= BUFFER_SIZE;
    while (remainingBytes > 0){
        USBTasks();
        if (!bufferFilled){
            size_t read_size = BUFFER_SIZE < remainingBytes ? 
                               BUFFER_SIZE : remainingBytes;
            FSfread(bufferPtr, 1, read_size, myFile);
            bufferFilled = TRUE;
            sampleAvailable = TRUE;
            remainingBytes -= read_size;
        } 
    }
    sampleAvailable = FALSE;

    FSfclose(myFile);
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
                   SPI_OPEN_SLVEN|SPI_OPEN_CKE_REV|
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
                    SearchRec music_search;
                    while(FindFirst("Smoke.w*", ATTR_MASK, &music_search) == 0){
                        play_file(music_search.filename);
                        // while(1){
                        //     play_file(music_search.filename);
                        //     if(FindNext(&music_search) != 0){
                        //         FindFirst("*", ATTR_MASK, &music_search);
                        //     }
                        // }
                    }

                    
                    

                    //Just sit here until the device is removed.
                    while(deviceAttached == TRUE)
                    {
                        USBTasks();
                    }
                }
            }
        }
}

void __ISR(_TIMER_2_VECTOR, ipl7) T2_IntHandler(void)
{
    IFS0CLR = 0x0100;
    if (sampleAvailable){
        int sample = next_sample();
        int duty_cycle = (sample-(MIN_SAMPLE_VALUE)) * 
                          PWM_PERIOD/(MAX_SAMPLE_VALUE-MIN_SAMPLE_VALUE);
        SetDCOC2PWM(duty_cycle);
    } else {
        SetDCOC2PWM(0);
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

