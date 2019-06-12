#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "TBCUART.h"

void TBC_initUART1()
{
    // Enable GPIO port A which is used for UART0 pins.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);

    // Configure the pin muxing for UART0 functions on port A0 and A1.
    GPIOPinConfigure(GPIO_PC4_U1RX);
    GPIOPinConfigure(GPIO_PC5_U1TX);

    // Enable UART0 so that we can configure the clock.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);

    // Select the alternate (UART) function for these pins.
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    // Initialize the UART for console I/O.
    UARTStdioConfig(1, 115200, 16000000);
}

void TBC_ReadUART(char* message, int maxLength)
{
  char data = 0;
  int complete = 0, num_bytes = 0;
  // loop until you get a '\r' or '\n'
  while (!complete) 
  {
    if (UARTCharsAvail(UART1_BASE)) 
    { // if data is available
      data = UARTCharGetNonBlocking(UART1_BASE); // read the data
      if ((data == '\n') || (data == '\r')) 
      {
        complete = 1;
      } 
      else 
      {
        message[num_bytes] = data;
        ++num_bytes;
        // roll over if the array is too small
        if (num_bytes >= maxLength) 
        {
          num_bytes = 0;
        }
      }
    }
  }
  
  // end the string
  message[num_bytes] = '\0';
}
