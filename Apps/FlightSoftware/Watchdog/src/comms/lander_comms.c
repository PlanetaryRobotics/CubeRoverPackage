
#include "include/comms/lander_comms.h"
#include "include/common.h"
#include "include/comms/slip_mpsm.h"
#include "include/comms/ip_udp.h"

//###########################################################
// Private types
//###########################################################

struct LanderComms__State
{
    BOOL initialized;
    UART__State* uartState;

    // Note: this size constant is defined in common.h
    uint8_t slipMsgBuffer[LANDER_COMMS__MAX_RX_SLIP_MESSAGE_SIZE];
    SlipMpsm__Msg slipMsgMpsm;

    uint16_t txPacketId;
};

//###########################################################
// Private globals and constants
//###########################################################

LanderComms__State theState = {
    .initialized = FALSE,
    .uartState = NULL,
    .slipMsgBuffer = { 0 },
    .slipMsgMpsm = { 0 },
    .txPacketId = 0;
}

//###########################################################
// Private function declarations
//###########################################################

LanderComms__Status LanderComms__slipEncodeAndTransmitBuffer(LanderComms__State* lcState,
                                                             const uint8_t* inputBuffer,
                                                             size_t inputLen,
                                                             uint8_t* outputBuffer,
                                                             size_t outputBufferTotalSize,
                                                             size_t initialUsedOutputByteCount);

LanderComms__Status LanderComms__transmitBuffer(LanderComms__State* lcState, const uint8_t* buffer, size_t len);

//###########################################################
// Public function definitions
//###########################################################

LanderComms__Status LanderComms__init(LanderComms__State** lcState, UART__State* uartState)
{
    if (NULL == lcState || NULL == uartState) {
        return LANDER_COMMS__STATUS__ERROR_NULL;
    }

    if (theState.initialized) {
        return LANDER_COMMS__STATUS__ERROR_ALREADY_INITIALIZED;
    }

    theState.uartState = uartState;

    theState.slipMsgMpsm.buffer = theState.slipMsgBuffer;
    theState.slipMsgMpsm.bufferLen = SIZE_OF_ARRAY(theState.slipMsgBuffer);

    SlipMpsm__Status mpsmStatus = SlipMpsm__initMsg(&(theState.slipMsgMpsm));

    if (SLIP_MPSM__STATUS__SUCCESS != mpsmStatus) {
        return LANDER_COMMS__STATUS__ERROR_MPSM_INIT_FAILURE;
    }

    theState.initialized = TRUE;
    *lcState = &theState;

    return LANDER_COMMS__STATUS__SUCCESS;
}

// If we received a message, will return the data. The message is received as
// a SLIP-encoded full IP packet containing a UDP datagram that contains the message data.
// The data returned by this function is the message data inside all of that (i.e.
// the SLIP encoding is decoded and the contents of the IP packet are extracted).
LanderComms__Status LanderComms__tryGetMessage(LanderComms__State* lcState,
                                               BOOL* gotMessage,
                                               uint8_t* buffer,
                                               size_t bufferLen,
                                               size_t* rxDataLen)
{
    static uint8_t uartRxData[64] = { 0 };

    if (NULL == lcState || NULL == gotMessage || NULL == buffer || NULL == rxDataLen) {
        return LANDER_COMMS__STATUS__ERROR_NULL;
    }

    if (!lcState->initialized) {
        return LANDER_COMMS__STATUS__ERROR_NOT_INITIALIZED;
    }

    // Make sure to initialize gotMessage to false
    *gotMessage = FALSE;

    LanderComms__Status returnStatus = LANDER_COMMS__STATUS__SUCCESS;
    BOOL tryToGetMoreData = TRUE;

    while (tryToGetMoreData) {
        // Zero out the static buffer on each iteration for easier debugging
        memset(uartRxData, 0, SIZE_OF_ARRAY(uartRxData));

        size_t numReceived = 0;

        UART__Status uartStatus = UART__receive(lcState->uartState,
                                                uartRxData,
                                                SIZE_OF_ARRAY(uartRxData),
                                                &numReceived);

        if (UART__STATUS__SUCCESS != uartStatus) {
            return LANDER_COMMS__STATUS__ERROR_UART_RX_FAILURE;
        }

        // Iterate through all data, adding it to the SLIP mpsm until a full SLIP packet has been found or
        // we use up all of the data
        for (size_t i = 0; i < numReceived; ++i) {
            BOOL resetMpsmMsg = FALSE;
            SlipMpsm__Status mpsmStatus = SlipMpsm__process(&(lcState->slipMsgMpsm));

            if (SLIP_MPSM__STATUS__PARSED_MESSAGE == mpsmStatus) {
                // If we've already gotten a message in this call and we've now parsed ANOTHER one, we need to
                // indicate this happened to the caller via the return status. All but the first message will
                // be discarded.
                if (gotMessage) {
                    returnStatus = LANDER_COMMS__STATUS__ERROR_MORE_THAN_ONE_MSG_RECEIVED;
                } else {
                    // We've gotten a complete packet. Now we need to parse the data out of the UDP packet.
                    uint8_t* udpData = NULL;
                    size_t udpDataSize = 0;

                    IpUdp__Status ipStatus = IpUdp__identifyDataInUdpPacket(lcState->slipMsgMpsm.buffer,
                                                                            lcState->slipMsgMpsm.msgLen,
                                                                            &udpData,
                                                                            &udpDataSize);

                    if (IP_UDP__STATUS__SUCCESS == ipStatus) {
                        // Make sure received data can fit into buffer
                        if (udpDataSize > bufferLen) {
                            return LANDER_COMMS__STATUS__ERROR_BUFFER_TOO_SMALL;
                        }

                        // Copy the data into the output buffer and update the size in the output parameter
                        memcpy(buffer, udpData, udpDataSize);
                        *rxDataLen = udpDataSize;
                        gotMessage = TRUE;
                    }
                }

                resetMpsmMsg = TRUE;
            } else if (SLIP_MPSM__STATUS__NEED_MORE_DATA != mpsmStatus) {
                // Some kind of unexpected error occurred. At a minimum we need to reset the MPSM
                // TODO: Additional handling? Logging?
                resetMpsmMsg = true;
            }

            if (resetMpsmMsg) {
                mpsmStatus = SlipMpsm__initMsg(&(lcState->slipMsgMpsm));

                if (SLIP_MPSM__STATUS__SUCCESS != mpsmStatus) {
                    // Don't overwrite an existing error return status with this one, but if we haven't had an
                    // error before now then set our return status to indicate this failure.
                    if (LANDER_COMMS__STATUS__SUCCESS == returnStatus) {
                        returnStatus = LANDER_COMMS__STATUS__ERROR_MPSM_INIT_FAILURE;
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

// Will send data as contents of a UDP packet, SLIP encode, then send over UART
LanderComms__Status LanderComms__txData(LanderComms__State* lcState, const uint8_t* data, size_t dataLen)
{
    static const uint8_t SLIP_END_AS_UINT8 = (uint8_t) SLIP_END;

    // TODO: verify these buffer sizes are OK; we can decrease these, it just means serialization may get more complex
    static uint8_t uartHeaderData[IP_UDP_HEADER_LEN] = { 0 };
    static uint8_t uartTxSlipData[256] = { 0 };

    /*
     * We want to write four things to be transmitted via the UART to the lander:
     *   1) An initial SLIP_END to indicate message start
     *   2) The SLIP-encoded IP and UDP headers
     *   3) The SLIP-encoded UDP packet data
     *   4) A final SLIP_END to indicate message end
     */

    // Zero out the static buffers before use for easier debugging
    memset(uartHeaderData, 0, SIZE_OF_ARRAY(uartHeaderData));
    memset(uartTxSlipData, 0, SIZE_OF_ARRAY(uartTxSlipData));

    /* ======== 1) An initial SLIP_END to indicate message start ======== */

    // Manually insert the initial SLIP_END into the output buffer
    uartTxSlipData[0] = SLIP_END_AS_UINT8;

    /* ======== 2) The SLIP-encoded IP and UDP headers ======== */

    // First, get the serialized IP and UDP headers
    IpUdp__Status ipStatus = IpUdp__generateAndSerializeIpUdpHeadersForData(data,
                                                                            dataLen,
                                                                            uartHeaderData,
                                                                            SIZE_OF_ARRAY(uartHeaderData),
                                                                            lcState->txPacketId);

    // The above call failing indicates some kind of programmer error
    assert(IP_UDP__STATUS__SUCCESS == ipStatus);

    // Increment the packet ID for the next packet
    lcState->txPacketId++;

    // Now, encode the headers with SLIP encoding and transmit the result
    LanderComms__Status lcStatus = LanderComms__slipEncodeAndTransmitBuffer(lcState,
                                                                            uartHeaderData,
                                                                            SIZE_OF_ARRAY(uartHeaderData),
                                                                            uartTxSlipData,
                                                                            SIZE_OF_ARRAY(uartTxSlipData),
                                                                            sizeof(SLIP_END_AS_UINT8));

    if (LANDER_COMMS__STATUS__SUCCESS != lcStatus) {
        return lcStatus;
    }

    /* ======== 3) The SLIP-encoded UDP packet data ======== */

    // Now, encode the packet data with SLIP encoding and transmit the result
    lcStatus = LanderComms__slipEncodeAndTransmitBuffer(lcState,
                                                        data,
                                                        dataLen,
                                                        uartTxSlipData,
                                                        SIZE_OF_ARRAY(uartTxSlipData),
                                                        0);

    if (LANDER_COMMS__STATUS__SUCCESS != lcStatus) {
        return lcStatus;
    }

    /* ======== 4) A final SLIP_END to indicate message end ======== */

    // Directly insert the SLIP_END into the UART tx ring buffer as a single byte "array"
    lcStatus = LanderComms__transmitBuffer(lcState, &SLIP_END_AS_UINT8, sizeof(SLIP_END_AS_UINT8));

    if (LANDER_COMMS__STATUS__SUCCESS != lcStatus) {
        return lcStatus;
    }

    return LANDER_COMMS__STATUS__SUCCESS;
}

//###########################################################
// Private function definitions
//###########################################################

LanderComms__Status LanderComms__slipEncodeAndTransmitBuffer(LanderComms__State* lcState,
                                                             const uint8_t* inputBuffer,
                                                             size_t inputLen,
                                                             uint8_t* outputBuffer,
                                                             size_t outputBufferTotalSize,
                                                             size_t initialUsedOutputByteCount)
{
    const uint8_t* inPtr = inputBuffer;
    size_t remainingInBuffSize = intputLen;

    size_t outputStartIndex = initialUsedOutputByteCount;
    uint8_t* outPtr = outputBuffer + outputStartIndex;
    size_t remainingOutBuffSize = outputBufferTotalSize - outputStartIndex;

    BOOL doneWritingBuffer = FALSE;

    while (!doneWritingBuffer) {
        // These will be set by the encoding function based on how much it is able to write
        size_t outputStartOffset = 0;
        size_t inputStartOffset = 0;

        SlipEncode__Status encodeStatus = SlipEncode__encode(inPtr,
                                                             remainingInBuffSize,
                                                             &inputStartOffset,
                                                             outPtr,
                                                             remainingOutBuffSize,
                                                             &outputStartOffset);

        if (SLIP_ENCODE__STATUS__OUTPUT_FULL == encodeStatus) {
            // We also need to update the input buffer variables to reflect the data that we were able to
            // encode in this last call
            inPtr += inputStartOffset;
            remainingInBuffSize -= inputStartOffset;
        } else if (SLIP_ENCODE__STATUS__INPUT_FINISHED == encodeStatus) {
            // We've encoded everything in the buffer, so we don't need to loop anymore.
            doneWritingBuffer = TRUE;
        } else {
            return LANDER_COMMS__STATUS__ERROR_SLIP_ENCODE_FAILURE;
        }

        // We've filled the output buffer (either completely or partially, so invoke the UART transmit
        // function to insert the data into the send buffer.
        size_t dataLen = outputStartIndex + outputStartOffset;
        LanderComms__Status lcStatus = LanderComms__transmitBuffer(lcState, outputBuffer, dataLen);

        if (LANDER_COMMS__STATUS__SUCCESS != lcStatus) {
            return lcStatus;
        }

        // We've written everything in the output buffer to the UART tx ring buffer, so we can reset
        // the outputBuffer to empty
        outputStartIndex = 0;
        outPtr = outputBuffer;
        remainingOutBuffSize = outputBufferTotalSize;
    }

    return LANDER_COMMS__STATUS__SUCCESS;
}

LanderComms__Status LanderComms__transmitBuffer(LanderComms__State* lcState, const uint8_t* buffer, size_t len)
{
    UART__Status uartStatus = UART__transmit(lcState->uartState, buffer, len);

    if (UART__STATUS__SUCCESS == uartStatus) {
        return LANDER_COMMS__STATUS__SUCCESS;
    } else if (UART__STATUS__ERROR_NOT_ENOUGH_SPACE == uartStatus) {
        return LANDER_COMMS__STATUS__ERROR_TX_OVERFLOW;
    } else {
        return LANDER_COMMS__STATUS__ERROR_UART_TX_FAILURE;
    }
}