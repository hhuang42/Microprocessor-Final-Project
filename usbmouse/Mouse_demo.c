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
#include <math.h>
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"
#include "usb_config.h"
#include "USB\usb.h"
#include "USB\usb_host_hid_parser.h"
#include "USB\usb_host_hid.h"
#include <plib.h>
#include <P32xxxx.h>


// *****************************************************************************
// *****************************************************************************
// Data Structures
// *****************************************************************************
// *****************************************************************************

typedef enum _APP_STATE
{
    DEVICE_NOT_CONNECTED,
    DEVICE_CONNECTED, /* Device Enumerated  - Report Descriptor Parsed */
    READY_TO_TX_RX_REPORT,
    GET_INPUT_REPORT, /* perform operation on received report */
    INPUT_REPORT_PENDING,
    ERROR_REPORTED 
} APP_STATE;

typedef struct _HID_REPORT_BUFFER
{
    WORD  Report_ID;
    WORD  ReportSize;
//    BYTE* ReportData;
    BYTE  ReportData[4];
    WORD  ReportPollRate;
}   HID_REPORT_BUFFER;

// *****************************************************************************
// *****************************************************************************
// Internal Function Prototypes
// *****************************************************************************
// *****************************************************************************
BYTE App_DATA2ASCII(BYTE a);
void AppInitialize(void);
BOOL AppGetParsedReportDetails(void);
void App_Detect_Device(void);
void App_ProcessInputReport(void);
BOOL USB_HID_DataCollectionHandler(void);


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

// Use SPI Channel 2 for PIC2 to FPGA connection
#define SPI_CHANNEL                     (2)

// Use SPI Channel 1 for PIC1 to PIC2 connection
#define SPI_CHANNEL2                    (1)

// Divide source clock by 2
#define SRC_CLK_DIV                     (2)

#define SCALING                         (2)

#define CLOCK_RATE                      (40000000)

#define POSITION_MASK                   (0x3FF)
#define TIME_MASK                       (0xFF)
#define LIFE_MASK                       (0xFF)
#define RESULT_MASK                     (0x3)
#define SCORE_MASK                      (0x3FFFFF)
#define ALIVE_MASK                      (0x1)

#define MIN_X                           (0)
#define MAX_X                           (640)
#define MIN_Y                           (0)
#define MAX_Y                           (480)

#define TARGET_LIFE                     (60)
#define TARGET_RADIUS                   (30)

#define MIN_HEALTH                      (0)
#define MAX_HEALTH                      (255)



// *****************************************************************************
// *****************************************************************************
// Global Variables
// *****************************************************************************
// *****************************************************************************

APP_STATE App_State_Mouse = DEVICE_NOT_CONNECTED;

HID_DATA_DETAILS Appl_Mouse_Buttons_Details;
HID_DATA_DETAILS Appl_XY_Axis_Details;

HID_REPORT_BUFFER  Appl_raw_report_buffer;

HID_USER_DATA_SIZE Appl_Button_report_buffer[3];
HID_USER_DATA_SIZE Appl_XY_report_buffer[3];

BYTE ErrorDriver;
BYTE ErrorCounter;
BYTE NumOfBytesRcvd;

BOOL ReportBufferUpdated;
BOOL LED_Key_Pressed = FALSE;

BYTE currCharPos;
BYTE FirstKeyPressed ;

volatile BOOL send_timer;

// x position of mouse with SCALING:1 value to pixel ratio 
int x_position = 0;

// y position of mouse with SCALING:1 value to pixel ratio 
int y_position = 0;

// buffer taking care of the messages received from PIC1
unsigned int octagonbuffer;

// the beat, number of octagons and offset received from PIC1
int beat;
int numofoctagons;
int offset;

volatile int life;
volatile int timer;
volatile int score;
volatile int total_target_count;
volatile int next_target;
volatile int next_target_to_appear;
volatile BOOL last_button_pressed_state;
volatile BOOL is_game_active;
volatile BOOL is_game_over;
volatile BOOL is_roll_active;



typedef struct target_info
{
    unsigned short x_position;
    unsigned short y_position;
    unsigned int   play_time;
    BOOL           is_rolling;
} target;
volatile target* start_roll_target;
volatile target* end_roll_target;
volatile int roll_score_type;

target targets[512];

//******************************************************************************
//******************************************************************************
// Main
//******************************************************************************
//******************************************************************************

void receiveOctagondata()
{   
    int i;
    
    SpiChnPutC(SPI_CHANNEL2, 0xFFFFFFFF);
    octagonbuffer = SpiChnGetC(SPI_CHANNEL2);
    octagonbuffer = (octagonbuffer >> 7);
    offset = (octagonbuffer) & 0xFF;
    octagonbuffer = (octagonbuffer >> 8);
    numofoctagons = (octagonbuffer) & 0b111111111;
    octagonbuffer = (octagonbuffer >> 9);
    beat = octagonbuffer & 0b11111111;
    
    for ( i=0; i<numofoctagons; i++){
        SpiChnPutC(SPI_CHANNEL2, 0xFFFFFFFF);
        octagonbuffer = SpiChnGetC(SPI_CHANNEL2);
        targets[i].is_rolling = (octagonbuffer) & 0b1;
        octagonbuffer = (octagonbuffer >> 1);
        targets[i].play_time = (octagonbuffer) & 0b11111111111;
        octagonbuffer = (octagonbuffer >> 11);
        targets[i].y_position = (octagonbuffer) & 0b1111111111;
        octagonbuffer = (octagonbuffer >> 10);
        targets[i].x_position = octagonbuffer & 0b1111111111;
        };
        
    //SpiChnPutC(SPI_CHANNEL2, 0x00000000);
    //SpiChnGetC(SPI_CHANNEL2);
}        

    
    
    
     /*for(i=0; i<200; ++i){
        targets[i].x_position = ((i/8)*430-i*70+200) % MAX_X;
        targets[i].y_position = ((i/8)*350-i*90+300) % MAX_Y;
        targets[i].play_time = i*40 + 300;
        targets[i].is_rolling = i%10==0;
        ++total_target_count;*/
        
        
    
    
    

unsigned short mouse_x_position(void){
    return x_position / SCALING;
}

unsigned short mouse_y_position(void){
    return y_position / SCALING;
}


unsigned short set_mouse_x_position(int x){
    return x_position = x * SCALING;
}

unsigned short set_mouse_y_position(int y){
    return y_position = y * SCALING;
}

void addNewOctagon(unsigned short x, unsigned short y)
{
	int data;
	data = 0b111;
	data = (x & POSITION_MASK) | (data<<10);
	data = (y & POSITION_MASK) | (data<<10);
	data = (data<<9);

	// send data over
	SpiChnPutC(SPI_CHANNEL, data);

	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};
}

void addNewPathTo(unsigned short x, unsigned short y)
{
	int data;
	data = 0b10011;
	data = (x & POSITION_MASK) | (data<<10);
	data = (y & POSITION_MASK) | (data<<10);
	data = (data<<7);

	// send data over
	SpiChnPutC(SPI_CHANNEL, data);

	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};
}

void deletePath(void)
{
	int data;
	data = 0b10010;
	data = (data<<10);
	data = (data<<10);
	data = (data<<7);

	// send data over
	SpiChnPutC(SPI_CHANNEL, data);

	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};
}

void rollOctagon(unsigned short x, unsigned short y)
{
	int data;
	data = 0b1011;
	data = (x & POSITION_MASK) | (data<<10);
	data = (y & POSITION_MASK) | (data<<10);
	data = (data<<8);

	// send data over
	SpiChnPutC(SPI_CHANNEL, data);

	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};
}

void stopRollOctagon()
{
	int data;
	data = 0xA0000000;

	// send data over
	SpiChnPutC(SPI_CHANNEL, data);

	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};
}

void deleteOldestOctagon(unsigned short x, unsigned short y, unsigned char result)
{
	int data;
	data = 0b110;
	data = (x & POSITION_MASK) | (data<<10);
	data = (y & POSITION_MASK) | (data<<10);
	data = (result & RESULT_MASK) | (data<<2);
	data = (data<<7);

	// send data over
	SpiChnPutC(SPI_CHANNEL, data);

	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};
}

void sendTimeandMouse(unsigned char time, unsigned short x_position, unsigned short y_position)
{
	int data;
	data = 0b01;
	data = (time & TIME_MASK) | (data<<8);
	data = (x_position & POSITION_MASK)  | (data<<10);
	data = (y_position & POSITION_MASK)  | (data<<10);
	data = (data<<2);

	// send data over
	SpiChnPutC(SPI_CHANNEL, data);

	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};
}

void sendLifeAndScore( unsigned char life, unsigned int score)
{
	unsigned int data;
	data = 0b00;
	data = (life & LIFE_MASK) | (data<<8);
	data = (score & SCORE_MASK) | (data<<22);
	
	// send data over
	SpiChnPutC(SPI_CHANNEL, data);
	
	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};

}

void sendRestart()
{
	unsigned int data;
	data = 0x80000000;

	// send data over
	SpiChnPutC(SPI_CHANNEL, data);

	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};
}

void sendGameOver()
{
	unsigned int data;
	data = 0x80000001;

	// send data over
	SpiChnPutC(SPI_CHANNEL, data);

	// clear out the receiving side
	SpiChnGetC(SPI_CHANNEL);
	while(SpiChnIsBusy(SPI_CHANNEL)){};
}

void loadTargets(void){
    size_t i;
    total_target_count = 0;
    for(i=0; i<200; ++i){
        targets[i].x_position = ((i/8)*430-i*70+200) % MAX_X;
        targets[i].y_position = ((i/8)*350-i*90+300) % MAX_Y;
        targets[i].play_time = i*40 + 300;
        targets[i].is_rolling = i%10==0;
        ++total_target_count;
    }
    
    
}

void startGame(void){
        loadTargets();
	// Initialize the SPI Channel 2 to be a master, reverse
		// clock, have 32 bits, enabled frame mode, and
        SpiChnOpen(SPI_CHANNEL, 
                   SPI_OPEN_MSTEN|SPI_OPEN_CKE_REV|
                   SPI_OPEN_MODE32|SPI_OPEN_FRMEN, SRC_CLK_DIV);
        INTSetVectorPriority(_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_2);
        INTSetVectorSubPriority(_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
        INTClearFlag(INT_T2);
        INTEnable(INT_T2, INT_ENABLED);

        // configure for multi-vectored mode
        INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

        // enable interrupts
        INTEnableInterrupts();

        /* Open Timer2 with Period register value */
        OpenTimer2(T2_ON | T2_PS_1_256, CLOCK_RATE/256/60);
        
        IFS0CLR = 0x00000100;
        IEC0SET = 0x00000100;
        IPC2SET = 0x0000001C;

        T2CONSET = 0x8000;
        
        
        life = 128;
        score = 0;
        timer = 0;
        set_mouse_x_position(MAX_X/2);
        set_mouse_y_position(MAX_Y/2);
        last_button_pressed_state = FALSE;
        sendRestart();
        is_game_over = FALSE;
        is_game_active = TRUE;
        
}

BOOL is_mouse_in_target(target next_target){
    return pow((TARGET_RADIUS),2) >= pow((mouse_x_position()-next_target.x_position), 2) + 
                                     pow((mouse_y_position()-next_target.y_position), 2);
}

BOOL is_mouse_in_circle(x_position, y_position, radius){
    return pow(radius,2) >= pow((mouse_x_position()-x_position), 2) + 
                                     pow((mouse_y_position()-y_position), 2);
}


void gameOverLoop(void){
    if(timer < 255)
    {
        timer = timer + 5;
        timer = timer > 255? 255: timer;
    }
}

void gameLoop(void){

    BOOL button_pressed_state = (BOOL) Appl_Button_report_buffer[0];
    int score_type = 0;
    BOOL delete_octagon = FALSE;
    
    
    if (is_roll_active && timer >= start_roll_target->play_time) {
        int delta_t = timer - start_roll_target->play_time;
        int total_delta_t = end_roll_target->play_time - start_roll_target->play_time;
        int total_delta_x = end_roll_target->x_position - start_roll_target->x_position;
        int total_delta_y = end_roll_target->y_position - start_roll_target->y_position;
        int roll_x = delta_t*total_delta_x/total_delta_t + start_roll_target->x_position;
        int roll_y = delta_t*total_delta_y/total_delta_t + start_roll_target->y_position;
        if (delta_t % 6 == 0){
            if(button_pressed_state && is_mouse_in_circle(roll_x, roll_y, 50)){
                life += 2;
                score += 30;
            } else {
                life -= 2;
            }
        }
        rollOctagon(roll_x, roll_y);
    } else if (next_target < total_target_count) {
        BOOL mouse_clicked = button_pressed_state && !last_button_pressed_state;
        if (mouse_clicked && is_mouse_in_target(targets[next_target])){
            
            if (targets[next_target].play_time - timer <= 8){
                score_type = 3;
            } else if (targets[next_target].play_time - timer <= 16){
                score_type = 2;
            } else if (targets[next_target].play_time - timer <= 24){
                score_type = 1;
            } else {
                score_type = 0;
            }

            delete_octagon = TRUE;
        }
    }
    
    
        
    if(next_target_to_appear < total_target_count && 
       targets[next_target_to_appear].play_time == timer + TARGET_LIFE){
        
        addNewOctagon(targets[next_target_to_appear].x_position, 
                      targets[next_target_to_appear].y_position);
        if(targets[next_target_to_appear].is_rolling){
            addNewPathTo(targets[next_target_to_appear+1].x_position, 
                         targets[next_target_to_appear+1].y_position);
        }
        
                            
        next_target_to_appear++;
    }
    
    if((next_target < total_target_count && targets[next_target].play_time == timer) || delete_octagon){
    
        if (is_roll_active) {
            score_type = roll_score_type;
            deleteOldestOctagon(targets[next_target].x_position, targets[next_target].y_position, 0);
            deleteOldestOctagon(targets[next_target].x_position, targets[next_target].y_position, roll_score_type);
            stopRollOctagon();
            is_roll_active = FALSE;
            deletePath();
        } else if (targets[next_target].is_rolling){
            is_roll_active = TRUE;
            start_roll_target = &targets[next_target];
            end_roll_target = &targets[next_target+1];
            roll_score_type = score_type;
        } else {
            deleteOldestOctagon(targets[next_target].x_position, targets[next_target].y_position, score_type);
        }
        life += score_type*3;
        score += score_type*100;
        if(score_type == 0){
            life -= 20;
        }
        next_target++;
        
    }
    
    last_button_pressed_state = button_pressed_state;

    timer++;
    if(timer%12 == 0){
        life--;
        score++;
    }
    life = life < 0   ? 0   :
           life > 255 ? 255 :
           life;
           
    
    
    if(life == 0 || next_target == total_target_count){
        is_game_active = FALSE;
        is_game_over = TRUE;
        sendGameOver();
        timer = 0;
    }
}

void mouse_actions (void){
	switch(App_State_Mouse)
            {
                case DEVICE_NOT_CONNECTED:
                             USBTasks();
                             
                             if(USBHostHID_ApiDeviceDetect()) /* True if report descriptor is parsed with no error */
                             {
                                App_State_Mouse = DEVICE_CONNECTED;
                             }
                    break;
                case DEVICE_CONNECTED:
                           App_State_Mouse = READY_TO_TX_RX_REPORT;
                    break;
                case READY_TO_TX_RX_REPORT:
                             if(!USBHostHID_ApiDeviceDetect())
                             {
                                App_State_Mouse = DEVICE_NOT_CONNECTED;
                             }
                             else
                             {
                                App_State_Mouse = GET_INPUT_REPORT;
                             }

                    break;
                case GET_INPUT_REPORT:
                            if(USBHostHID_ApiGetReport(Appl_raw_report_buffer.Report_ID,0,
                                                        Appl_raw_report_buffer.ReportSize, Appl_raw_report_buffer.ReportData))
                            {
                               /* Host may be busy/error -- keep trying */
                            }
                            else
                            {
                                App_State_Mouse = INPUT_REPORT_PENDING;
                            }
                            USBTasks();
                    break;
                case INPUT_REPORT_PENDING:
                           if(USBHostHID_ApiTransferIsComplete(&ErrorDriver,&NumOfBytesRcvd))
                            {
                                if(ErrorDriver ||(NumOfBytesRcvd != Appl_raw_report_buffer.ReportSize ))
                                {
                                  ErrorCounter++ ; 
                                  if(MAX_ERROR_COUNTER <= ErrorDriver)
                                     App_State_Mouse = ERROR_REPORTED;
                                  else
                                     App_State_Mouse = READY_TO_TX_RX_REPORT;
                                }
                                else
                                {
                                  ErrorCounter = 0; 
                                  ReportBufferUpdated = TRUE;
                                  App_State_Mouse = READY_TO_TX_RX_REPORT;

                                  App_ProcessInputReport();
                                }
                            }
                    break;

               case ERROR_REPORTED:
                    break;
                default:
                    break;

            }
}

int main (void)
{
        BYTE i;
        int  value;
    
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

		
		startGame();
        while(1)
        {
            USBTasks();
            App_Detect_Device();
            mouse_actions();
            
        }
}

void __ISR(_TIMER_2_VECTOR, ipl7) T2_IntHandler(void)
{
    IFS0CLR = 0x0100;
    sendLifeAndScore(life, score);
    sendTimeandMouse(timer, mouse_x_position() , mouse_y_position());
    if (is_game_active){
        gameLoop();
    }
    if (is_game_over){
        gameOverLoop();
    }
    
}

void App_ProcessInputReport(void)
{
	// data to be sent
    unsigned int data = 0;

   /* process input report received from device */
    USBHostHID_ApiImportData(Appl_raw_report_buffer.ReportData, Appl_raw_report_buffer.ReportSize
                          ,Appl_Button_report_buffer, &Appl_Mouse_Buttons_Details);
    USBHostHID_ApiImportData(Appl_raw_report_buffer.ReportData, Appl_raw_report_buffer.ReportSize
                          ,Appl_XY_report_buffer, &Appl_XY_Axis_Details);

	// calculate new positions
    int new_x_position = x_position + (signed char)Appl_XY_report_buffer[0];
    int new_y_position = y_position + (signed char)Appl_XY_report_buffer[1];

	// adjust positions based on whether new positions are valid
    x_position = new_x_position >= MAX_X * SCALING ? MAX_X * SCALING - 1 :
                 new_x_position <= MIN_X * SCALING ? MIN_X * SCALING :
                 new_x_position;
    y_position = new_y_position >= MAX_Y * SCALING ? MAX_Y * SCALING - 1 :
                 new_y_position <= MIN_Y * SCALING ? MIN_Y * SCALING :
                 new_y_position;
	


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
              //  UART2PrintString( "\r\n***** USB Error - device requires too much current *****\r\n" );
            }
            break;

        case EVENT_VBUS_RELEASE_POWER:
            // Turn off Vbus power.
            // The PIC24F with the Explorer 16 cannot turn off Vbus through software.
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

        case EVENT_HID_RPT_DESC_PARSED:
             #ifdef APPL_COLLECT_PARSED_DATA
                 return(APPL_COLLECT_PARSED_DATA());
             #else
                 return TRUE;
             #endif
            break;

        default:
            break;
    }
    return FALSE;
}


/****************************************************************************
  Function:
    BYTE App_HID2ASCII(BYTE a)
  Description:
    This function converts the HID code of the key pressed to coressponding
    ASCII value. For Key strokes like Esc, Enter, Tab etc it returns 0.
***************************************************************************/
BYTE App_DATA2ASCII(BYTE a) //convert USB HID code (buffer[2 to 7]) to ASCII code
{
   if(a<=0x9)
    {
       return(a+0x30);
    }

   if(a>=0xA && a<=0xF) 
    {
       return(a+0x37);
    }

   return(0);
}


/****************************************************************************
  Function:
    void App_Detect_Device(void)
  Description:
    This function monitors the status of device connected/disconnected
    None
***************************************************************************/
void App_Detect_Device(void)
{
  if(!USBHostHID_ApiDeviceDetect())
  {
     App_State_Mouse = DEVICE_NOT_CONNECTED;
  }
}


/****************************************************************************
  Function:
    BOOL USB_HID_DataCollectionHandler(void)
  Description:
    This function is invoked by HID client , purpose is to collect the 
    details extracted from the report descriptor. HID client will store
    information extracted from the report descriptor in data structures.
    Application needs to create object for each report type it needs to 
    extract.
    For ex: HID_DATA_DETAILS Appl_ModifierKeysDetails;
    HID_DATA_DETAILS is defined in file usb_host_hid_appl_interface.h
    Each member of the structure must be initialized inside this function.
    Application interface layer provides functions :
    USBHostHID_ApiFindBit()
    USBHostHID_ApiFindValue()
    These functions can be used to fill in the details as shown in the demo
    code.

  Return Values:
    TRUE    - If the report details are collected successfully.
    FALSE   - If the application does not find the the supported format.

  Remarks:
    This Function name should be entered in the USB configuration tool
    in the field "Parsed Data Collection handler".
    If the application does not define this function , then HID cient 
    assumes that Application is aware of report format of the attached
    device.
***************************************************************************/
BOOL USB_HID_DataCollectionHandler(void)
{
  BYTE NumOfReportItem = 0;
  BYTE i;
  USB_HID_ITEM_LIST* pitemListPtrs;
  USB_HID_DEVICE_RPT_INFO* pDeviceRptinfo;
  HID_REPORTITEM *reportItem;
  HID_USAGEITEM *hidUsageItem;
  BYTE usageIndex;
  BYTE reportIndex;

  pDeviceRptinfo = USBHostHID_GetCurrentReportInfo(); // Get current Report Info pointer
  pitemListPtrs = USBHostHID_GetItemListPointers();   // Get pointer to list of item pointers

  BOOL status = FALSE;
   /* Find Report Item Index for Modifier Keys */
   /* Once report Item is located , extract information from data structures provided by the parser */
   NumOfReportItem = pDeviceRptinfo->reportItems;
   for(i=0;i<NumOfReportItem;i++)
    {
       reportItem = &pitemListPtrs->reportItemList[i];
       if((reportItem->reportType==hidReportInput) && (reportItem->dataModes == (HIDData_Variable|HIDData_Relative))&&
           (reportItem->globals.usagePage==USAGE_PAGE_GEN_DESKTOP))
        {
           /* We now know report item points to modifier keys */
           /* Now make sure usage Min & Max are as per application */
            usageIndex = reportItem->firstUsageItem;
            hidUsageItem = &pitemListPtrs->usageItemList[usageIndex];

            reportIndex = reportItem->globals.reportIndex;
            Appl_XY_Axis_Details.reportLength = (pitemListPtrs->reportList[reportIndex].inputBits + 7)/8;
            Appl_XY_Axis_Details.reportID = (BYTE)reportItem->globals.reportID;
            Appl_XY_Axis_Details.bitOffset = (BYTE)reportItem->startBit;
            Appl_XY_Axis_Details.bitLength = (BYTE)reportItem->globals.reportsize;
            Appl_XY_Axis_Details.count=(BYTE)reportItem->globals.reportCount;
            Appl_XY_Axis_Details.interfaceNum= USBHostHID_ApiGetCurrentInterfaceNum();
        }
        else if((reportItem->reportType==hidReportInput) && (reportItem->dataModes == HIDData_Variable)&&
           (reportItem->globals.usagePage==USAGE_PAGE_BUTTONS))
        {
           /* We now know report item points to modifier keys */
           /* Now make sure usage Min & Max are as per application */
            usageIndex = reportItem->firstUsageItem;
            hidUsageItem = &pitemListPtrs->usageItemList[usageIndex];

            reportIndex = reportItem->globals.reportIndex;
            Appl_Mouse_Buttons_Details.reportLength = (pitemListPtrs->reportList[reportIndex].inputBits + 7)/8;
            Appl_Mouse_Buttons_Details.reportID = (BYTE)reportItem->globals.reportID;
            Appl_Mouse_Buttons_Details.bitOffset = (BYTE)reportItem->startBit;
            Appl_Mouse_Buttons_Details.bitLength = (BYTE)reportItem->globals.reportsize;
            Appl_Mouse_Buttons_Details.count=(BYTE)reportItem->globals.reportCount;
            Appl_Mouse_Buttons_Details.interfaceNum= USBHostHID_ApiGetCurrentInterfaceNum();
        }
    }

   if(pDeviceRptinfo->reports == 1)
    {
        Appl_raw_report_buffer.Report_ID = 0;
        Appl_raw_report_buffer.ReportSize = (pitemListPtrs->reportList[reportIndex].inputBits + 7)/8;
//        Appl_raw_report_buffer.ReportData = (BYTE*)malloc(Appl_raw_report_buffer.ReportSize);
        Appl_raw_report_buffer.ReportPollRate = pDeviceRptinfo->reportPollingRate;
        status = TRUE;
    }

    return(status);
}