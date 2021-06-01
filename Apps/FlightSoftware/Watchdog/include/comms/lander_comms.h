#ifndef __WATCHDOG_LANDER_COMMS_H__
#define __WATCHDOG_LANDER_COMMS_H__

#include "include/drivers/uart.h"
#include "include/common.h"

typedef struct LanderComms__State LanderComms__State;


typedef enum LanderComms__Status
{
    LANDER_COMMS__STATUS__SUCCESS = 0,
    LANDER_COMMS__STATUS__ERROR_NULL = -1,
    LANDER_COMMS__STATUS__ERROR_ALREADY_INITIALIZED = -2,
    LANDER_COMMS__STATUS__ERROR_NOT_INITIALIZED = -3,
    LANDER_COMMS__STATUS__ERROR_MORE_THAN_ONE_MSG_RECEIVED = -4,
    LANDER_COMMS__STATUS__ERROR_BUFFER_TOO_SMALL = -5,
    LANDER_COMMS__STATUS__ERROR_TX_OVERFLOW = -6,


    LANDER_COMMS__STATUS__ERROR_MPSM_INIT_FAILURE = -10,

    LANDER_COMMS__STATUS__ERROR_UART_RX_FAILURE = -20,
    LANDER_COMMS__STATUS__ERROR_UART_TX_FAILURE = -21

    LANDER_COMMS__STATUS__ERROR_SLIP_ENCODE_FAILURE = -30


} LanderComms__Status;

LanderComms__Status LanderComms__init(LanderComms__State** lcState, UART__State* uartState);

// If we received a message, will return the data. The message is received as
// a SLIP-encoded full IP packet containing a UDP datagram that contains the message data.
// The data returned by this function is the message data inside all of that (i.e.
// the SLIP encoding is decoded and the contents of the IP packet are extracted).
LanderComms__Status LanderComms__tryGetMessage(LanderComms__State* lcState,
                                               BOOL* gotMessage,
                                               uint8_t* buffer,
                                               size_t bufferLen,
                                               size_t* rxDataLen);

// Will send data as contents of a UDP packet, SLIP encode, then send over UART
LanderComms__Status LanderComms__txData(LanderComms__State* lcState, const uint8_t* data, size_t dataLen);


#endif /* __WATCHDOG_LANDER_COMMS_H__ */