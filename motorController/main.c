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
#include "driverlib/uart.h"
#include "driverlib/qei.h"
#include "utils/uartstdio.h"

//*****************************************************************************
//! CANBUS implementation
//! - CAN0 peripheral
//! - GPIO port B peripheral (for CAN0 pins)
//! - CAN0RX - PB4
//! - CAN0TX - PB5
//! - INT_CAN0 - CANIntHandler
//*****************************************************************************
//! Console implementation
//! - UART0 peripheral
//! - GPIO port A peripheral (for UART0 pins)
//! - UART0RX - PA0
//! - UART0TX - PA1
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
// 100 Hz
#define PERIOD 16000 // Period of PWM signal sent to stepper in clock ticks equal to 1 ms

// Counters to confirm number of messages sent and received are consistent
// TODO: remove once we are confident CAN is working consistently
volatile uint32_t g_ui32RXMsgCount = 0;
volatile uint32_t g_ui32TXMsgCount = 0;

// Flag for the interrupt handler to indicate that a message was received.
volatile bool g_bRXFlag = 0;

// CAN error flag
volatile bool g_bErrFlag = 0;

volatile uint32_t stepCount = 0;

void initCAN(void);
void initStepper(void);
void setDir(int dir);
void initQEI(void);
void initStepCountTimer(void);

// Sets up UART0 to send messages to the client (putty, etc.)
void InitConsole(void)
{
    // Enable GPIO port A which is used for UART0 pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Configure the pin muxing for UART0 functions on port A0 and A1.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    // Enable UART0 so that we can configure the clock.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    // Select the alternate (UART) function for these pins.
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 115200, 16000000);
}

void SimpleDelay(void)
{
    // Delay cycles for 1 second
    SysCtlDelay(16000000 / 3);
}

//*****************************************************************************
// This function is the interrupt handler for the CAN peripheral.  It checks
// for the cause of the interrupt, and maintains a count of all messages that
// have been received.
//*****************************************************************************
void CANIntHandler(void)
{
    uint32_t ui32Status;

    // Read the CAN interrupt status to find the cause of the interrupt
    ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    // If the cause is a controller status interrupt, then get the status
    if(ui32Status == CAN_INT_INTID_STATUS)
    {
        // Read the controller status.  This will return a field of status
        // error bits that can indicate various errors.  Error processing
        // is not done in this example for simplicity.  Refer to the
        // API documentation for details about the error status bits.
        // The act of reading this status will clear the interrupt.
        ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);

        // Set a flag to indicate some errors may have occurred.
        g_bErrFlag = 1;
    }

    // Check if the cause is message object 1, which what we are using for
    // receiving messages.
    else if(ui32Status == 1)
    {
        // Getting to this point means that the RX interrupt occurred on
        // message object 1, and the message reception is complete.  Clear the
        // message object interrupt.
        CANIntClear(CAN0_BASE, 1);

        // Increment a counter to keep track of how many messages have been
        // received.  In a real application this could be used to set flags to
        // indicate when a message is received.
        // TODO: Remove once variable declarations are removed
        g_ui32RXMsgCount++;

        // Set flag to indicate received message is pending.
        g_bRXFlag = 1;

        // Since a message was received, clear any error flags.
        g_bErrFlag = 0;
    }
    else if(ui32Status == 2)
    {
        // Getting to this point means that the TX interrupt occurred on message object 2
        CANIntClear(CAN0_BASE, 2);
        g_ui32TXMsgCount++; //TODO: remove once variable declarations are removed
        g_bErrFlag = 0;
    }

    // Otherwise, something unexpected caused the interrupt. This should
    // never happen.
    else
    {
        // Spurious interrupt handling can go here.
    }
}

// Incrementing the step count at a fixed frequency in order to
// determine if the motion has completed
void StepperCountHandler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);

    if(stepCount % 2 == 0)
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
    else
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);

    stepCount++;
}

int main(void)
{

    tCANMsgObject sCANMessage1;
    uint8_t pui8Msg1Data[8];

    tCANMsgObject sCANMessage2;
    uint32_t ui32Msg2Data;
    uint8_t *pui8Msg2Data;

    int qeiPosition = 0;

    pui8Msg2Data = (uint8_t *)&ui32Msg2Data;

    // Set the clocking to run directly from the external crystal/oscillator.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    IntMasterDisable();
    InitConsole();
    initCAN();
    initQEI();
    initStepCountTimer();
    IntMasterEnable();

    initStepper();

    // Initialize a message object to be used for receiving CAN messages with
    // any CAN ID.  In order to receive any CAN ID, the ID and mask must both
    // be set to 0, and the ID filter enabled.
    sCANMessage1.ui32MsgID = 0x1001;
    sCANMessage1.ui32MsgIDMask = 0xfffff;
    sCANMessage1.ui32Flags = (MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER |
                              MSG_OBJ_EXTENDED_ID);
    sCANMessage1.ui32MsgLen = 8;

    ui32Msg2Data = 0;
    sCANMessage2.ui32MsgID = 0x2001;
    sCANMessage2.ui32MsgIDMask = 0;
    sCANMessage2.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    sCANMessage2.ui32MsgLen = sizeof(pui8Msg2Data);
    sCANMessage2.pui8MsgData = pui8Msg2Data;

    // Now load the message object into the CAN peripheral.  Once loaded the
    // CAN will receive any message on the bus, and an interrupt will occur.
    CANMessageSet(CAN0_BASE, 1, &sCANMessage1, MSG_OBJ_TYPE_RX);

    // Setting up output pin to flash RGB led on CAN message receptions
    // TODO: Remove once CAN testing has been completed
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));


    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);

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
        unsigned int uIdx;

        // If the flag is set, that means that the RX interrupt occurred and
        // there is a message ready to be read from the CAN
        if(g_bRXFlag)
        {
            // Reuse the same message object that was used earlier to configure
            // the CAN for receiving messages.  A buffer for storing the
            // received data must also be provided, so set the buffer pointer
            // within the message object.
            sCANMessage1.pui8MsgData = pui8Msg1Data;

            // Read the message from the CAN.  Message object number 1 is used
            // (which is not the same thing as CAN ID).  The interrupt clearing
            // flag is not set because this interrupt was already cleared in
            // the interrupt handler.
            CANMessageGet(CAN0_BASE, 1, &sCANMessage1, 0);

            // Clear the pending message flag so that the interrupt handler can
            // set it again when the next message arrives.
            g_bRXFlag = 0;

            // Check to see if there is an indication that some messages were
            // lost.
            if(sCANMessage1.ui32Flags & MSG_OBJ_DATA_LOST)
            {
                UARTprintf("CAN message loss detected\n");
            }

            // Print out the contents of the message that was received.
            UARTprintf("Msg ID=0x%08X len=%u data=0x",
                       sCANMessage1.ui32MsgID, sCANMessage1.ui32MsgLen);
            for(uIdx = 0; uIdx < sCANMessage1.ui32MsgLen; uIdx++)
            {
                UARTprintf("%02X ", pui8Msg1Data[uIdx]);
            }
            UARTprintf("total count=%u\n", g_ui32RXMsgCount);

            if(pui8Msg1Data[3] == 0xff)
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
            else if(pui8Msg1Data[3] == 0xaa)
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

            setDir(1);

            stepCount = 0;
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PERIOD/2);

            while(200 != stepCount){;}
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);

            // Reading data from QEI
            qeiPosition = QEIPositionGet(QEI0_BASE);

            UARTprintf("Sending msg: 0x%02X %02X %02X %02X",
                       pui8Msg2Data[0], pui8Msg2Data[1], pui8Msg2Data[2],
                       pui8Msg2Data[3]);

            CANMessageSet(CAN0_BASE, 2, &sCANMessage2, MSG_OBJ_TYPE_TX);

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
            ui32Msg2Data++;
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
    QEIConfigure(QEI0_BASE, (QEI_CONFIG_CAPTURE_A_B  | QEI_CONFIG_NO_RESET  | QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), 1000);

    // Enable the quadrature encoder.
    QEIEnable(QEI0_BASE);

    //Set position to a middle value so we can see if things are working
    QEIPositionSet(QEI0_BASE, 500);
}

void initStepCountTimer()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));

    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_B, PERIOD);

    TimerIntEnable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    IntEnable(INT_TIMER0B);
    TimerIntRegister(TIMER0_BASE, TIMER_B, StepperCountHandler);

    TimerEnable(TIMER0_BASE, TIMER_B);
}
