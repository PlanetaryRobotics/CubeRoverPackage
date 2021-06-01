
#include "include/comms/slip_mpsm.h"
#include "include/comms/ip_udp.h"
#include "include/common.h"

//###########################################################
// Private types
//###########################################################

typedef enum SlipMpsm__State
{
    SLIP_MPSM_STATE__FIRST_END = 0,
    SLIP_MPSM_STATE__FIRST_BYTE_OR_STARTING_END = 1,
    SLIP_MPSM_STATE__STARTED = 2
} SlipMpsm__State;

typedef struct SlipMpsm__StateMachine
{
    SlipMpsm__State currentState;
    BOOL lastByteWasSlipEscape;
} SlipMpsm__StateMachine;

//###########################################################
// Private globals and constants
//###########################################################

SlipMpsm__StateMachine theStateMachine = {
    .currentState = SLIP_MPSM_STATE__FIRST_END,
    .lastByteWasSlipEscape = FALSE
};

//###########################################################
// Private function declarations
//###########################################################

SlipMpsm__Status SlipMpsm__appendData(SlipMpsm__Msg* msg, uint8_t newData);

//###########################################################
// Public function definitions
//###########################################################

SlipMpsm__Status SlipMpsm__initMsg(SlipMpsm__Msg* msg)
{
    if (NULL == msg || NULL == msg->buffer) {
        return SLIP_MPSM__STATUS__ERROR_NULL;
    }

    if (0 == msg->bufferLen) {
        return SLIP_MPSM__STATUS__ERROR_ZERO_LENGTH_BUFFER;
    }

    msg->msgStatus = SLIP_MPSM__MSG_STATUS__IN_PROGRESS;
    msg->msgLen = 0;

    return SLIP_MPSM__STATUS__SUCCESS;
}

SlipMpsm__Status SlipMpsm__process(SlipMpsm__Msg* msg, uint8_t newData)
{
    if (NULL == msg || NULL == msg->buffer) {
        return SLIP_MPSM__STATUS__ERROR_NULL;
    }

    if (SLIP_MPSM__MSG_STATUS__IN_PROGRESS != msg->msgStatus) {
        return SLIP_MPSM__STATUS__ERROR_WRONG_STATE;
    }

    switch (theStateMachine.currentState) {
        case SLIP_MPSM_STATE__FIRST_END:
            if (SLIP_END == newData) {
                // This byte is the very first end
                theStateMachine.currentState = SLIP_MPSM_STATE__FIRST_BYTE_OR_STARTING_END;
            } else {
                // Haven't yet found the first end, and this byte isn't it
            }

            return SLIP_MPSM__STATUS__NEED_MORE_DATA;

        case SLIP_MPSM_STATE__FIRST_BYTE_OR_STARTING_END:
            if (SLIP_END == newData) {
                // This byte is an "END" that is indicating the start of a message.
                // Don't need to do anything special here, as we transition the state
                // machine to the "STARTED" state unconditionally.
            } else {
                SlipMpsm__Status stat = SlipMpsm__appendData(msg, newData);

                if (SLIP_MPSM__STATUS__SUCCESS != stat) {
                    // Reset the state to look for the next message
                    theStateMachine.currentState = SLIP_MPSM_STATE__FIRST_END;
                    return stat;
                }
            }

            theStateMachine.currentState = SLIP_MPSM_STATE__STARTED;
            break;

        case SLIP_MPSM_STATE__STARTED:
            if (SLIP_END == newData) {
                // The current message is complete, so reset the state machine and update the msg
                theStateMachine.currentState = SLIP_MPSM_STATE__FIRST_BYTE_OR_STARTING_END;
                msg->msgStatus = SLIP_MPSM__MSG_STATUS__DONE_VALID;
                return SLIP_MPSM__STATUS__PARSED_MESSAGE;
            } else {
                SlipMpsm__Status stat = SlipMpsm__appendData(msg, newData);

                if (SLIP_MPSM__STATUS__SUCCESS != stat) {
                    // Reset the state to look for the next message
                    theStateMachine.currentState = SLIP_MPSM_STATE__FIRST_END;
                    return stat;
                }
            }
            break;

        default:
            // This is bad and shouldn't ever happen
            return SLIP_MPSM__STATUS__ERROR_INTERNAL_ERROR;
    }

    return SLIP_MPSM__STATUS__NEED_MORE_DATA;
}

//###########################################################
// Private function definitions
//###########################################################

SlipMpsm__Status SlipMpsm__appendData(SlipMpsm__Msg* msg, uint8_t newData);
{
    BOOL gotByteToWrite = FALSE;
    uint8_t byteToWrite = 0;

    if (theStateMachine.lastByteWasSlipEscape) {
        if (newData == SLIP_ESC_ESC) {
            byteToWrite = (uint8_t) SLIP_ESC;
        } else if (newData == SLIP_ESC_END) {
            byteToWrite = (uint8_t) SLIP_END;
        } else {
            msg->msgStatus = SLIP_MPSM__MSG_STATUS__ERROR_INVALID_SEQUENCE;
            return SLIP_MPSM__STATUS__ERROR_INVALID_SEQUENCE;
        }

        theStateMachine.lastByteWasSlipEscape = FALSE;
        gotByteToWrite = TRUE;
    } else if (SLIP_ESC == newData) {
        // Mark that the next byte was escaped, but don't add this escape byte to the output buffer
        theStateMachine.lastByteWasSlipEscape = TRUE;
    } else {
        // The new byte is an unescaped data byte
        gotByteToWrite = TRUE;
        byteToWrite = newData;
    }

    if (gotByteToWrite) {
        if (msg->msgLen + 1 > bufferLen) {
            msg->msgStatus = SLIP_MPSM__MSG_STATUS__ERROR_BUFFER_TOO_SMALL;
            return SLIP_MPSM__STATUS__ERROR_BUFFER_TOO_SMALL;
        }

        msg->buffer[msg->msgLen] = byteToWrite;
        msg->msgLen++;
    }

    return SLIP_MPSM__STATUS__SUCCESS;
}