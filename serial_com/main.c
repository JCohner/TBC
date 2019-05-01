#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#include "TBCUART.h"

//*****************************************************************************
//
//! \addtogroup can_examples_list
//! <h1>Simple CAN TX (simple_tx)</h1>
//!
//! This example shows the basic setup of CAN in order to transmit messages
//! on the CAN bus.  The CAN peripheral is configured to transmit messages
//! with a specific CAN ID.  A message is then transmitted once per second,
//! using a simple delay loop for timing.  The message that is sent is a 4
//! byte message that contains an incrementing pattern.  A CAN interrupt
//! handler is used to confirm message transmission and count the number of
//! messages that have been sent.
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - CAN0 peripheral
//! - GPIO Port B peripheral (for CAN0 pins)
//! - CAN0RX - PB4
//! - CAN0TX - PB5
//!
//! The following UART signals are configured only for displaying console
//! messages for this example.  These are not required for operation of CAN.
//! - GPIO port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - INT_CAN0 - CANIntHandler
//
//*****************************************************************************

#define BUF_SIZE 200

// CAN IDs
#define MAIN_ID 0x01
#define MOTOR0_ID 600
#define MOTOR1_ID 601
#define MOTOR2_ID 602
#define MOTOR3_ID 603
#define MOTOR4_ID 604
#define MOTOR5_ID 605

// CAN Objects
#define MAIN_ID_OBJ 1
#define MOTOR0_ID_OBJ 2
#define MOTOR1_ID_OBJ 3
#define MOTOR2_ID_OBJ 4
#define MOTOR3_ID_OBJ 5
#define MOTOR4_ID_OBJ 6
#define MOTOR5_ID_OBJ 7

// CAN Receive Flags
volatile bool RXFlag1 = 0;
volatile bool RXFlag2 = 0;
volatile bool RXFlag3 = 0;
volatile bool RXFlag4 = 0;
volatile bool RXFlag5 = 0;
volatile bool RXFlag6 = 0;

//*****************************************************************************
//
// A counter that keeps track of the number of times the TX interrupt has
// occurred, which should match the number of TX messages that were sent.
//
//*****************************************************************************
volatile uint32_t g_ui32TXMsgCount = 0;
volatile uint32_t g_ui32RXMsgCount = 0;


volatile bool g_bRXFlag = 0;

//*****************************************************************************
//
// A flag to indicate that some transmission error occurred.
//
//*****************************************************************************
volatile bool g_bErrFlag = 0;

volatile bool mc0 = 0;
volatile bool mc1 = 0;

void initCAN(void);

//*****************************************************************************
//
// This function is the interrupt handler for the CAN peripheral.  It checks
// for the cause of the interrupt, and maintains a count of all messages that
// have been transmitted.
//
//*****************************************************************************
void CANIntHandler(void)
{
    uint32_t CANStatus;

    // Read the CAN interrupt status to find the cause of the interrupt
    CANStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    // If the cause is a controller status interrupt, then get the status
    switch(CANStatus)
    {
        case(CAN_INT_INTID_STATUS):
        {
            // Read the controller status.  This will return a field of status
            // error bits that can indicate various errors.  Error processing
            // is not done in this example for simplicity.  Refer to the
            // API documentation for details about the error status bits.
            // The act of reading this status will clear the interrupt.  If the
            // CAN peripheral is not connected to a CAN bus with other CAN devices
            // present, then errors will occur and will be indicated in the
            // controller status.
            ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);

            // Set a flag to indicate some errors may have occurred.
            g_bErrFlag = 1;
            break;
        }
        case(MAIN_ID_OBJ):
        {
            // Getting to this point means that the TX interrupt occurred on
            // message object 1, and the message TX is complete.  Clear the
            // message object interrupt.
            CANIntClear(CAN0_BASE, 1);

            // Increment a counter to keep track of how many messages have been
            // sent.  In a real application this could be used to set flags to
            // indicate when a message is sent.
            g_ui32TXMsgCount++;

            // Since the message was sent, clear any error flags.
            g_bErrFlag = 0;
            break;
        }
        case(MOTOR0_ID_OBJ):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, 2);
            RXFlag1 = 1;
            g_ui32RXMsgCount++;
            g_bErrFlag = 0;
            break;
        }
        case(MOTOR1_ID_OBJ):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, 3);
            RXFlag2 = 1;
            g_ui32RXMsgCount++;
            g_bErrFlag = 0;
            break;
        }
        case(MOTOR2_ID_OBJ):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, 4);
            RXFlag3 = 1;
            g_ui32RXMsgCount++;
            g_bErrFlag = 0;
            break;
        }
        case(MOTOR3_ID_OBJ):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, 5);
            RXFlag4 = 1;
            g_ui32RXMsgCount++;
            g_bErrFlag = 0;
            break;
        }
        case(MOTOR4_ID_OBJ):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, 6);
            RXFlag4 = 1;
            g_ui32RXMsgCount++;
            g_bErrFlag = 0;
            break;
        }
        case(MOTOR5_ID_OBJ):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, 7);
            RXFlag5 = 1;
            g_ui32RXMsgCount++;
            g_bErrFlag = 0;
            break;
        }
        default:
            break;
    }
}

//*****************************************************************************
//
// Configure the CAN and enter a loop to transmit periodic CAN messages.
//
//*****************************************************************************
int main(void)
{
    char buffer[BUF_SIZE];
    int motor, position;

    tCANMsgObject MAIN_MSG;
    uint32_t MAIN_MSG_Data;
    uint8_t *pMAIN_MSG_Data;

    pMAIN_MSG_Data = (uint8_t *)&MAIN_MSG_Data;

    tCANMsgObject MOTOR_MSG;
    uint8_t MOTOR_MSG_Data[8];

    // Set the clocking to run directly from the external crystal/oscillator.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    IntMasterDisable();
    initCAN();
    TBC_initUART1();
    IntMasterEnable();

    ui32Msg1Data = 0;
    MAIN_MSG.ui32MsgID = MAIN_ID;
    MAIN_MSG.ui32MsgIDMask = 0;
    MAIN_MSG.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    MAIN_MSG.ui32MsgLen = sizeof(pMAIN_MSG_Data);
    MAIN_MSG.pui8MsgData = pMAIN_MSG_Data;

    MOTOR_MSG.ui32MsgID = MOTOR0_ID;
    MOTOR_MSG.ui32MsgIDMask = 0;
    MOTOR_MSG.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    MOTOR_MSG.ui32MsgLen = 8;

    CANMessageSet(CAN0_BASE, MOTOR0_ID_OBJ, &MOTOR_MSG, MSG_OBJ_TYPE_RX);

    MOTOR_MSG.ui32MsgID = MOTOR1_ID;
    CANMessageSet(CAN0_BASE, MOTOR1_ID_OBJ, &MOTOR_MSG, MSG_OBJ_TYPE_RX);

    MOTOR_MSG.ui32MsgID = MOTOR2_ID;
    CANMessageSet(CAN0_BASE, MOTOR2_ID_OBJ, &MOTOR_MSG, MSG_OBJ_TYPE_RX);

    MOTOR_MSG.ui32MsgID = MOTOR3_ID;
    CANMessageSet(CAN0_BASE, MOTOR3_ID_OBJ, &MOTOR_MSG, MSG_OBJ_TYPE_RX);

    MOTOR_MSG.ui32MsgID = MOTOR4_ID;
    CANMessageSet(CAN0_BASE, MOTOR4_ID_OBJ, &MOTOR_MSG, MSG_OBJ_TYPE_RX);

    MOTOR_MSG.ui32MsgID = MOTOR5_ID;
    CANMessageSet(CAN0_BASE, MOTOR5_ID_OBJ, &MOTOR_MSG, MSG_OBJ_TYPE_RX);

    // Enter loop to send messages.  A new message will be sent once per
    // second.  The 4 bytes of message content will be treated as an uint32_t
    // and incremented by one each time.
    while(1)
    {
        //TODO: Change pui8MsgXData to be new message variable names
        unsigned int uIdx;

        TBC_ReadUART(buffer, BUF_SIZE);
        sscanf(buffer, "%d %d", &motor, &position);

        switch(motor)
        {
            case(0):
            {
                pui8Msg1Data[0] = 0x02;

                // This is the desired position
                pui8Msg1Data[2] = (position >> 4) && 0xFF;
                pui8Msg1Data[3] = position && 0xFF;

                // Print a message to the console showing the message count and the
                // contents of the message being sent.
                UARTprintf("Sending msg: 0x%02X %02X %02X %02X",
                           pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                           pui8Msg1Data[3]);

                // Send the CAN message using object number 1
                CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

                // Check the error flag to see if errors occurred
                if(g_bErrFlag)
                {
                    UARTprintf(" error - cable connected?\n");
                }
                else
                {
                    // If no errors then print the count of message sent
                    UARTprintf(" total count = %u\n", g_ui32TXMsgCount);
                }

                // Increment the value in the message data.
                ui32Msg1Data++;
                break;
            }
            case(1):
            {
                pui8Msg1Data[0] = 0x03;

                // This is the desired position
                pui8Msg1Data[3] = position;

                // Print a message to the console showing the message count and the
                // contents of the message being sent.
                UARTprintf("Sending msg: 0x%02X %02X %02X %02X",
                           pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                           pui8Msg1Data[3]);

                // Send the CAN message using object number 1 (not the same thing as
                // CAN ID, which is also 1 in this example).  This function will cause
                // the message to be transmitted right away.
                CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

                // Check the error flag to see if errors occurred
                if(g_bErrFlag)
                {
                    UARTprintf(" error - cable connected?\n");
                }
                else
                {
                    // If no errors then print the count of message sent
                    UARTprintf(" total count = %u\n", g_ui32TXMsgCount);
                }

                // Increment the value in the message data.
                ui32Msg1Data++;
                break;
            }
            case(2):
            {
                pui8Msg1Data[0] = 0x04;

                // This is the desired position
                pui8Msg1Data[3] = position;

                // Print a message to the console showing the message count and the
                // contents of the message being sent.
                UARTprintf("Sending msg: 0x%02X %02X %02X %02X",
                           pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                           pui8Msg1Data[3]);

                // Send the CAN message using object number 1 (not the same thing as
                // CAN ID, which is also 1 in this example).  This function will cause
                // the message to be transmitted right away.
                CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

                // Check the error flag to see if errors occurred
                if(g_bErrFlag)
                {
                    UARTprintf(" error - cable connected?\n");
                }
                else
                {
                    // If no errors then print the count of message sent
                    UARTprintf(" total count = %u\n", g_ui32TXMsgCount);
                }

                // Increment the value in the message data.
                ui32Msg1Data++;
                break;
            }
            case(3):
            {
                pui8Msg1Data[0] = 0x05;

                // This is the desired position
                pui8Msg1Data[3] = position;

                // Print a message to the console showing the message count and the
                // contents of the message being sent.
                UARTprintf("Sending msg: 0x%02X %02X %02X %02X",
                           pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                           pui8Msg1Data[3]);

                // Send the CAN message using object number 1 (not the same thing as
                // CAN ID, which is also 1 in this example).  This function will cause
                // the message to be transmitted right away.
                CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

                // Check the error flag to see if errors occurred
                if(g_bErrFlag)
                {
                    UARTprintf(" error - cable connected?\n");
                }
                else
                {
                    // If no errors then print the count of message sent
                    UARTprintf(" total count = %u\n", g_ui32TXMsgCount);
                }

                // Increment the value in the message data.
                ui32Msg1Data++;
                break;
            }
            default:
            {
                break; // don't send any messages if there is an unexpected case
            }
        }

        // This is the special case where all of the motors are supposed to go up
        if(motor == 10)
        {
            pui8Msg1Data[0] = 0x02;
            UARTprintf("Sending msg: 0x%02X %02X %02X %02X",
                       pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                       pui8Msg1Data[3]);
            CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

            pui8Msg1Data[0] = 0x03;
            UARTprintf("Sending msg: 0x%02X %02X %02X %02X",
                       pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                       pui8Msg1Data[3]);
            CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

            pui8Msg1Data[0] = 0x04;
            UARTprintf("Sending msg: 0x%02X %02X %02X %02X",
                       pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                       pui8Msg1Data[3]);
            CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

            pui8Msg1Data[0] = 0x05;
            UARTprintf("Sending msg: 0x%02X %02X %02X %02X",
                       pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                       pui8Msg1Data[3]);
            CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

            // Check the error flag to see if errors occurred
            if(g_bErrFlag)
            {
                UARTprintf(" error - cable connected?\n");
            }
            else
            {
                // If no errors then print the count of message sent
                UARTprintf(" total count = %u\n", g_ui32TXMsgCount);
            }

            // Increment the value in the message data.
            ui32Msg1Data++;
        }

        //TODO: change g_bRXFlag to RXFlag0, RXFlag1, etc.
        if(g_bRXFlag)
        {
            sCANMessage2.pui8MsgData = pui8Msg2Data;

            // Read the message from the CAN.  Message object number 1 is used
            // (which is not the same thing as CAN ID).  The interrupt clearing
            // flag is not set because this interrupt was already cleared in
            // the interrupt handler.
            if(mc0)
                CANMessageGet(CAN0_BASE, 2, &sCANMessage2, 0);
            else if(mc1)
                CANMessageGet(CAN0_BASE, 3, &sCANMessage3, 0);

            // Clear the pending message flag so that the interrupt handler can
            // set it again when the next message arrives.
            g_bRXFlag = 0;

            if(mc0)
            {
                // Check to see if there is an indication that some messages were
                // lost.
                if(sCANMessage2.ui32Flags & MSG_OBJ_DATA_LOST)
                {
                    UARTprintf("CAN message loss detected\n");
                }

                // Print out the contents of the message that was received.
                UARTprintf("Msg ID=0x%08X len=%u data=0x",
                           sCANMessage2.ui32MsgID, sCANMessage2.ui32MsgLen);
                for(uIdx = 0; uIdx < sCANMessage2.ui32MsgLen; uIdx++)
                {
                    UARTprintf("%02X ", pui8Msg2Data[uIdx]);
                }
                UARTprintf("total count=%u\n", g_ui32RXMsgCount);
                mc0 = 0;
            }
            else if(mc1)
            {
                // Check to see if there is an indication that some messages were
                // lost.
                if(sCANMessage3.ui32Flags & MSG_OBJ_DATA_LOST)
                {
                    UARTprintf("CAN message loss detected\n");
                }

                // Print out the contents of the message that was received.
                UARTprintf("Msg ID=0x%08X len=%u data=0x",
                           sCANMessage3.ui32MsgID, sCANMessage3.ui32MsgLen);
                for(uIdx = 0; uIdx < sCANMessage3.ui32MsgLen; uIdx++)
                {
                    UARTprintf("%02X ", pui8Msg3Data[uIdx]);
                }
                UARTprintf("total count=%u\n", g_ui32RXMsgCount);
                mc1 = 0;
            }
        }
    }
}

void initCAN()
{
    // For this example CAN0 is used with RX and TX pins on port B4 and B5.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Configure the GPIO pin muxing to select CAN0 functions for these pins.
    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);

    // Enable the alternate function on the GPIO pins.
    GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    // The GPIO port and pins have been set up for CAN.  The CAN peripheral
    // must be enabled.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    // Initialize the CAN controller
    CANInit(CAN0_BASE);

    // Set up the bit rate for the CAN bus.  This function sets up the CAN
    // bus timing for a nominal configuration.  You can achieve more control
    // over the CAN bus timing by using the function CANBitTimingSet() instead
    // of this one, if needed.
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);

    // Enable interrupts on the CAN peripheral.
    CANIntRegister(CAN0_BASE, CANIntHandler); // if using dynamic vectors
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);

    // Enable the CAN interrupt on the processor (NVIC).
    IntEnable(INT_CAN0);

    // Enable the CAN for operation.
    CANEnable(CAN0_BASE);
}
