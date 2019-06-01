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

#include "driverlib/systick.h"

#include "TBCMotorController.h"

#define FORWARD 1
#define REVERSE -1

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

#define MAX_LEN 30000 // 30,000 microns = 3 cm

#define KP 0
#define KI 0
#define MAX_I 10
#define MAX_ACCEL 50

#define PERIOD 800 // Period of PWM signal sent to stepper in clock ticks equal to 1 ms
#define UPDATE_freq 800 // Period of timer for motor control
#define QEI_freq 200 // Period of timer for capturing QEI value

extern bool start;
extern bool homed;

volatile int limit = NOT_PRESSED;

volatile uint32_t position = 0;
volatile uint32_t desired_pos = 0;

// TBC_initMotorBoard
// Initialization function to be called from main loop once.
// Calls all other initialization functions necessary for operation
// except for CAN.
void TBC_initMotorBoard()
{
    TBC_initQEI();
    TBC_initTimers();
    TBC_initLimitSwitch();
    TBC_initStepper();

    // Setting up output pin to turn on when there is an error
    SysCtlPeripheralEnable(DEBUG_PERIPH);
    while(!SysCtlPeripheralReady(DEBUG_PERIPH));

    GPIOPinTypeGPIOOutput(DEBUG_PORT, DEBUG_LED);
    GPIOPinWrite(DEBUG_PORT, DEBUG_LED, 0);
}

// TBC_initLimitSwitch
// Initializes limit switch input to detect when it has been triggered
void TBC_initLimitSwitch()
{
    SysCtlPeripheralEnable(LIMIT_SWITCH_PERIPH);
    while(!SysCtlPeripheralReady(LIMIT_SWITCH_PERIPH));

    GPIOPinTypeGPIOInput(LIMIT_SWITCH_PORT, LIMIT_INPUT);
    GPIOIntTypeSet(LIMIT_SWITCH_PORT, LIMIT_INPUT, GPIO_BOTH_EDGES);
    GPIOIntEnable(LIMIT_SWITCH_PORT, LIMIT_INPUT);

    GPIOIntRegister(LIMIT_SWITCH_PORT, LimitSwitchHandler);

    return;
}

// TBC_initTimers
// Initializes timers for QEI sampling and stepper motor control
void TBC_initTimers()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));

    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC | TIMER_CFG_B_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, UPDATE_freq);
    TimerLoadSet(TIMER0_BASE, TIMER_B, QEI_freq);

    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);
    TimerIntRegister(TIMER0_BASE, TIMER_A, MotorUpdateHandler);

    TimerIntEnable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    IntEnable(INT_TIMER0B);
    TimerIntRegister(TIMER0_BASE, TIMER_B, QEIPositionTimerHandler);

    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(TIMER0_BASE, TIMER_B);
}

// TBC_initQEI
// Initializing QEI interface to read linear encoder
void TBC_initQEI()
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

    // Configure quadrature encoder, use an arbitrary top limit of 30000
    QEIConfigure(QEI0_BASE, (QEI_CONFIG_CAPTURE_A_B  | QEI_CONFIG_NO_RESET  | QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), MAX_LEN);

    // Enable the quadrature encoder.
    QEIEnable(QEI0_BASE);

    //Set position to 0
    QEIPositionSet(QEI0_BASE, 0);
}

// TBC_initStepper
// Initializing PWM and output for STEP and DIR to DRV8825 stepper motor driver.
// Setting up output for micro-stepping
void TBC_initStepper()
{
    // Setting up PWM on PC5
    SysCtlPeripheralEnable(STEPPER_PERIPH);
    while(!SysCtlPeripheralReady(STEPPER_PERIPH));

    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0));

    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    GPIOPinConfigure(GPIO_PC5_M0PWM7);

    GPIOPinTypePWM(STEPPER_PORT, STEPPER_STEP);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_UP_DOWN |
                  PWM_GEN_MODE_NO_SYNC);

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, PERIOD);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true);

    GPIOPinTypeGPIOOutput(STEPPER_PORT, STEPPER_DIR);

    GPIOPinTypeGPIOOutput(STEPPER_PORT, STEPPER_M0 | STEPPER_M1 | STEPPER_M2);
    GPIOPinWrite(STEPPER_PORT, STEPPER_M0, STEPPER_M0);
    GPIOPinWrite(STEPPER_PORT, STEPPER_M1, 0);
    GPIOPinWrite(STEPPER_PORT, STEPPER_M2, 0);
}

// QEIPositionTimerHandler
// Timer handler used to sample linear encoder at 80 kHz (min. sampling rate is 24 kHz in order to sense at least 10 micron steps)
void QEIPositionTimerHandler()
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);

    position = QEIPositionGet(QEI0_BASE);
}

// TODO: Finish implementation
// MotorUpdateHandler
// Timer handler to perform the PI feedback controller
//
void MotorUpdateHandler()
{
    static int i_term = 0;
    int position_err = 0;
    int direction = 0;

    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    position_err = desired_pos - position;
    i_term += position_err * KI;

    if(i_term > MAX_I)
    {
        i_term = MAX_I;
    }
    else if(i_term < -MAX_I)
    {
        i_term = -MAX_I;
    }

    // Do calculation for PWM frequency; check sign to change direction pin

    // Implement acceleration limits so the stepper motors won't stall

    if(homed)
    {
       if(desired_pos > MAX_LEN)
       {
           desired_pos = MAX_LEN;
       }

       // Set direction pin
       if(FORWARD == direction)
       {

       }
       else if(REVERSE == direction)
       {

       }

       // Set new PWM frequency

    }
}

// LimitSwitchHandler
// GPIO interrupt handler to stop the motion of an actuator if a limit switch is pressed.
// If the platform was not homed yet it must move to down until the limit is hit.
void LimitSwitchHandler()
{
    int trigger;

    GPIOIntClear(LIMIT_SWITCH_PORT, LIMIT_INPUT);
    trigger = GPIOPinRead(LIMIT_SWITCH_PORT, LIMIT_INPUT);

    // If the pin is low, then the switch has been pressed
    if(0 == trigger)
    {
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0); // Turn off motor regardless of direction

        if(FORWARD == QEIDirectionGet(QEI0_BASE))
        {
            // Indicate top limit switch was pressed
            limit = UPPER;
        }
        else if(REVERSE == QEIDirectionGet(QEI0_BASE))
        {
            // Indicate home/lower switch was pressed
            limit = LOWER;
        }
    }
    // If pin was low and is now high then switch is no longer pressed
    else
    {
        limit = NOT_PRESSED;
    }
}

// home
// Performs series of motions with actuator to determine when it has achieved its lowest position designated by limit switches
// Does a fast motion to get to the bottom
int home()
{
    // Set motor PWM frequency to fastest speed that doesn't require acceleration, moving down
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, PERIOD);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PERIOD/2);

    // Wait til limit flag is low and then turn off motor
    while(limit != LOWER){}; // TODO: May want to add a part to inside of while loop to look at QEI to make sure position is changing. If not then actuator could be stalling
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);

    // Move motor back up a set amount
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PERIOD/2);

    // Wait til limit flag is no longer set and then turn off motor
    while(limit != NOT_PRESSED){};
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);

    // Move back down at a slower speed until the limit is triggered again
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, PERIOD*2); // Should move twice as slow
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PERIOD);

    // Move motor back down til limit flag is set low
    while(limit != LOWER){};
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);

    QEIPositionSet(QEI0_BASE, 0); // Set QEI to be 0

    return 1;
}

// setDesiredPosition
// Helper function to set the desired position used in PI controller
void setDesiredPosition(int desired)
{
    desired_pos = desired;
}

// getPosition
// Helper function to retrieve current position of linear encoder
int getPosition(void)
{
    return position;
}
