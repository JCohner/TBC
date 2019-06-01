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

void TBC_initMainBoard()
{
  TBC_initUART1();
  TBC_initSPI();
  TBC_initLimitSwitch();
}


void TBC_initSPI()
{

}

void TBC_initLimitSwitch()
{

}

void TBC_LimitSwitchIntHandler()
{

}
