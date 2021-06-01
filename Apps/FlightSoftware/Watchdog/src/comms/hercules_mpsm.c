
#include "include/comms/hercules_mpsm.h"
#include "include/common.h"
#include "include/utils/ring_buffer.h"


//###########################################################
// Private types
//###########################################################

typedef enum HerculesMpsm__HeaderContentsByteIndices
{
    HEADER_INDEX__MAGIC_ONE = 0,
    HEADER_INDEX__MAGIC_TWO = 1,
    HEADER_INDEX__MAGIC_THREE = 2,
    HEADER_INDEX__PARITY = 3,
    HEADER_INDEX__DATA_LEN_LSB = 4,
    HEADER_INDEX__DATA_LEN_MSB = 5,
    HEADER_INDEX__ONLY_USED_FOR_PARITY_ONE = 6,
    HEADER_INDEX__ONLY_USED_FOR_PARITY_TWO = 7
} HerculesMpsm__HeaderContentsByteIndices;

typedef enum HerculesMpsm__State
{
    HERCULES_MPSM_STATE__VALIDATE_HEADER = 0,
    HERCULES_MPSM_STATE__DATA = 1
} HerculesMpsm__State;

typedef struct HerculesMpsm__StateMachine
{
    BOOL initialized;

    HerculesMpsm__State currentState;

    // This one doesn't need to be volatile, but it's what RingBuffer takes.
    // This is a bit inefficient but hopefully won't have any real impact.
    // If it does have a performance impact we'll need to make something
    // custom for this use-case.
    volatile uint8_t ringBufferMemory[8];
    RingBuffer* headerRb;

    uint16_t dataLength;
} HerculesMpsm__StateMachine;

//###########################################################
// Private globals and constants
//###########################################################

static const uint8_t MAGIC_BYTE_ONE_EXPECTED_VALUE = 0x0BU;
static const uint8_t MAGIC_BYTE_TWO_EXPECTED_VALUE = 0xB0U;
static const uint8_t MAGIC_BYTE_THREE_EXPECTED_VALUE = 0x21U;
static const uint8_t MAGIC_BYTE_EXPECTED_VALUE_SUM = 0xDCU;

HerculesMpsm__StateMachine theStateMachine = {
    .initialized = FALSE,
    .currentState = HERCULES_MPSM_STATE__VALIDATE_HEADER,
    .ringBufferMemory = { 0 },
    .headerRb = NULL
    .dataLength = 0
};

//###########################################################
// Private function declarations
//###########################################################

HerculesMpsm__Status HerculesMpsm__checkForValidHeader(HerculesMpsm__Msg* msg, uint8_t newData);
BOOL HerculesMpsm__checkRb(size_t index, uint8_t expected);
uint8_t HerculesMpsm__peekRb(size_t index);
HerculesMpsm__Status HerculesMpsm__appendData(HerculesMpsm__Msg* msg, uint8_t newData);

//###########################################################
// Public function definitions
//###########################################################

HerculesMpsm__Status HerculesMpsm__initMsg(HerculesMpsm__Msg* msg)
{
    if (NULL == msg || NULL == msg->dataBuffer) {
        return HERCULES_MPSM__STATUS__ERROR_NULL;
    }

    if (!(theStateMachine.initialized)) {
        RingBuffer__Status rbStatus = RingBuffer__init(theStateMachine.headerRb,
                                                       theStateMachine.ringBufferMemory,
                                                       SIZE_OF_ARRAY(theStateMachine.ringBufferMemory));

        if (RB__STATUS__SUCCESS != rbStatus) {
            return HERCULES_MPSM__STATUS__ERROR_RB_INIT_FAILURE;
        }

        theStateMachine.initialized = TRUE;
    }

    if (0 == msg->dataBufferLen) {
        return HERCULES_MPSM__STATUS__ERROR_ZERO_LENGTH_BUFFER;
    }

    msg->msgStatus = HERCULES_MPSM__MSG_STATUS__IN_PROGRESS;
    msg->msgLen = 0;

    memset(msg->header, 0, SIZE_OF_ARRAY(msg->header));

    return HERCULES_MPSM__STATUS__SUCCESS;
}

HerculesMpsm__Status HerculesMpsm__process(HerculesMpsm__Msg* msg, uint8_t newData)
{
    if (NULL == msg || NULL == msg->dataBuffer) {
        return HERCULES_MPSM__STATUS__ERROR_NULL;
    }

    if (HERCULES_MPSM__MSG_STATUS__IN_PROGRESS != msg->msgStatus) {
        return HERCULES_MPSM__STATUS__ERROR_WRONG_STATE;
    }

    switch (theStateMachine.currentState) {
        case HERCULES_MPSM_STATE__VALIDATE_HEADER:
            return HerculesMpsm__checkForValidHeader(msg, newData);

        case HERCULES_MPSM_STATE__DATA:
            return HerculesMpsm__appendData(msg, newData);

        default:
            // This is bad and shouldn't ever happen
            return HERCULES_MPSM__STATUS__ERROR_INTERNAL_ERROR;
    }
}

//###########################################################
// Private function definitions
//###########################################################

HerculesMpsm__Status HerculesMpsm__checkForValidHeader(HerculesMpsm__Msg* msg, uint8_t newData)
{
    RingBuffer__Status rbStatus = RingBuffer__putOverwrite(theStateMachine.headerRb, newData);

    if (RB__STATUS__SUCCESS != rbStatus) {
        msg->msgStatus = HERCULES_MPSM__MSG_STATUS__ERROR_RB_FAILURE;
        return HERCULES_MPSM__STATUS__ERROR_RB_PUT_FAILURE;
    }

    // The header is 8 bytes, so the ring buffer (which has a size of 8) needs to be full for the
    // header to be valid.
    if (!RingBuffer__full(theStateMachine.headerRb)) {
        return HERCULES_MPSM__STATUS__NEED_MORE_DATA;
    }

    // Start checking the header bytes
    if (HerculesMpsm__checkRb((uint8_t) HEADER_INDEX__MAGIC_ONE, MAGIC_BYTE_ONE_EXPECTED_VALUE)
        && HerculesMpsm__checkRb((uint8_t) HEADER_INDEX__MAGIC_TWO, MAGIC_BYTE_TWO_EXPECTED_VALUE)
        && HerculesMpsm__checkRb((uint8_t) HEADER_INDEX__MAGIC_THREE, MAGIC_BYTE_THREE_EXPECTED_VALUE)) {
        // The magic numbers are correct, now check parity
        uint8_t expectedParity = HerculesMpsm__peekRb(HEADER_INDEX__PARITY);

        uint8_t dataLenLsb = HerculesMpsm__peekRb(HEADER_INDEX__DATA_LEN_LSB);
        uint8_t dataLenMsb = HerculesMpsm__peekRb(HEADER_INDEX__DATA_LEN_MSB);

        uint8_t computedParity = MAGIC_BYTE_EXPECTED_VALUE_SUM;
        computedParity += dataLenLsb + dataLenMsb;
        computedParity += HerculesMpsm__peekRb(HEADER_INDEX__ONLY_USED_FOR_PARITY_ONE);
        computedParity += HerculesMpsm__peekRb(HEADER_INDEX__ONLY_USED_FOR_PARITY_TWO);
        computedParity = ~computedParity;

        if (expectedParity == computedParity) {
            // The parity is correct, so this is a valid header. Record the data length and then advance
            // the state of the state machine so that we record future input as data bytes rather than
            // header bytes
            theStateMachine.dataLength = (uint16_t) (((uint16_t) dataLenLsb) & 0x00FFU);
            theStateMachine.dataLength |= (uint16_t) ((((uint16_t) dataLenMsb) << 8) & 0xFF00U);
            theStateMachine.currentState = HERCULES_MPSM_STATE__DATA;

            // We can check now if the buffer given by the user is big enough for our data
            if (theStateMachine.dataLength >= msg->dataBufferLen) {
                rbstatus = RingBuffer__clear(theStateMachine.headerRb);

                // Clear should only throw is headerRb is NULL, and it shouldn't be NULL
                assert(RB__STATUS__SUCCESS == rbstatus);

                msg->msgStatus = HERCULES_MPSM__MSG_STATUS__ERROR_BUFFER_TOO_SMALL;
                return HERCULES_MPSM__STATUS__ERROR_BUFFER_TOO_SMALL;
            }

            // Copy the header into the msg buffer
            for (size_t i = 0; i < SIZE_OF_ARRAY(msg->header); ++i) {
                rbStatus = RingBuffer__getOverwrite(theStateMachine.headerRb, msg->header + i);

                if (RB__STATUS__SUCCESS != rbStatus) {
                    rbstatus = RingBuffer__clear(theStateMachine.headerRb);

                    // Clear should only throw is headerRb is NULL, and it shouldn't be NULL
                    assert(RB__STATUS__SUCCESS == rbstatus);

                    msg->msgStatus = HERCULES_MPSM__MSG_STATUS__ERROR_RB_FAILURE;
                    return HERCULES_MPSM__STATUS__ERROR_RB_GET_FAILURE;
                }
            }
        }
    }

    return HERCULES_MPSM__STATUS__NEED_MORE_DATA;
}

BOOL HerculesMpsm__checkRb(size_t index, uint8_t expected)
{
    uint8_t value = 0;

    RingBuffer__Status rbStatus = RingBuffer__peekAt(theStateMachine.headerRb, index, &value);

    if (RB__STATUS__SUCCESS != rbStatus) {
        return FALSE;
    } else {
        return (value == expected);
    }
}

uint8_t HerculesMpsm__peekRb(size_t index)
{
    uint8_t value = 0;

    RingBuffer__Status rbStatus = RingBuffer__peekAt(theStateMachine.headerRb, index, &value);

    if (RB__STATUS__SUCCESS != rbStatus) {
        return 0;
    } else {
        return value;
    }
}


HerculesMpsm__Status HerculesMpsm__appendData(HerculesMpsm__Msg* msg, uint8_t newData);
{
    msg->dataBuffer[msg->msgLen] = newData;
    msg->msgLen++;

    // Check if we've finished reading the data portion, and if so reset
    if (msg->msgLen == theStateMachine.dataLength) {
        // Reset the state machine
        theStateMachine.currentState = HERCULES_MPSM_STATE__VALIDATE_HEADER;
        theStateMachine.dataLength = 0;

        msg->msgStatus = HERCULES_MPSM__MSG_STATUS__DONE_VALID;
        return HERCULES_MPSM__STATUS__PARSED_MESSAGE;
    } else {
        return HERCULES_MPSM__STATUS__NEED_MORE_DATA;
    }
}