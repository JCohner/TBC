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

//*****************************************************************************
//! CANBUS implementation
//! - CAN0 peripheral
//! - GPIO port B peripheral (for CAN0 pins)
//! - CAN0RX - PB4
//! - CAN0TX - PB5
//! - INT_CAN0 - CANIntHandler
//*****************************************************************************
//! Stepper implementation
//! - PWM3 peripheral
//! - GPIO port C peripheral (for PWM7 and DIR output)
//! - PWM7 - PC5
//! - DIR - PC6
//*****************************************************************************
//! QEI implementation
//! - QEI peripheral
//! - GPIO port D peripheral (for QEI0)
//! - Channel A - PD6
//! - Channel B - PD7
//! - Index - PD3
//*****************************************************************************

#define PERIOD 16000 // Period of PWM signal sent to stepper in clock ticks equal to 1 ms
#define UPDATE_freq 8000 // Period of timer for motor control
#define QEI_freq 130 // Period of timer for capturing QEI value

// CAN Message constants
#define CAN_ERROR CAN_INT_INTID_STATUS
#define MAIN_CON 1
#define MOTOR_CON 2

// Flag for the interrupt handler to indicate that a message was received.
volatile bool RXFlag = 0;

// CAN error flag
volatile bool ErrFlag = 0;

volatile uint32_t position = 0;
volatile uint32_t desired_pos = 0;
volatile bool move = false;
volatile uint32_t stepCount = 0;

void initCAN(void);
void initStepper(void);
void setDir(int dir);
void initQEI(void);
void initQEITimer(void);
void initStepCountTimer(void);

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
        case(MAIN_CON):
        {
            // Motor controller received a message from the main unit
            CANIntClear(CAN0_BASE, 1);

            // Set flag to indicate received message is pending.
            RXFlag = 1;

            // Since a message was received, clear any error flags.
            ErrFlag = 0;
            break;
        }
        case(MOTOR_CON):
        {
            CANIntClear(CAN0_BASE, 2);
            ErrFlag = 0;
            break;
        }
        default:
            break;
    }
}

// Incrementing the step count at a fixed frequency in order to
// determine if the motion has completed
void QEIPositionTimerHandler(void)
{
    static uint32_t prev_position = 0;

    TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);

    prev_position = position;
    position = QEIPositionGet(QEI0_BASE);
}

void MotorUpdateHandler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    if(position < desired_pos)
    {
        setDir(0);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PERIOD/2);
    }
    else
    {
        setDir(1);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PERIOD/2);
    }
}

int main(void)
{

    tCANMsgObject sCANMessage1;
    uint8_t pui8Msg1Data[8];

    tCANMsgObject sCANMessage2;
    uint32_t ui32Msg2Data;
    uint8_t *pui8Msg2Data;

    pui8Msg2Data = (uint8_t *)&ui32Msg2Data;

    // Set the clocking to run directly from the external crystal/oscillator.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    IntMasterDisable();
    initCAN();
    initQEI();
    initStepCountTimer();
    IntMasterEnable();

    initStepper();

    // Initialize a message object to be used for receiving CAN messages.
    // In order to receive any CAN ID, the ID and mask must both
    // be set to 0, and the ID filter enabled.
    sCANMessage1.ui32MsgID = 0x1001;
    sCANMessage1.ui32MsgIDMask = 0xfffff;
    sCANMessage1.ui32Flags = (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER |
                              MSG_OBJ_EXTENDED_ID);
    sCANMessage1.ui32MsgLen = 8;

    // Initialize a message object to be used for sending CAN messages.
    ui32Msg2Data = 0;
    sCANMessage2.ui32MsgID = 0x2001;
    sCANMessage2.ui32MsgIDMask = 0;
    sCANMessage2.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    sCANMessage2.ui32MsgLen = sizeof(pui8Msg2Data);
    sCANMessage2.pui8MsgData = pui8Msg2Data;

    // Now load the message object into the CAN peripheral.  Once loaded the
    // CAN will receive any message on the bus, and an interrupt will occur.
    CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_RX);

    // Setting up output pin to turn on when there is an error
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));


    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

    // Enter loop to process received messages.  This loop just checks a flag
    // that is set by the interrupt handler, and if set it reads out the
    // message and displays the contents.  This is not a robust method for
    // processing incoming CAN data and can only handle one messages at a time.
    // If many messages are being received close together, then some messages
    // may be dropped.  In a real application, some other method should be used
    // for queuing received messages in a way to ensure they are not lost.  You
    // can also make use of CAN FIFO mode which will allow messages to be
    // buffered before they are processed.
    for(;;)
    {
        // If the flag is set, there is a message ready to be processed
        if(RXFlag)
        {
            sCANMessage1.pui8MsgData = pui8Msg1Data;

            // Read the message from the CAN.
            CANMessageGet(CAN0_BASE, 1, &sCANMessage1, 0);

            // Clear the pending message flag so that the interrupt handler can
            // set it again when the next message arrives.
            RXFlag = 0;

            if(pui8Msg1Data[0] == 0x02)
            {
                // Check to see if there is an indication that some messages were
                // lost.
                if(sCANMessage1.ui32Flags & MSG_OBJ_DATA_LOST)
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                }

                // Setting desired position
                desired_pos = pui8Msg1Data[3];

                pui8Msg2Data[0] = 0x2001;
                pui8Msg2Data[3] = position;

                CANMessageSet(CAN0_BASE, 2, &sCANMessage2, MSG_OBJ_TYPE_TX);

                // Check the error flag to see if errors occurred
                /*
                if(ErrFlag)
                {
                    //Probably add a condition to turn on a LED
                }
                */
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

void initStepper()
{
    // Setting up PWM on PC5
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC));

    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0));

    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    GPIOPinConfigure(GPIO_PC5_M0PWM7);

    GPIOPinTypePWM(GPIO_PORTC_BASE, GPIO_PIN_5);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_UP_DOWN |
                    PWM_GEN_MODE_NO_SYNC);

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, PERIOD);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true);

    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_6);
}

void setDir(int dir)
{
    switch(dir)
    {
        case 0:
        {
            // Reversing direction
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
        }
        case 1:
        {
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
        }
        default:
        {
            // This case should never be entered so if it does then
            // turn off the motor
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);
        }
    }
}

void initQEI()
{
    // Enable QEI Peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));

    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_QEI0));

    //Unlock GPIOD7 - Like PF0 its used for NMI - Without this step it doesn't work
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY; //In Tiva include this is the same as "_DD" in older versions (0x4C4F434B)
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= 0x80;
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = 0;

    //Set Pins to be PHA0 and PHB0
    GPIOPinConfigure(GPIO_PD6_PHA0);
    GPIOPinConfigure(GPIO_PD7_PHB0);
    GPIOPinConfigure(GPIO_PD3_IDX0);

    //Set GPIO pins for QEI. PhA0 -> PD6, PhB0 ->PD7. I believe this sets the pull up and makes them inputs
    GPIOPinTypeQEI(GPIO_PORTD_BASE, GPIO_PIN_3 | GPIO_PIN_6 |  GPIO_PIN_7);

    //DISable peripheral and int before configuration
    QEIDisable(QEI0_BASE);
    QEIIntDisable(QEI0_BASE,QEI_INTERROR | QEI_INTDIR | QEI_INTTIMER | QEI_INTINDEX);

    // Configure quadrature encoder, use an arbitrary top limit of 1000
    QEIConfigure(QEI0_BASE, (QEI_CONFIG_CAPTURE_A_B  | QEI_CONFIG_NO_RESET  | QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), 10000);

    // Enable the quadrature encoder.
    QEIEnable(QEI0_BASE);

    //Set position to a middle value so we can see if things are working
    QEIPositionSet(QEI0_BASE, 0);
}

void initQEITimer()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));

    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_B, QEI_freq);

    TimerIntEnable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    IntEnable(INT_TIMER0B);
    TimerIntRegister(TIMER0_BASE, TIMER_B, QEIPositionTimerHandler);

    TimerEnable(TIMER0_BASE, TIMER_B);
}

void initStepCountTimer()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));

    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, UPDATE_freq);

    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);
    TimerIntRegister(TIMER0_BASE, TIMER_A, MotorUpdateHandler);

    TimerEnable(TIMER0_BASE, TIMER_A);
}
