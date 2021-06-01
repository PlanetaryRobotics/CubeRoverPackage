/**
 * @file uart.h
 * @brief File for interfacing with UART protocol hardware module
 * File for interfacing with UART protocol hardware module. Also includes some
 * basic communications protocol parsing.
 *
 * UART communication uses eUSCI_A0 (Hercules) and eUSCI_A1 (Lander).
 */
#ifndef __WATCHDOG_UART_H__
#define __WATCHDOG_UART_H__

#include <stdint.h>

// Forward definition of opaque UART structure, defined in source file
// Other modules using this structure will not and do not need to know its contents
typedef struct UART__State UART__State;

typedef struct UART__Buffers
{
    volatile uint8_t* txBuffer;
    size_t txBufferSize; // Must be a power of two

    volatile uint8_t* rxBuffer;
    size_t rxBufferSize; // Must be a power of two
} UART__Buffers;

typedef struct UART__Config
{
    UART__Buffers uart0Buffers;
    UART__Buffers uart1Buffers;
} UART__Config;

typedef enum UART__Status {
    UART__STATUS__SUCCESS = 0,
    UART__STATUS__ERROR_NULL = -1,
    UART__STATUS__ERROR_ALREADY_INITIALIZED = -2,
    UART__STATUS__ERROR_NOT_INITIALIZED = -3,
    UART__STATUS__ERROR_ZERO_LENGTH_DATA = -4,
    UART__STATUS__ERROR_NOT_ENOUGH_SPACE = -5,

    UART__STATUS__ERROR_RB_PUT_FAILURE = -10,
    UART__STATUS__ERROR_RB_GET_FAILURE = -11,
    UART__STATUS__ERROR_RB_INIT_FAILURE = -12
} UART__Status;

UART__Status UART__init(UART__Config* config,
                        UART__State** uart0State,
                        UART__State** uart1State);

UART__Status UART__transmit(UART__State* uartState, const uint8_t* data, size_t dataLen);

UART__Status UART__receive(UART__State* uartState,
                           uint8_t* data,
                           size_t dataLen,
                           size_t* numReceived);

#endif /* __WATCHDOG_UART_H__ */
