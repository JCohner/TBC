// Files provided by TI and Code Composer for TI TM4C123G
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "driverlib/uart.h"

#include "utils/uartstdio.h"

// User managed code to initialize main controller board
#include "TBCMainController.h"
#include "TBCUART.h"

#define MOTOR_SEND_PERIOD

void TBC_initMainBoard()
{
  TBC_initUART1();
  TBC_initDebug();
  //TBC_initSPI();
  //TBC_initLimitSwitch();
}

void TBC_initDebug()
{
    // Setting up output pin to turn on when there is an error
    SysCtlPeripheralEnable(DEBUG_PERIPH);
    while(!SysCtlPeripheralReady(DEBUG_PERIPH));

    GPIOPinTypeGPIOOutput(DEBUG_PORT, DEBUG_LED);
    GPIOPinWrite(DEBUG_PORT, DEBUG_LED, 0);
}

//
void TBC_initSPI()
{

}

void TBC_initLimitSwitch()
{

}

void TBC_LimitSwitchIntHandler()
{

}

void TBC_setDebug(int onoff)
{
    if(onoff >= 1)
        GPIOPinWrite(DEBUG_PORT, DEBUG_LED, DEBUG_LED);
    else
        GPIOPinWrite(DEBUG_PORT, DEBUG_LED, 0);
}
