// TBCCAN.h
// Defines constants used between main and motor controllers for CAN communication
#ifndef TBC_CAN_H
#define TBC_CAN_H

// Constants for defining CAN message objects for receiving messages
#define CANOBJ_MAIN 1
#define CANOBJ_M0 2
#define CANOBJ_M1 3
#define CANOBJ_M2 4
#define CANOBJ_M3 5
#define CANOBJ_M4 6
#define CANOBJ_M5 7

// Constants for defining CAN message IDs
#define MAIN_ID 0x1001
#define M0_ID 0x2001
#define M1_ID 0x3001
#define M2_ID 0x4001
#define M3_ID 0x5001
#define M4_ID 0x6001
#define M5_ID 0x7001

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
