#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#define DEBUG_PERIPH SYSCTL_PERIPH_GPIOF
#define DEBUG_PORT GPIO_PORTF_BASE
#define DEBUG_LED GPIO_PIN_3

enum
{
    IDLE,
    HOME,
    STOP,
    MOVE,
    REPORT,
    SET_PI,
    GET_PI,
    LOAD
};

void TBC_initMainBoard(void);
void TBC_initSPI(void);
void TBC_initLimitSwitch(void);
void TBC_LimitSwitchIntHandler(void);

// May want to add CAN peripheral initializations here as well
// Timers for sending position requests and F/T data?

#endif
