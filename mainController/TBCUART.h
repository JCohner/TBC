#ifndef TBCREADUART_H
#define TBCREADUART_H

// Buffer for sending and receiving from PC over UART
#define COMMAND_BUF_SIZE 4
#define BUF_SIZE 200

// Messages enum
// Command codes that are expected from PC
enum
{
    UART_DYNAMIC,
    UART_STATIC,
    UART_HOME,
    UART_ABORT = 4,
    UART_POS_REPORT = 8,
    UART_STATE_REPORT = 16,
    UART_SET_PI = 32,
    UART_GET_PI = 64,
    UART_READ_FT = 128
};

void TBC_initUART1(void);
void TBC_ReadUART(char* message, int maxLength);

#endif
