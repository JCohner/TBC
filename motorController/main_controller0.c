#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/qei.h"

#include "TBCMotorController.h"
#include "TBCCAN.h"

//*****************************************************************************
//! CANBUS implementation
//! - CAN0 peripheral
//! - GPIO port B peripheral (for CAN0 pins)
//! - CAN0RX - PB4
//! - CAN0TX - PB5
//! - INT_CAN0 - CANIntHandler
//*****************************************************************************

// CAN Interrupt and Setup constants
#define CAN_ERROR CAN_INT_INTID_STATUS

#define MOTOR_OBJ CANOBJ_M0
#define MOTOR_ID M0_ID

// State variable to keep track of what to do
volatile int state = IDLE;

// Flag for the interrupt handler to indicate that a message was received.
volatile bool RXFlag = 0;

// CAN error flag
volatile bool ErrFlag = 0;

int new_position;
bool homed = 0;

void initCAN(void);

//*****************************************************************************
// This function is the interrupt handler for the CAN peripheral.  It checks
// for the cause of the interrupt.
//*****************************************************************************
void CANIntHandler(void)
{
    uint32_t CANStatus;

    // Read the CAN interrupt status to find the cause of the interrupt
    CANStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    // If the cause is a controller status interrupt, then get the status
    switch(CANStatus)
    {
        case(CAN_ERROR):
        {
            // Read the controller status.  This will return a field of status
            // error bits that can indicate various errors. Refer to the
            // API documentation for details about the error status bits.
            // The act of reading this status will clear the interrupt.
            CANStatus = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);

            // Set a flag to indicate some errors may have occurred.
            ErrFlag = 1;
            break;
        }
        case(CANOBJ_MAIN):
        {
            // Motor controller received a message from the main unit
            CANIntClear(CAN0_BASE, CANOBJ_MAIN);

            // Set flag to indicate received message is pending.
            RXFlag = 1;

            // Since a message was received, clear any error flags.
            ErrFlag = 0;
            break;
        }
        case(MOTOR_OBJ):
        {
            CANIntClear(CAN0_BASE, MOTOR_OBJ);
            ErrFlag = 0;
            break;
        }
        default:
            break;
    }
}

int main(void)
{
    tCANMsgObject MainMessage;
    uint8_t pMainMsgData[8];

    tCANMsgObject MotorMessage;
    uint32_t MotorMsgData;
    uint8_t *pMotorMsgData;

    pMotorMsgData = (uint8_t *)&MotorMsgData;

    int invert = 0;

    // Set the clocking to run directly from the external crystal/oscillator.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    IntMasterDisable();
    initCAN();
    TBC_initMotorBoard();
    IntMasterEnable();

    // Initialize a message object to be used for receiving CAN messages.
    // In order to receive any CAN ID, the ID and mask must both
    // be set to 0, and the ID filter enabled.
    MainMessage.ui32MsgID = MAIN_ID;
    MainMessage.ui32MsgIDMask = 0xfffff;
    MainMessage.ui32Flags = (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER |
                             MSG_OBJ_EXTENDED_ID);
    MainMessage.ui32MsgLen = 8;

    // Initialize a message object to be used for sending CAN messages.
    MotorMsgData = 0;
    MotorMessage.ui32MsgID = MOTOR_ID;
    MotorMessage.ui32MsgIDMask = 0;
    MotorMessage.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    MotorMessage.ui32MsgLen = sizeof(pMotorMsgData);
    MotorMessage.pui8MsgData = pMotorMsgData;

    // Now load the message object into the CAN peripheral.  Once loaded the
    // CAN will receive any message on the bus, and an interrupt will occur.
    CANMessageSet(CAN0_BASE, CANOBJ_MAIN, &MainMessage, MSG_OBJ_TYPE_RX);

    GPIOPinWrite(DEBUG_PORT, DEBUG_LED, 0);

    for(;;)
    {
        switch(state)
        {
            case IDLE:
            {
                // If the flag is set, there is a message ready to be processed
                if(RXFlag)
                {
                    char motor, command, message;

                    GPIOPinWrite(DEBUG_PORT, DEBUG_LED, 0);

                    MainMessage.pui8MsgData = pMainMsgData;

                    // Read the message from the CAN.
                    CANMessageGet(CAN0_BASE, CANOBJ_MAIN, &MainMessage, 0);
                    motor = pMainMsgData[0];
                    command = pMainMsgData[1];

                    // Clear the pending message flag so that the interrupt handler can
                    // set it again when the next message arrives.
                    RXFlag = 0;

                    if(MOTOR_OBJ == motor)
                    {

                        // If the actuator is not homed and the received message wasn't to home then don't do any motion
                        if(!homed && (command != CAN_HOME))
                        {
                            // Send a message back to the controller to say the motor is not homed
                            // TODO: choose message to represent not homed

                            pMotorMsgData[0] = MOTOR_ID;
                            pMotorMsgData[3] = 0xAA;

                            CANMessageSet(CAN0_BASE, MOTOR_OBJ, &MotorMessage, MSG_OBJ_TYPE_TX);
                            GPIOPinWrite(DEBUG_PORT, DEBUG_LED, DEBUG_LED);
                        }
                        // If command is HOME then change state to HOME
                        else if(CAN_HOME == command)
                        {
                            state = HOME;
                            break;
                        }
                        else
                        {
                            if(pMainMsgData[0] == CAN_MOVE)
                            {
                                state = MOVE;
                            }
                        }
                    }
                }
                break;
            }

            case HOME:
            {
                homed = home();
                state = IDLE;
                break;
            }

            case MOVE:
            {
                // Check to see if there is an indication that some messages were
                // lost.
                if(MainMessage.ui32Flags & MSG_OBJ_DATA_LOST)
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                }

                // Setting desired position
                new_position = pMainMsgData[2];
                new_position = ((new_position << 8) | pMainMsgData[3]) & 0xFFFF;
                setDesiredPosition(new_position);

                pMotorMsgData[0] = MOTOR_ID;
                pMotorMsgData[3] = getPosition();

                CANMessageSet(CAN0_BASE, MOTOR_OBJ, &MotorMessage, MSG_OBJ_TYPE_TX);

                // Check the error flag to see if errors occurred
                /*
                if(ErrFlag)
                {
                    //Probably add a condition to turn on a LED
                }
                */
                state = IDLE;
                break;
            }

            case SEND_POS:
            {
                break;
            }
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
    // bus timing for a nominal configuration.
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);

    // Enable interrupts on the CAN peripheral.
    CANIntRegister(CAN0_BASE, CANIntHandler); // if using dynamic vectors
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);

    // Enable the CAN interrupt on the processor (NVIC).
    IntEnable(INT_CAN0);

    // Enable the CAN for operation.
    CANEnable(CAN0_BASE);
}
