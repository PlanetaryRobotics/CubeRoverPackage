#ifndef __WATCHDOG_HERCULES_MPSM_H__
#define __WATCHDOG_HERCULES_MPSM_H__

//Note: MPSM stands for "Message parsing state machine"

typedef enum HerculesMpsm__Status {
    HERCULES_MPSM__STATUS__NEED_MORE_DATA = 2, /*!< The function was successful, but needs more data to parse a message. */
    HERCULES_MPSM__STATUS__PARSED_MESSAGE = 1, /*!< The function was successful, and successfully parsed a complete message. */
    HERCULES_MPSM__STATUS__SUCCESS = 0, /*!< The function was successful. */
    HERCULES_MPSM__STATUS__ERROR_NULL = -1, /*!< A required argument was NULL */
    HERCULES_MPSM__STATUS__ERROR_BUFFER_TOO_SMALL = -2,
    HERCULES_MPSM__STATUS__ERROR_INVALID_SEQUENCE = -3,

    HERCULES_MPSM__STATUS__ERROR_RB_INIT_FAILURE = -10,
    HERCULES_MPSM__STATUS__ERROR_RB_PUT_FAILURE = -11,
    HERCULES_MPSM__STATUS__ERROR_RB_PEEK_FAILURE = -12,
    HERCULES_MPSM__STATUS__ERROR_RB_GET_FAILURE = -13,

    HERCULES_MPSM__STATUS__ERROR_INTERNAL_ERROR = -255 /*!< An unexpected error occurred */
} HerculesMpsm__Status;

typedef enum HerculesMpsm__MsgStatus
{
    /**
     * The state machine is done parsing the message, and it is valid.
     */
    HERCULES_MPSM__MSG_STATUS__DONE_VALID = 2,

    /**
     * This message structure has been initialized, but the message hasn't been fully received and/or parsed yet.
     */
    HERCULES_MPSM__MSG_STATUS__IN_PROGRESS = 1,

    /**
     * This message structure hasn't yet been initialized.
     */
    HERCULES_MPSM__MSG_STATUS__NOT_INITIALIZED = 0,

    /**
     * The state machine is done parsing the message because the buffer was too small for the message
     */
    HERCULES_MPSM__MSG_STATUS__ERROR_BUFFER_TOO_SMALL = HERCULES_MPSM__STATUS__ERROR_BUFFER_TOO_SMALL,

    /**
     * The state machine is done parsing the message because an unexpected failure occurred with a RingBuffer function
     */
    HERCULES_MPSM__MSG_STATUS__ERROR_RB_FAILURE = -10
}

struct HerculesMpsm__Msg
{
    HerculesMpsm__MsgStatus msgStatus;

    uint8_t header[8];

    uint8_t* dataBuffer;
    size_t dataBufferLen;

    size_t msgLen;
}

// Should be called before the first time process() is called with a given Msg structure, and before process()
// is called again with the same Msg structure after a complete message is received.
HerculesMpsm__Status HerculesMpsm__initMsg(HerculesMpsm__Msg* msg);

HerculesMpsm__Status HerculesMpsm__process(HerculesMpsm__Msg* msg, uint8_t newData);



#endif /* __WATCHDOG_HERCULES_MPSM_H__ */