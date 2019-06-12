#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#define DEBUG_PERIPH SYSCTL_PERIPH_GPIOF
#define DEBUG_PORT GPIO_PORTF_BASE
#define DEBUG_LED GPIO_PIN_3

#define NUM_P_V 12
#define P_V_DATA_SIZE 1000

enum
{
    IDLE,
    HOME,
    STOP,
    MOVE,
    REPORT,
    SET_PI,
    GET_PI,
    LOAD,
};

// Initialization functions
void TBC_initMainBoard(void);
void TBC_initDebug(void);
void TBC_initSPI(void); // TODO: Implement SPI communication for SD card read/write
void TBC_initLimitSwitch(void);
void TBC_LimitSwitchIntHandler(void);
void TBC_setDebug(int onoff);

#endif
