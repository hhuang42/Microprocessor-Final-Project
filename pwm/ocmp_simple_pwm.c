/*********************************************************************
 *
 *                  OCMP Simple PWM Application
 *
 *********************************************************************
 * FileName:        ocmp_simple_pwm.c
 * Dependencies:
 * Processor:       PIC32
 *
 * Complier:        MPLAB C32
 *                  MPLAB IDE
 * Company:         Microchip Technology Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the ìCompanyÅE for its PIC32 Microcontroller is intended
 * and supplied to you, the Companyís customer, for use solely and
 * exclusively on Microchip PIC32 Microcontroller products.
 * The software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN ìAS ISÅECONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *
 * $Id: ocmp_simple_pwm.c 7587 2008-01-10 18:16:01Z rajbhartin $
 * $Name: x.x $
 *
 *********************************************************************
 *
 * This Program Demonstrates continuous Pulse generation on
 * Output compare module1 of the PIC32 controller.
 *
 * The H/W setup contains PIC32 placed on an Explorer16 board.
 * Observe the output pulse on OC1 pin using an Oscilloscope.
 *
 **********************************************************************/
#include <plib.h>

#define CLOCK_RATE (40000000)

#define SOUND_RATE (44100)

#define SAMPLE_LENGTH (169)

#define MIN_SAMPLE_VALUE (-32768)

#define MAX_SAMPLE_VALUE (32767)

#define PWM_PERIOD (CLOCK_RATE/SOUND_RATE)



const short samples [SAMPLE_LENGTH] = 
        {1217, 2434, 3647, 4855, 6056, 7249, 8431, 9603, 10760, 11903, 13030, \
        14139, 15228, 16296, 17341, 18362, 19358, 20328, 21269, 22181, 23062, \
        23911, 24728, 25510, 26257, 26967, 27640, 28275, 28871, 29427, 29943, \
        30417, 30849, 31238, 31584, 31887, 32145, 32359, 32529, 32653, 32732, \
        32766, 32755, 32698, 32596, 32450, 32258, 32022, 31741, 31417, 31049, \
        30638, 30185, 29690, 29154, 28578, 27963, 27308, 26616, 25888, 25123, \
        24324, 23491, 22625, 21729, 20802, 19847, 18864, 17855, 16821, 15764, \
        14686, 13587, 12469, 11334, 10183, 9019, 7841, 6653, 5456, 4251, \
        3041, 1826, 609, -610, -1827, -3042, -4252, -5457, -6654, -7842, \
        -9020, -10184, -11335, -12470, -13588, -14687, -15765, -16822, \
        -17856, -18865, -19848, -20803, -21730, -22626, -23492, -24325, \
        -25124, -25889, -26617, -27309, -27964, -28579, -29155, -29691, \
        -30186, -30639, -31050, -31418, -31742, -32023, -32259, -32451, \
        -32597, -32699, -32756, -32767, -32733, -32654, -32530, -32360, \
        -32146, -31888, -31585, -31239, -30850, -30418, -29944, -29428, \
        -28872, -28276, -27641, -26968, -26258, -25511, -24729, -23912, \
        -23063, -22182, -21270, -20329, -19359, -18363, -17342, -16297, \
        -15229, -14140, -13031, -11904, -10761, -9604, -8432, -7250, -6057, \
        -4856, -3648, -2435, -1218, 0};

int index = 0;

int main(void)
{
    
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

    //SetDCOC2PWM(0);
    /*
      The expected output looks like the diagram below with approximately 6% duty-cycle

           ||      ||      ||
     ______||______||______||______

    */
    while(1); /* can have user code here */

    /* Close Output Compare */
    CloseOC2();

    return (0);
}

short next_sample(void){
    short sample = samples[index];
    index = (index + 1) % SAMPLE_LENGTH;
    return sample;
}

void __ISR(_TIMER_2_VECTOR, ipl7) T2_IntHandler(void)
{
    IFS0CLR = 0x0100;
    int sample = next_sample();
    int duty_cycle = (sample-(MIN_SAMPLE_VALUE)) * 
                      PWM_PERIOD/(MAX_SAMPLE_VALUE-MIN_SAMPLE_VALUE);
    SetDCOC2PWM(duty_cycle);
}



