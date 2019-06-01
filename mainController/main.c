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

#include "TBCMainController.h"
#include "TBCUART.h"
#include "TBCCAN.h"

/****************************************************/
/*              CAN Message variables               */
/****************************************************/
// Flags to indicate which message object was received
volatile bool M0_RXFlag = 0;
volatile bool M1_RXFlag = 0;
volatile bool M2_RXFlag = 0;
volatile bool M3_RXFlag = 0;
volatile bool M4_RXFlag = 0;
volatile bool M5_RXFlag = 0;

// A flag to indicate that some transmission error occurred.
volatile bool g_bErrFlag = 0;

tCANMsgObject motorMessage;
uint8_t pMotorMsg[8];
/*****************************************************/

int state = IDLE;

void initCAN(void);
void SendToMotor(int motorID, int com, int data0, int data1);
void CAN_Response(void);

//*****************************************************************************
// This function is the interrupt handler for the CAN peripheral.  It checks
// for the cause of the interrupt.
//*****************************************************************************
void CANIntHandler(void)
{
    uint32_t CANstatus;

    GPIOPinWrite(DEBUG_PORT, DEBUG_LED, DEBUG_LED);

    // Read the CAN interrupt status to find the cause of the interrupt
    CANstatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    switch(CANstatus)
    {
        // If the cause is a controller status interrupt, then get the status
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
            CANstatus = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);

            // Set a flag to indicate some errors may have occurred.
            g_bErrFlag = 1;

            break;
        }

        // Check if the cause is message object 1, which what we are using for
        // sending messages.
        case(CANOBJ_MAIN):
        {
            // TX interrupt occurred on message object 1. Clear the message object interrupt.
            CANIntClear(CAN0_BASE, CANOBJ_MAIN);

            // Since the message was sent, clear any error flags.
            g_bErrFlag = 0;
            break;
        }

        case(CANOBJ_M0):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, CANOBJ_M0);
            M0_RXFlag = 1;
            g_bErrFlag = 0;
            break;
        }

        case(CANOBJ_M1):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, CANOBJ_M0);
            M1_RXFlag = 1;
            g_bErrFlag = 0;
            break;
        }

        case(CANOBJ_M2):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, CANOBJ_M0);
            M2_RXFlag = 1;
            g_bErrFlag = 0;
            break;
        }

        case(CANOBJ_M3):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, CANOBJ_M0);
            M3_RXFlag = 1;
            g_bErrFlag = 0;
            break;
        }

        case(CANOBJ_M4):
         {
             // Getting to this point means that the RX interrupt occurred on message object 2
             CANIntClear(CAN0_BASE, CANOBJ_M0);
             M4_RXFlag = 1;
             g_bErrFlag = 0;
             break;
         }

        case(CANOBJ_M5):
        {
            // Getting to this point means that the RX interrupt occurred on message object 2
            CANIntClear(CAN0_BASE, CANOBJ_M0);
            M5_RXFlag = 1;
            g_bErrFlag = 0;
            break;
        }

        // Otherwise, something unexpected caused the interrupt.  This should
        // never happen.
        default:
        {
            // Spurious interrupt handling can go here.
            break;
        }
    }
}

//*****************************************************************************
//
// Configure the CAN and enter a loop to transmit periodic CAN messages.
//
//*****************************************************************************
int main(void)
{
    // Command variables
    char command_buffer[COMMAND_BUF_SIZE];
    int command = 0, info1 = 0, info2 = 0;

    char buffer[BUF_SIZE];

    // Position variables
    int mp0, mp1, mp2, mp3, mp4, mp5;
    int pos[6];

    tCANMsgObject mainMessage;
    uint32_t MainMsgData;
    uint8_t *pMainMsgData;

    pMainMsgData = (uint8_t *)&MainMsgData;

    // Set the clocking to run directly from the external crystal/oscillator.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    IntMasterDisable();
    initCAN();
    TBC_initUART1();
    IntMasterEnable();

    // Initialize the message object used for sending CAN messages.
    MainMsgData = 0;
    mainMessage.ui32MsgID = MAIN_ID;
    mainMessage.ui32MsgIDMask = 0;
    mainMessage.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    mainMessage.ui32MsgLen = sizeof(pMainMsgData);
    mainMessage.pui8MsgData = pMainMsgData;

    // Initialize the message object used for receiving CAN messages.
    motorMessage.ui32MsgID = M0_ID;
    motorMessage.ui32MsgIDMask = 0xfffff;
    motorMessage.ui32Flags = (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER | MSG_OBJ_EXTENDED_ID);
    motorMessage.ui32MsgLen = 8;

    // Load message object into Motor 0 message object
    CANMessageSet(CAN0_BASE, CANOBJ_M0, &motorMessage, MSG_OBJ_TYPE_RX);

    // Change ID to Motor 1 and load message object into Motor 1 message object
    motorMessage.ui32MsgID = M1_ID;
    CANMessageSet(CAN0_BASE, CANOBJ_M1, &motorMessage, MSG_OBJ_TYPE_RX);

    // Change ID to Motor 2 and load message object into Motor 2 message object
    motorMessage.ui32MsgID = M2_ID;
    CANMessageSet(CAN0_BASE, CANOBJ_M2, &motorMessage, MSG_OBJ_TYPE_RX);

    // Change ID to Motor 3 and load message object into Motor 3 message object
    motorMessage.ui32MsgID = M3_ID;
    CANMessageSet(CAN0_BASE, CANOBJ_M3, &motorMessage, MSG_OBJ_TYPE_RX);

    // Change ID to Motor 4 and load message object into Motor 4 message object
    motorMessage.ui32MsgID = M4_ID;
    CANMessageSet(CAN0_BASE, CANOBJ_M4, &motorMessage, MSG_OBJ_TYPE_RX);

    // Change ID to Motor 5 and load message object into Motor 5 message object
    motorMessage.ui32MsgID = M5_ID;
    CANMessageSet(CAN0_BASE, CANOBJ_M5, &motorMessage, MSG_OBJ_TYPE_RX);

    // Setting up output pin to turn on when there is an error
    SysCtlPeripheralEnable(DEBUG_PERIPH);
    while(!SysCtlPeripheralReady(DEBUG_PERIPH));

    GPIOPinTypeGPIOOutput(DEBUG_PORT, DEBUG_LED);
    GPIOPinWrite(DEBUG_PORT, DEBUG_LED, DEBUG_LED);

    while(1)
    {
        switch(state)
        {
            case IDLE:
            {
                TBC_ReadUART(command_buffer, COMMAND_BUF_SIZE);
                sprintf(command_buffer, "%d %d %d", &command, &info1, &info2);

                switch(command)
                {
                    case UART_DYNAMIC:
                    case UART_STATIC:
                    {
                        state = LOAD;
                        break;
                    }

                    case UART_HOME:
                    {
                        state = HOME;
                        break;
                    }

                    case UART_ABORT:
                    {
                        state = STOP;
                        break;
                    }

                    case UART_POS_REPORT:
                    {
                        state = REPORT;
                        break;
                    }

                    case UART_STATE_REPORT:
                    {
                        // what state would this be? Just for sending errors back?
                        break;
                    }

                    case UART_SET_PI:
                    {
                        state = SET_PI;
                        break;
                    }

                    case UART_GET_PI:
                    {
                        state = GET_PI;
                        break;
                    }

                    case UART_READ_FT:
                    {
                        // TODO: Implement a state and some functions to read from F/T
                        break;
                    }

                    // Error Handling state
                    default:
                    {
                        // If case is entered then an invalid UART command was sent
                        // TODO: Have debug LED flash at some rate and send back ERROR
                        break;
                    }
                }
                break;
            }

            case HOME:
            {
                int i = 0, ID = CANOBJ_M0;

                for(i = 0; i < 6; i++)
                {
                    pMainMsgData[0] = ID;
                    pMainMsgData[1] = CAN_HOME;
                    pMainMsgData[2] = 0;
                    pMainMsgData[3] = 0;
                    //pMainMsgData[4] = 0;
                    //pMainMsgData[5] = 0;

                    // Print a message to the console showing the message count and the
                    // contents of the message being sent.
                    UARTprintf("Sending msg: 0x%02X %02X %02X %02X\n",
                               pMainMsgData[0], pMainMsgData[1], pMainMsgData[2],
                               pMainMsgData[3]);

                    // Send the CAN message using object number 1
                    CANMessageSet(CAN0_BASE, CANOBJ_MAIN, &mainMessage, MSG_OBJ_TYPE_TX);

                    // Check the error flag to see if errors occurred
                    if(g_bErrFlag)
                    {
                        UARTprintf(" error - cable connected?\n");
                    }

                    while(!M0_RXFlag && !M1_RXFlag && !M2_RXFlag && !M3_RXFlag
                            && !M4_RXFlag && !M5_RXFlag)
                    {
                        // Waiting for receipt message; Do nothing for now
                        // TODO: Add checking to see if been waiting for long time
                        // or if error message received
                    }

                    CAN_Response();

                    ID++;
                }
                break;
            }

            case STOP:
            {
                break;
            }

            case MOVE:
            {
                break;
            }

            case REPORT:
            {
                break;
            }

            case SET_PI:
            {
                break;
            }

            case GET_PI:
            {
                break;
            }

            case LOAD:
            {
                // TODO: Switch to new implementation of reading in all position data at once
                // TODO: Add velocity information
                TBC_ReadUART(buffer, BUF_SIZE);
                sscanf(buffer, "%d %d %d %d %d %d", &mp0, &mp1, &mp2, &mp3, &mp4, &mp5);

                pos[0] = mp0;
                pos[1] = mp1;
                pos[2] = mp2;
                pos[3] = mp3;
                pos[4] = mp4;
                pos[5] = mp5;

                int i = 0, ID = 0x02;

                for(i = 0; i < 6; i++)
                {
                    pMainMsgData[0] = ID;
                    pMainMsgData[1] = CAN_MOVE;
                    pMainMsgData[2] = (pos[i] >> 8) & 0xFF;
                    pMainMsgData[3] = pos[i] & 0xFF;
                    //pMainMsgData[4] = (vel[i] >> 8) && 0xFF;
                    //pMainMsgData[5] = vel[i] && 0xFF;

                    // Print a message to the console showing the message count and the
                    // contents of the message being sent.
                    UARTprintf("Sending msg: 0x%02X %02X %02X %02X\n",
                               pMainMsgData[0], pMainMsgData[1], pMainMsgData[2],
                               pMainMsgData[3]);

                    // Send the CAN message using object number 1
                    CANMessageSet(CAN0_BASE, 1, &mainMessage, MSG_OBJ_TYPE_TX);

                    // Check the error flag to see if errors occurred
                    if(g_bErrFlag)
                    {
                        UARTprintf(" error - cable connected?\n");
                    }

                    while(!M0_RXFlag && !M1_RXFlag && !M2_RXFlag && !M3_RXFlag
                            && !M4_RXFlag && !M5_RXFlag)
                    {
                        // Waiting for receipt message; Do nothing for now
                        // TODO: Add checking to see if been waiting for long time
                        // or if error message received
                    }

                    CAN_Response();

                    ID++;
                }
                break;
            }

            default:
                break;
        }
    }
}

void initCAN()
{
    // For this example CAN0 is used with RX and TX pins on port B4 and B5.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));

    // Configure the GPIO pin muxing to select CAN0 functions for these pins.
    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);

    // Enable the alternate function on the GPIO pins.
    GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    // The GPIO port and pins have been set up for CAN.  The CAN peripheral
    // must be enabled.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_CAN0));

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

void CAN_Response()
{
    unsigned int uIdx;

    if(M0_RXFlag)
    {
        motorMessage.pui8MsgData = pMotorMsg;

        // Read the message from the CAN.
        CANMessageGet(CAN0_BASE, CANOBJ_M0, &motorMessage, 0);

        // Clear the pending message flag.
        M0_RXFlag = 0;
    }
    else if(M1_RXFlag)
    {
        motorMessage.pui8MsgData = pMotorMsg;

        // Read the message from the CAN.
        CANMessageGet(CAN0_BASE, CANOBJ_M1, &motorMessage, 0);

        // Clear the pending message flag.
        M0_RXFlag = 0;
    }
    else if(M2_RXFlag)
    {
        motorMessage.pui8MsgData = pMotorMsg;

        // Read the message from the CAN.
        CANMessageGet(CAN0_BASE, CANOBJ_M2, &motorMessage, 0);

        // Clear the pending message flag.
        M0_RXFlag = 0;
    }
    else if(M3_RXFlag)
    {
        motorMessage.pui8MsgData = pMotorMsg;

        // Read the message from the CAN.
        CANMessageGet(CAN0_BASE, CANOBJ_M3, &motorMessage, 0);

        // Clear the pending message flag.
        M0_RXFlag = 0;
    }
    else if(M4_RXFlag)
    {
        motorMessage.pui8MsgData = pMotorMsg;

        // Read the message from the CAN.
        CANMessageGet(CAN0_BASE, CANOBJ_M4, &motorMessage, 0);

        // Clear the pending message flag.
        M0_RXFlag = 0;
    }
    else if(M5_RXFlag)
    {
        motorMessage.pui8MsgData = pMotorMsg;

        // Read the message from the CAN.
        CANMessageGet(CAN0_BASE, CANOBJ_M5, &motorMessage, 0);

        // Clear the pending message flag.
        M0_RXFlag = 0;
    }

    // Check to see if there is an indication that some messages were
    // lost.
    if(motorMessage.ui32Flags & MSG_OBJ_DATA_LOST)
    {
        UARTprintf("CAN message loss detected\n");
    }

    // Print out the contents of the message that was received.
    UARTprintf("Msg ID=0x%08X len=%u data=0x",
               motorMessage.ui32MsgID, motorMessage.ui32MsgLen);
    for(uIdx = 0; uIdx < motorMessage.ui32MsgLen; uIdx++)
    {
        UARTprintf("%02X ", pMotorMsg[uIdx]);
    }
}
