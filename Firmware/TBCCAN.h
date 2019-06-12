// TBCCAN.h
// Defines constants used between main and motor controllers for CAN communication
#ifndef TBC_CAN_H
#define TBC_CAN_H

#define MAIN_SEND_BUFF 8
#define MOTOR_SEND_BUFF 6

// Constants for defining CAN message objects for receiving messages
// Main controller CAN objects
#define CANOBJ_MAIN 1
#define CANOBJ_MOTORS 2

// CAN objects for motor controllers
#define CANOBJ_M0 2
#define CANOBJ_M1 3
#define CANOBJ_M2 4
#define CANOBJ_M3 5
#define CANOBJ_M4 6
#define CANOBJ_M5 7
#define CANOBJ_FT 3

// Constants for defining CAN message IDs
#define MAIN_ID 100
#define M0_ID 101
#define M1_ID 102
#define M2_ID 103
#define M3_ID 104
#define M4_ID 105
#define M5_ID 106

// FT Sensor base address
#define FT_SENSOR 432

// Commands being sent to the FT Sensor
#define LONG_REQUEST 0x01
#define SHORT_REQUEST 0x02
#define TAR 0x04

// Addresses for long data
#define X_DATA_LONG 433
#define Y_DATA_LONG 434
#define Z_DATA_LONG 435
#define STATUS_LONG 436

// Addresses for short data
#define XY_DATA_SHORT 437
#define Z_STATUS_SHORT 438

// CAN Command enum
// Command codes that can be sent and received from CAN nodes
enum
{
    CAN_HOME,
    CAN_MOVE,
    CAN_REPORT,
    CAN_SETPI = 4,
    CAN_GETPI = 8,
    CAN_STOP = 16
};

#endif
