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
#include "driverlib/timer.h"

#include "utils/uartstdio.h"

#include "TBCUART.h"
#include "TBCMainController.h"
#include "TBCCAN.h"

/****************************************************/
/*              CAN Message variables               */
/****************************************************/
// Flags to indicate which message object was received
volatile bool M_RXFlag = 0;

// A flag to indicate that some transmission error occurred.
volatile bool g_bErrFlag = 0;
/*****************************************************/

volatile int reps = 0;
volatile int lines = 0;
volatile int rep_count = 0;
volatile int line_count = 0;
volatile bool stop_send = 0;

#define MOTOR_SEND_FREQ 62500
#define FT_UPDATE_FREQ 16000

char buffer[BUF_SIZE];

//
char send[MAIN_SEND_BUFF];

bool move = 0;

// Array to store position and velocity data that will be sent to motors
int16_t pos_vel_data[P_V_DATA_SIZE][NUM_P_V];

tCANMsgObject sCANMessage1;
uint32_t ui32Msg1Data;
uint8_t *pui8Msg1Data;

tCANMsgObject sCANMessage2;
uint8_t pui8Msg2Data[8];

tCANMsgObject sCANMessage3;
uint32_t ui32Msg3Data;
uint8_t *pui8Msg3Data;

void initCAN(void);
void loadMoveData(int lineCount);
void sendAllMotorsPos(int lineCount);
void sendAllMotorsCommand(int command, int kp, int ki);
void TBC_initTimers(void);
void MotorUpdateHandler(void);
void ReadFTData(void);

//*****************************************************************************
// This function is the interrupt handler for the CAN peripheral.  It checks
// for the cause of the interrupt.
//*****************************************************************************
void CANIntHandler(void)
{
    uint32_t ui32Status;

    // Read the CAN interrupt status to find the cause of the interrupt
    ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    switch(ui32Status)
    {
        case(CAN_INT_INTID_STATUS):
        {
            // Read the controller status.  This will return a field of status
            // error bits that can indicate various errors. Refer to the
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

        // Check if the cause is message object 1, which what we are using for
        // sending messages.
        case(CANOBJ_MAIN):
        {
            // Main Controller has successfully sent CAN message.
            CANIntClear(CAN0_BASE, 1);

            // Since the message was sent, clear any error flags.
            g_bErrFlag = 0;
            break;
        }

        case(CANOBJ_MOTORS):
        {
            // Main controller has received a CAN message. Do more processing outside of interrupt
            CANIntClear(CAN0_BASE, 2);
            M_RXFlag = 1;
            g_bErrFlag = 0;
            break;
        }

        case(CANOBJ_FT):
        {
            // Main Controller successfully sent CAN message to FT sensor
            CANIntClear(CAN0_BASE,3);

            // Since the message was sent, clear any error flags.
            g_bErrFlag = 0;
            break;
        }

        default:
            break;
    }
}

//*****************************************************************************
//
//*****************************************************************************
int main(void)
{
    int state = IDLE;

    // Command variables
    char command_buffer[COMMAND_BUF_SIZE];
    int command = 0, info1 = 0, info2 = 0;

    pui8Msg1Data = (uint8_t *)&ui32Msg1Data;
    pui8Msg3Data = (uint8_t *)&ui32Msg3Data;

    // Set the clocking to run directly from the external crystal/oscillator.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    IntMasterDisable();
    initCAN();
    TBC_initMainBoard();
    //TBC_initTimers();
    IntMasterEnable();

    // Initialize the message object used for sending CAN messages.
    ui32Msg1Data = 0;
    sCANMessage1.ui32MsgID = MAIN_ID;
    sCANMessage1.ui32MsgIDMask = 0;
    sCANMessage1.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    sCANMessage1.ui32MsgLen = sizeof(pui8Msg1Data);
    sCANMessage1.pui8MsgData = pui8Msg1Data;

    // Initialize the message object used for receiving CAN messages.
    sCANMessage2.ui32MsgID = M0_ID;
    sCANMessage2.ui32MsgIDMask = 0;
    sCANMessage2.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    sCANMessage2.ui32MsgLen = 8;

    // Initialize the message to be sent to the FT sensor
    ui32Msg3Data = SHORT_REQUEST;
    sCANMessage3.ui32MsgID = FT_SENSOR;
    sCANMessage3.ui32MsgIDMask = 0;
    sCANMessage3.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    sCANMessage3.ui32MsgLen = sizeof(pui8Msg3Data);
    sCANMessage3.pui8MsgData = pui8Msg3Data;

    CANMessageSet(CAN0_BASE, CANOBJ_MOTORS, &sCANMessage2, MSG_OBJ_TYPE_RX);

    while(1)
    {
        switch(state)
        {
            case IDLE:
            {
                TBC_ReadUART(command_buffer, COMMAND_BUF_SIZE);
                sscanf(command_buffer, "%d %d %d", &command, &info1, &info2);

                switch(command)
                {
                    case UART_DYNAMIC:
                    case UART_STATIC:
                    {
                        stop_send = 1;
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
                        CANMessageSet(CAN0_BASE, 3, &sCANMessage3, MSG_OBJ_TYPE_TX);

                        while(!M_RXFlag){} // Wait until a response is heard

                        CANMessageGet(CAN0_BASE, 2, &sCANMessage2, 0);

                        if(sCANMessage2.ui32MsgID == XY_DATA_SHORT ||
                                sCANMessage2.ui32MsgID == Z_STATUS_SHORT)
                        {
                            // Look at data received from FT sensor and send
                            // back to GUI
                        }

                        break;
                    }

                    default:
                    {
                        // If case is entered then an invalid UART command was sent
                        // TODO: Have debug LED flash at some rate and set some error flag
                    }
                }
                break;
            }

            case HOME:
            {
                sendAllMotorsCommand(CAN_HOME, 0, 0);
                state = IDLE;
                break;
            }

            case STOP:
            {
                sendAllMotorsCommand(CAN_STOP, 0, 0);
                move = 0;
                state = IDLE;
                break;
            }

            case MOVE:
            {
                // Send motor positions that were stored from LOAD state
                move = 1;
                state = IDLE;
                break;
            }

            case REPORT:
            {
                // Tell motors to report their positions and statuses
                sendAllMotorsCommand(CAN_REPORT, 0, 0);
                state = IDLE;
                break;
            }

            case SET_PI:
            {
                // Send Kp and Ki term to each motor to be stored in FLASH
                // If the command is to set Kp and Ki values then info1 and info2 contain those values
                sendAllMotorsCommand(SET_PI, info1, info2);
                state = IDLE;
                break;
            }

            case GET_PI:
            {
                // Ask for Kp and Ki terms stored on flash
                sendAllMotorsCommand(GET_PI, info1, info2);
                state = IDLE;
                break;
            }

            case LOAD:
            {
                // Will get all of the motor positions and velocities to perform motions
                loadMoveData(info1); // Info 1 stores line counts (# of 6 motor moves to be performed in given motion)
                state = MOVE;
                stop_send = 0;
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
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)){}

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

    // Set up the bit rate for the CAN bus.
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);

    // Enable interrupts on the CAN peripheral.
    CANIntRegister(CAN0_BASE, CANIntHandler); // if using dynamic vectors
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);

    // Enable the CAN interrupt on the processor (NVIC).
    IntEnable(INT_CAN0);

    // Enable the CAN for operation.
    CANEnable(CAN0_BASE);
}

// TBC_initTimers
// Initializes timers for QEI sampling and stepper motor control
void TBC_initTimers()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));

    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC | TIMER_CFG_B_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, MOTOR_SEND_FREQ);
    TimerPrescaleSet(TIMER0_BASE, TIMER_A, 16);
    TimerLoadSet(TIMER0_BASE, TIMER_B, FT_UPDATE_FREQ);

    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);
    TimerIntRegister(TIMER0_BASE, TIMER_A, MotorUpdateHandler);

    TimerIntEnable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    IntEnable(INT_TIMER0B);
    TimerIntRegister(TIMER0_BASE, TIMER_B, ReadFTData);

    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(TIMER0_BASE, TIMER_B);
}

void loadMoveData(int lineCount)
{
    int i = 0;

    // Position variables
    int mp0, mp1, mp2, mp3, mp4, mp5;

    for(i = 0; i < lineCount; i++)
    {
        TBC_ReadUART(buffer, BUF_SIZE);
        sscanf(buffer, "%d %d %d %d %d %d", &mp0, &mp1, &mp2, &mp3, &mp4, &mp5);

        pos_vel_data[i][0] = (uint16_t) mp0;
        pos_vel_data[i][1] = 0; // Will be mv0
        pos_vel_data[i][2] = (uint16_t) mp1;
        pos_vel_data[i][3] = 0; // will be mv1
        pos_vel_data[i][4] = (uint16_t) mp2;
        pos_vel_data[i][5] = 0; // will be mv2
        pos_vel_data[i][6] = (uint16_t) mp3;
        pos_vel_data[i][7] = 0; // will be mv3
        pos_vel_data[i][8] = (uint16_t) mp4;
        pos_vel_data[i][9] = 0; // will be mv4
        pos_vel_data[i][10] = (uint16_t) mp5;
        pos_vel_data[i][11] = 0; // will be mv5
    }
}

void sendAllMotorsPos(int lineCount)
{
    unsigned int uIdx;
    int i = 0;

    pui8Msg1Data = (uint8_t *)&ui32Msg1Data;

    // Setting the message object used for sending CAN messages.
    ui32Msg1Data = 0;
    sCANMessage1.ui32MsgID = MAIN_ID;
    sCANMessage1.ui32MsgIDMask = 0;
    sCANMessage1.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    sCANMessage1.ui32MsgLen = sizeof(pui8Msg1Data);
    sCANMessage1.pui8MsgData = pui8Msg1Data;

    // Initialize the message object used for receiving CAN messages.
    sCANMessage2.ui32MsgID = M0_ID;
    sCANMessage2.ui32MsgIDMask = 0;
    sCANMessage2.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    sCANMessage2.ui32MsgLen = 8;

    int ID = CANOBJ_M0;

    for(i = 0; i < 12; i+=2)
    {
        pui8Msg1Data[0] = ID;
        pui8Msg1Data[1] = CAN_MOVE;
        pui8Msg1Data[2] = (pos_vel_data[lineCount][i] >> 8) & 0xFF;
        pui8Msg1Data[3] = pos_vel_data[lineCount][i] & 0xFF;
        pui8Msg1Data[4] = (pos_vel_data[lineCount][i+1] >> 8) & 0xFF;
        pui8Msg1Data[5] = pos_vel_data[lineCount][i+1] & 0xFF;

        // Print a message to the console showing the message count and the
        // contents of the message being sent.
        UARTprintf("Sending msg: 0x%02X %02X %02X %02X %02X %02X\n",
                   pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                   pui8Msg1Data[3], pui8Msg1Data[4], pui8Msg1Data[5]);

        // Send the CAN message using object number 1
        CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

        // Check the error flag to see if errors occurred
        if(g_bErrFlag)
        {

        }

        while(!M_RXFlag)
        {
            // Waiting for receipt message; Do nothing for now
            // TODO: Add checking to see if been waiting for long time
            // or if error message received
        }

        sCANMessage2.pui8MsgData = pui8Msg2Data;

        if(M_RXFlag)
        {
            M_RXFlag = 0;
            CANMessageGet(CAN0_BASE, 2, &sCANMessage2, 0);

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
            UARTprintf("\n");
        }

        ID++;
    }
}

void sendAllMotorsCommand(int command, int kp, int ki)
{
    unsigned int uIdx;
    int i = 0;

    pui8Msg1Data = (uint8_t *)&ui32Msg1Data;

    // Setting the message object used for sending CAN messages.
    ui32Msg1Data = 0;
    sCANMessage1.ui32MsgID = MAIN_ID;
    sCANMessage1.ui32MsgIDMask = 0;
    sCANMessage1.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    sCANMessage1.ui32MsgLen = sizeof(pui8Msg1Data);
    sCANMessage1.pui8MsgData = pui8Msg1Data;

    // Initialize the message object used for receiving CAN messages.
    sCANMessage2.ui32MsgID = M0_ID;
    sCANMessage2.ui32MsgIDMask = 0;
    sCANMessage2.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    sCANMessage2.ui32MsgLen = 8;

    int ID = CANOBJ_M0;

    for(i = 0; i < 12; i+=2)
    {
        pui8Msg1Data[0] = ID;
        pui8Msg1Data[1] = command;
        pui8Msg1Data[2] = (kp >> 8) & 0xFF;
        pui8Msg1Data[3] = kp & 0xFF;
        pui8Msg1Data[4] = (ki >> 8) & 0xFF;
        pui8Msg1Data[5] = ki & 0xFF;

        // Print a message to the console showing the message count and the
        // contents of the message being sent.
        UARTprintf("Sending msg: 0x%02X %02X %02X %02X %02X %02X\n",
                   pui8Msg1Data[0], pui8Msg1Data[1], pui8Msg1Data[2],
                   pui8Msg1Data[3], pui8Msg1Data[4], pui8Msg1Data[5]);

        // Send the CAN message using object number 1
        CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_TX);

        // Check the error flag to see if errors occurred
        if(g_bErrFlag)
        {

        }

        while(!M_RXFlag)
        {
            // Waiting for receipt message; Do nothing for now
            // TODO: Add checking to see if been waiting for long time
            // or if error message received
        }

        sCANMessage2.pui8MsgData = pui8Msg2Data;

        if(M_RXFlag)
        {
            M_RXFlag = 0;
            CANMessageGet(CAN0_BASE, 2, &sCANMessage2, 0);

            // Check to see if there is an indication that some messages were lost.
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
            UARTprintf("\n");
        }

        ID++;
    }
}

// MotorUpdateHandler
// This is a timer interrupt handler that sends motor positions
// and velocities at a set time rate if it is supposed to be moving
void MotorUpdateHandler()
{

    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    if(move)
    {
        if(rep_count < reps)
        {
            if(line_count < lines)
            {
                sendAllMotorsPos(line_count);
                line_count++;
            }

            if(line_count == lines)
            {
                line_count = 0;
                rep_count++;
            }
        }
        else
            move = 0;
    }
}

// ReadFTData
// This is a test function to get reports from the motor controllers and then read from the FT
// sensor. Since the FT sensor was not here during testing, it is currently sending random data.
void ReadFTData()
{
    static int count = 0, i = 0;

    TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);

    //sendAllMotorsCommand(CAN_REPORT, 0, 0);

//    if(2000 == count)
//    {
//        for(i = 0; i < 6; i++)
//        {
//            UARTprintf("%d ", rand());
//        }
//        UARTprintf("\n");
//        count = 0;
//    }

    count++;
}
