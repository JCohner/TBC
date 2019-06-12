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
#include "driverlib/eeprom.h"

#include "TBCMotorController.h"

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

// EEPROM constants
#define EEPROM_LOC 0x0 // Offset from EEPROM starting address

// Constants for physical parameters
#define MAX_LEN 50000 // 30,000 microns = 3 cm; maximum actuator travel
#define ENCODER_TO_MICRON 1.95 // Conversion of encoder ticks to micron

// Constants for PI controller
#define MAX_I 10 // Constant used for anti-windup in PI control
#define MAX_ACCEL 5 // Acceleration limit constant in clock ticks
#define DEADBAND 75 //

// Constants for periods used in timers
#define STEPPER_PERIOD 800 // Period of pulse sent to stepper motor; shortest period before acceleration is needed
#define UPDATE_freq 400 // Period of timer for motor control (400 clock ticks = 40 kHz)
#define QEI_freq 200 // Period of timer for capturing QEI value (200 clock ticks = 80 kHz)

#define FORWARD 1
#define REVERSE -1

// Constants defining the relationship between clock ticks and micron/s of the stepper motor
#define TICK_SLOPE -0.005
#define TICK_INTERCEPT 1000

extern volatile bool homed; // Contains homed value
extern volatile bool stopped;

volatile int limit = NOT_PRESSED; // Keeps track of limit switch state

volatile uint32_t position = 0; // Current position of actuator (in microns)
volatile uint32_t desired_pos = 0; // Desired position based on kinematics
volatile int32_t desired_vel = 0; // Desired velocity based on kinematics
volatile uint32_t ticks = 0;

volatile int stepCount = 0;

// PI controller terms
volatile float KP = 0;
volatile float KI = 0;

// TBC_initMotorBoard
// Initialization function to be called from main loop once.
// Calls all other initialization functions necessary for operation
// except for CAN.
void TBC_initMotorBoard()
{
    // Initialization functions
    TBC_initQEI();
    TBC_initTimers();
    TBC_initLimitSwitch();
    TBC_initStepper();
    TBC_initEEPROM();

    // Reading PI control values from EEPROM to run control loop
    int PIvals[2];
    readEEPROMPI(PIvals);

    KP = (float) PIvals[0] / 100;
    KI = (float) PIvals[1] / 100;

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

    // Enable QEI Peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    SysCtlPeripheralEnable(SYSCTL_PERIPH_QEI0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_QEI0));

    //Unlock GPIOD7 - Like PF0 its used for NMI - Without this step it doesn't work
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY; //In Tiva include this is the same as "_DD" in older versions (0x4C4F434B)
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= 0x80;
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = 0;

    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY; //In Tiva include this is the same as "_DD" in older versions (0x4C4F434B)
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

    //Set Pins to be PHA0 and PHB0
    GPIOPinConfigure(GPIO_PF0_PHA0);
    GPIOPinConfigure(GPIO_PD7_PHB0);
    GPIOPinConfigure(GPIO_PD3_IDX0);

    //Set GPIO pins for QEI. PhA0 -> PD6, PhB0 ->PD7. I believe this sets the pull up and makes them inputs
    GPIOPinTypeQEI(GPIO_PORTD_BASE, GPIO_PIN_3 |  GPIO_PIN_7);
    GPIOPinTypeQEI(GPIO_PORTF_BASE, GPIO_PIN_0);

    //DISable peripheral and int before configuration
    QEIDisable(QEI0_BASE);
    QEIIntDisable(QEI0_BASE,QEI_INTERROR | QEI_INTDIR | QEI_INTTIMER | QEI_INTINDEX);

    // Configure quadrature encoder, use an arbitrary top limit of 1000
    QEIConfigure(QEI0_BASE, (QEI_CONFIG_CAPTURE_A_B  | QEI_CONFIG_NO_RESET  | QEI_CONFIG_QUADRATURE | QEI_CONFIG_NO_SWAP), 30000);

    // Enable the quadrature encoder.
    QEIEnable(QEI0_BASE);

    //Set position to a middle value so we can see if things are working
    QEIPositionSet(QEI0_BASE, 100);
}

// TBC_initStepper
// Initializing PWM and output for STEP and DIR to DRV8825 stepper motor driver.
// Setting up output for micro-stepping
void TBC_initStepper()
{
    // Setting up PWM on PC5
    SysCtlPeripheralEnable(STEPPER_PERIPH);
    while(!SysCtlPeripheralReady(STEPPER_PERIPH)){}

    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0)){}

    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    GPIOPinConfigure(GPIO_PC5_M0PWM7);

    GPIOPinTypePWM(STEPPER_PORT, STEPPER_STEP);

    PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_UP_DOWN |
                  PWM_GEN_MODE_NO_SYNC);

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, STEPPER_PERIOD);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true);

    GPIOPinTypeGPIOOutput(STEPPER_PORT, STEPPER_DIR);

    if(!SysCtlPeripheralReady(MICROSTEP_PERIPH))
    {
        SysCtlPeripheralEnable(MICROSTEP_PERIPH);
        while(!SysCtlPeripheralReady(MICROSTEP_PERIPH)){}
    }

    GPIOPinTypeGPIOOutput(MICROSTEP_PORT, STEPPER_M0 | STEPPER_M1 | STEPPER_M2);
    GPIOPinWrite(MICROSTEP_PORT, STEPPER_M0, STEPPER_M0);
    GPIOPinWrite(MICROSTEP_PORT, STEPPER_M1, 0);
    GPIOPinWrite(MICROSTEP_PORT, STEPPER_M2, 0);
}

// TBC_initEEPROM
//
void TBC_initEEPROM()
{
    int returnCode = 1;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);

    while(returnCode != EEPROM_INIT_OK)
    {
        returnCode = EEPROMInit();
    }
}

// QEIPositionTimerHandler
// Timer handler used to sample linear encoder at 80 kHz (min. sampling rate is 24 kHz in order to sense at least 10 micron steps)
void QEIPositionTimerHandler()
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);

    position = QEIPositionGet(QEI0_BASE);
}

// MotorUpdateHandler
// Timer handler to perform the PI feedback controller
void MotorUpdateHandler()
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    static float i_term = 0;
    int position_err = 0, direction = 0;
    int off = 0;
    int32_t velocity = 0; // Current velocity of actuator (in microns/s)
    int32_t prev_ticks = ticks;
    int32_t ticks = 0;

    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, STEPPER_PERIOD);

    position_err = desired_pos - position;
    i_term += position_err * KI;

    // Anti-windup
    if(i_term > MAX_I)
    {
        i_term = MAX_I;
    }
    else if(i_term < -MAX_I)
    {
        i_term = -MAX_I;
    }

    // Do calculation for PWM frequency
    velocity = KP * (float)position_err + i_term + desired_vel;
    ticks = abs(fabs((float)velocity) * TICK_SLOPE + TICK_INTERCEPT);

    off = 0;

    // Implement acceleration limits so the stepper motors won't stall
    if(ticks < STEPPER_PERIOD) // The frequency is faster than the value observed to be the fastest velocity before acceleration is needed
    {
        if(abs(ticks - prev_ticks) > MAX_ACCEL)
        {
            if(ticks < prev_ticks)
                ticks = prev_ticks - MAX_ACCEL;
            else
                ticks = prev_ticks + MAX_ACCEL;
        }
    }
//    else if(ticks >= TICK_INTERCEPT)
//        off = 1;

    // Checking sign of velocity to determine direction
    if(velocity < 0)
        direction = REVERSE;
    else
        direction = FORWARD;

    // The actuator must be homed, the limit switches cannot be enabled,
    // and the stopped condition can not be set
    if(homed && !limit && !stopped)
    {
       if(desired_pos > MAX_LEN)
       {
           desired_pos = MAX_LEN;
       }

//       if(off)
//       {
//           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0); // Off will get set if the velocity is
//       }
//       else
//       {
//           PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, ticks);
//           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, ticks/2);
//       }
//
//       // Set direction pin
//       if(FORWARD == direction)
//       {
//           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, ticks/2);
//           GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
//       }
//       else if(REVERSE == direction)
//       {
//           GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
//       }

         // TODO: Remove this section after initial testing
//       if(abs(position - desired_pos) <= DEADBAND)
//           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);
//       else if(position < desired_pos)
//       {
//           GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
//           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, STEPPER_PERIOD/2);
//       }
//       else if(position > desired_pos)
//       {
//           GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
//           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, STEPPER_PERIOD/2);
//       }
//       else
//       {
//           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);
//       }

       // TODO: This is performing motion in terms of step count
       // This should be removed as soon as encoder feedback is needed
       if(2*stepCount < desired_pos)
       {
           GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, STEPPER_PERIOD/2);
           stepCount++;
       }
       else if(2*stepCount > desired_pos)
       {
           GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, STEPPER_PERIOD/2);
           stepCount--;
       }

       else
       {
           PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);
       }
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
    if(trigger)
    {
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0); // Turn off motor regardless of direction

        // Indicate home/lower switch was pressed
        limit = LOWER;
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
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, STEPPER_PERIOD);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, STEPPER_PERIOD/2);

    // Wait til limit flag is low and then turn off motor
    while(limit != LOWER){}; // TODO: May want to add a part to inside of while loop to look at QEI to make sure position is changing. If not then actuator could be stalling
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);

    // Move motor back up a set amount
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, STEPPER_PERIOD/2);

    // Wait til limit flag is no longer set and then turn off motor
    while(limit != NOT_PRESSED){};
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);

    // Move back down at a slower speed until the limit is triggered again
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, STEPPER_PERIOD*2); // Should move twice as slow
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, STEPPER_PERIOD);

    // Move motor back down til limit flag is set low
    while(limit != LOWER){}
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);

    // Move motor back up a set amount
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, STEPPER_PERIOD);

    // Wait til limit flag is no longer set and then turn off motor
    while(limit != NOT_PRESSED){}
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);

    QEIPositionSet(QEI0_BASE, 0); // Set QEI to be 0
    stepCount = 0;

    return 0;
}

// setDesiredPosition
// Helper function to set the desired position used in PI controller
void setDesiredPosition(int desired)
{
    float desiredTransform;
    desiredTransform = (float) desired / ENCODER_TO_MICRON; // Encoder ticks correspond to 1.95 micron so need to change position internally
    desired_pos = (int) desiredTransform;
}

// setDesiredVelocity
// Helper function to set the desired velocity used in the PI controller
void setDesiredVelocity(int desired)
{
    desired_vel = desired;
}

// getPosition
// Helper function to retrieve current position of linear encoder
int getPosition()
{
    float desiredTransform;
    desiredTransform = (float) position * ENCODER_TO_MICRON; // Converting from encoder ticks back to micron before sending
    return (int) desiredTransform;
}

// writePI2EEPROM
// Writes the new values of Kp and Ki into EEPROM. This is only called when the user wants to set new values
void writePI2EEPROM(int Kp, int Ki)
{
    e2prom_write_val.Kp = Kp;
    e2prom_write_val.Ki = Ki;

    EEPROMProgram((uint32_t*) &e2prom_write_val, EEPROM_LOC, sizeof(e2prom_write_val));
}

// readEEPROMPI
// Reads the value of Kp and Ki that were stored into EEPROM. This is called at startup and
// if there is a request from the user
void readEEPROMPI(int* ControlVals)
{
    EEPROMRead((uint32_t*) &e2prom_read_val, EEPROM_LOC, sizeof(e2prom_read_val));
    ControlVals[0] = e2prom_read_val.Kp;
    ControlVals[1] = e2prom_read_val.Ki;
}
