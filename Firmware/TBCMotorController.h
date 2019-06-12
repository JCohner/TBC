// TBCMotorController.h
// Helper file used to initialize functions for motor controller
// operation

#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#define STEPPER_PERIPH SYSCTL_PERIPH_GPIOC
#define STEPPER_PORT GPIO_PORTC_BASE
#define STEPPER_DIR GPIO_PIN_6
#define STEPPER_STEP GPIO_PIN_5

#define MICROSTEP_PERIPH SYSCTL_PERIPH_GPIOD
#define MICROSTEP_PORT GPIO_PORTD_BASE
#define STEPPER_M0 GPIO_PIN_0
#define STEPPER_M1 GPIO_PIN_1
#define STEPPER_M2 GPIO_PIN_2

#define DEBUG_PERIPH SYSCTL_PERIPH_GPIOF
#define DEBUG_PORT GPIO_PORTF_BASE
#define DEBUG_LED GPIO_PIN_3

#define LIMIT_SWITCH_PERIPH SYSCTL_PERIPH_GPIOE
#define LIMIT_SWITCH_PORT GPIO_PORTE_BASE
#define LIMIT_INPUT GPIO_PIN_3

struct CONTROL_VALS
{
    int Kp;
    int Ki;
};

struct CONTROL_VALS e2prom_write_val;
struct CONTROL_VALS e2prom_read_val;

// State machine enumerations
enum
{
    IDLE,
    HOME,
    MOVE,
    SEND_POS,
    WRITE_PI,
    READ_PI,
    STOP
};

// Limit switch enumerations
enum
{
    NOT_PRESSED,
    LOWER,
    UPPER
};

// Errors
#define ERROR_NOT_HOMED 1 // Bit will be set if actuator is not homed but a message is received to do something else
#define ERROR_HOMING 2 // Bit will be set if actuator was homing but an error was encountered
#define ERROR_CAN 4 // Bit will be set if a CAN error is detected

//

// Initialization functions
void TBC_initMotorBoard(void);
void TBC_initLimitSwitch(void);
void TBC_initTimers(void);
void TBC_initQEI(void);
void TBC_initStepper(void);
void TBC_initEEPROM(void);

// Interrupt Handlers
void QEIPositionTimerHandler(void);
void MotorUpdateHandler(void);
void LimitSwitchHandler(void);

// Helper functions for main
int home(void);
void setDesiredPosition(int desired);
int getPosition(void);
void setDesiredVelocity(int desired);

// Functions to read and write PI controller values from EEPROM
void writePI2EEPROM(int Kp, int Ki);
void readEEPROMPI(int* ControlVals);

#endif
