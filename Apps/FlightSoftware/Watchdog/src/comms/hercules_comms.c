
#include "include/comms/hercules_comms.h"
#include "include/common.h"
#include "include/comms/hercules_mpsm.h"
#include "include/comms/ip_udp.h"

//###########################################################
// Private types
//###########################################################

struct HerculesComms__State
{
    BOOL initialized;
    UART__State* uartState;

    // Note: this size constant is defined in common.h
    uint8_t rxMsgBuffer[HERCULES_COMMS__MAX_RX_DATA_SIZE];
    HerculesMpsm__Msg herculesMsgMpsm;

    uint16_t txPacketId;
};

//###########################################################
// Private globals and constants
//###########################################################

HerculesComms__State theState = {
    .initialized = FALSE,
    .uartState = NULL,
    .rxMsgBuffer = { 0 },
    .herculesMsgMpsm = { 0 },
    .txPacketId = 0;
}

//###########################################################
// Private function declarations
//###########################################################

HerculesComms__Status HerculesComms__transmitBuffer(HerculesComms__State* hState, const uint8_t* buffer, size_t len);

//###########################################################
// Public function definitions
//###########################################################

HerculesComms__Status HerculesComms__init(HerculesComms__State** hState, UART__State* uartState)
{
    if (NULL == hState || NULL == uartState) {
        return HERCULES_COMMS__STATUS__ERROR_NULL;
    }

    if (theState.initialized) {
        return HERCULES_COMMS__STATUS__ERROR_ALREADY_INITIALIZED;
    }

    theState.uartState = uartState;

    theState.herculesMsgMpsm.dataBuffer = theState.rxMsgBuffer;
    theState.herculesMsgMpsm.dataBufferLen = SIZE_OF_ARRAY(theState.rxMsgBuffer);

    HerculesMpsm__Status mpsmStatus = HerculesMpsm__initMsg(&(theState.herculesMsgMpsm));

    if (HERCULES_MPSM__STATUS__SUCCESS != mpsmStatus) {
        return HERCULES_COMMS__STATUS__ERROR_MPSM_INIT_FAILURE;
    }

    theState.initialized = TRUE;
    *hState = &theState;

    return HERCULES_COMMS__STATUS__SUCCESS;
}

HerculesComms__Status HerculesComms__tryGetMessage(HerculesComms__State* hState,
                                                   BOOL* gotMessage,
                                                   uint8_t* buffer,
                                                   size_t bufferLen,
                                                   size_t* rxDataLen)
{
    static uint8_t uartRxData[64] = { 0 };

    if (NULL == hState || NULL == gotMessage || NULL == buffer || NULL == rxDataLen) {
        return HERCULES_COMMS__STATUS__ERROR_NULL;
    }

    if (!hState->initialized) {
        return HERCULES_COMMS__STATUS__ERROR_NOT_INITIALIZED;
    }

    // Make sure to initialize gotMessage to false
    *gotMessage = FALSE;

    HerculesComms__Status returnStatus = HERCULES_COMMS__STATUS__SUCCESS;
    BOOL tryToGetMoreData = TRUE;

    while (tryToGetMoreData) {
        // Zero out the static buffer on each iteration for easier debugging
        memset(uartRxData, 0, SIZE_OF_ARRAY(uartRxData));

        size_t numReceived = 0;

        UART__Status uartStatus = UART__receive(hState->uartState,
                                                uartRxData,
                                                SIZE_OF_ARRAY(uartRxData),
                                                &numReceived);

        if (UART__STATUS__SUCCESS != uartStatus) {
            return HERCULES_COMMS__STATUS__ERROR_UART_RX_FAILURE;
        }

        // Iterate through all data, adding it to the hercules mpsm until packet data has been found or
        // we use up all of the data from the UART
        for (size_t i = 0; i < numReceived; ++i) {
            BOOL resetMpsmMsg = FALSE;
            HerculesMpsm__Status mpsmStatus = HerculesMpsm__process(&(hState->herculesMsgMpsm));

            if (HERCULES_MPSM__STATUS__PARSED_MESSAGE == mpsmStatus) {
                // If we've already gotten a message in this call and we've now parsed ANOTHER one, we need to
                // indicate this happened to the caller via the return status. All but the first message will
                // be discarded.
                if (gotMessage) {
                    returnStatus = HERCULES_COMMS__STATUS__ERROR_MORE_THAN_ONE_MSG_RECEIVED;
                } else {
                    // We've gotten our data. Make sure received data can fit into buffer
                    if (hState->herculesMsgMpsm.msgLen > bufferLen) {
                        return HERCULES_COMMS__STATUS__ERROR_BUFFER_TOO_SMALL;
                    }

                    // Copy the data into the output buffer and update the size in the output parameter
                    memcpy(buffer, hState->herculesMsgMpsm.dataBuffer, hState->herculesMsgMpsm.msgLen);
                    *rxDataLen = hState->herculesMsgMpsm.msgLen;
                    gotMessage = TRUE;
                }

                resetMpsmMsg = TRUE;
            } else if (HERCULES_MPSM__STATUS__NEED_MORE_DATA != mpsmStatus) {
                // Some kind of unexpected error occurred. At a minimum we need to reset the MPSM
                // TODO: Additional handling? Logging?
                resetMpsmMsg = true;
            }

            if (resetMpsmMsg) {
                mpsmStatus = HerculesMpsm__initMsg(&(hState->herculesMsgMpsm));

                if (HERCULES_MPSM__STATUS__SUCCESS != mpsmStatus) {
                    // Don't overwrite an existing error return status with this one, but if we haven't had an
                    // error before now then set our return status to indicate this failure.
                    if (HERCULES_COMMS__STATUS__SUCCESS == returnStatus) {
                        returnStatus = HERCULES_COMMS__STATUS__ERROR_MPSM_INIT_FAILURE;
                    }

                    // If we weren't able to reset the MPSM we shouldn't try to process any more data
                    return returnStatus;
                }
            }
        }

        // Call receive again if our buffer for getting data from the uart was saturated with data in the last call.
        tryToGetMoreData = (numReceived == SIZE_OF_ARRAY(uartRxData));
    }

    return returnStatus;
}

HerculesComms__Status HerculesComms__txData(HerculesComms__State* hState, const uint8_t* data, size_t dataLen)
{
    // Directly insert the data into the UART tx ring buffer with no modification
    hStatus = HerculesComms__transmitBuffer(hState, data, dataLen);

    if (HERCULES_COMMS__STATUS__SUCCESS != hStatus) {
        return hStatus;
    }

    return HERCULES_COMMS__STATUS__SUCCESS;
}

//###########################################################
// Private function definitions
//###########################################################

HerculesComms__Status HerculesComms__transmitBuffer(HerculesComms__State* hState, const uint8_t* buffer, size_t len)
{
    UART__Status uartStatus = UART__transmit(hState->uartState, buffer, len);

    if (UART__STATUS__SUCCESS == uartStatus) {
        return HERCULES_COMMS__STATUS__SUCCESS;
    } else if (UART__STATUS__ERROR_NOT_ENOUGH_SPACE == uartStatus) {
        return HERCULES_COMMS__STATUS__ERROR_TX_OVERFLOW;
    } else {
        return HERCULES_COMMS__STATUS__ERROR_UART_TX_FAILURE;
    }
}