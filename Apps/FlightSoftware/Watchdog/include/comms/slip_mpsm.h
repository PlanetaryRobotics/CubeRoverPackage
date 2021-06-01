#ifndef __WATCHDOG_SLIP_MPSM_H__
#define __WATCHDOG_SLIP_MPSM_H__

#include "include/comms/ip_udp.h"

//Note: MPSM stands for "Message parsing state machine"

typedef enum SlipMpsm__Status {
    SLIP_MPSM__STATUS__NEED_MORE_DATA = 2, /*!< The function was successful, but needs more data to parse a message. */
    SLIP_MPSM__STATUS__PARSED_MESSAGE = 1, /*!< The function was successful, and successfully parsed a complete message. */
    SLIP_MPSM__STATUS__SUCCESS = 0, /*!< The function was successful. */
    SLIP_MPSM__STATUS__ERROR_NULL = -1, /*!< A required argument was NULL */
    SLIP_MPSM__STATUS__ERROR_BUFFER_TOO_SMALL = -2, /*!< A required argument was NULL */
    SLIP_MPSM__STATUS__ERROR_INVALID_SEQUENCE = -3, /*!< A required argument was NULL */
    SLIP_MPSM__STATUS__ERROR_INTERNAL_ERROR = -255 /*!< An unexpected error occurred */
} SlipMpsm__Status;

typedef enum SlipMpsm__MsgStatus
{
    /**
     * The state machine is done parsing the message, and it is valid.
     */
    SLIP_MPSM__MSG_STATUS__DONE_VALID = 2,

    /**
     * This message structure has been initialized, but the message hasn't been fully received and/or parsed yet.
     */
    SLIP_MPSM__MSG_STATUS__IN_PROGRESS = 1,

    /**
     * This message structure hasn't yet been initialized.
     */
    SLIP_MPSM__MSG_STATUS__NOT_INITIALIZED = 0,

    /**
     * The state machine is done parsing the message because the buffer was too small for the message
     */
    SLIP_MPSM__MSG_STATUS__ERROR_BUFFER_TOO_SMALL = SLIP_MPSM__STATUS__ERROR_BUFFER_TOO_SMALL,

    /**
     * The state machine is done parsing the message because an invalid byte sequence was encountered.
     *
     * For SLIP, the only invalid sequences are an ESC byte followed by something other than ESC_END or ESC_ESC.
     */
    SLIP_MPSM__MSG_STATUS__ERROR_INVALID_SEQUENCE = SLIP_MPSM__STATUS__ERROR_INVALID_SEQUENCE
}

struct SlipMpsm__Msg
{
    SlipMpsm__MsgStatus msgStatus;

    uint8_t* buffer;
    size_t bufferLen;

    size_t msgLen;
}

// Should be called before the first time process() is called with a given Msg structure, and before process()
// is called again with the same Msg structure after a complete message is received.
SlipMpsm__Status SlipMpsm__initMsg(SlipMpsm__Msg* msg);

SlipMpsm__Status SlipMpsm__process(SlipMpsm__Msg* msg, uint8_t newData);



#endif /* __WATCHDOG_SLIP_MPSM_H__ */