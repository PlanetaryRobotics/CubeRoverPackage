#ifndef WatchDogInterface_WatchDogMpsm_HPP
#define WatchDogInterface_WatchDogMpsm_HPP

#include <stdint.h>
#include <stddef.h>

#define WATCH_DOG_HEADER_PACKED_SIZE 12

namespace CubeRover
{
    /**
     * @brief A Message Parsing State Machine (MPSM) that facilitates receiving and parsing messages from the MSP430
     *        watchdog.
     *
     * This class is specifically tailored to integrate with a DMA-based approach for requesting data from the
     * underlying UART driver. Due to this tailoring, the implementation of this class ends up not actually being
     * much of a state machine. However, since I've consistently named classes with this function MPSM in other
     * places on the Iris project, I've chosen to keep the name here.
     */
    class WatchDogMpsm
    {
        public:
            /**
             * The status of parsing the header of a message.
             */
            enum ParseHeaderStatus 
            {
                PHS_NEED_MORE_DATA = 0, //!< Need more data to complete the header.
                PHS_PARSED_VALID_HEADER = 1, //!< Parsed a full header that passes its parity check.
                PHS_PARSED_HEADER_BAD_PARITY = 2, //!< Parsed a full header that failed its parity check.

                /**
                 * @brief An unexpected internal error occurred.
                 *
                 * I don't think should actually be possible to occur (it's only returned if we land
                 * in the default case of a switch statement, and that should only occur in the event
                 * of programmer error).
                 */
                PHS_INTERNAL_ERROR = -255
            };

            /**
             * The status of parsing the payload of a message.
             */
            enum ParseDataStatus 
            {
                PDS_NEED_MORE_DATA = 0, //!< Need more data to complete the payload.
                PDS_PARSED_ALL_DATA = 1 //!< Successfully got all payload data.
            };

            /**
             * @brief The unpacked contents of a message header from the MSP430 watchdog.
             */
            class Header
            {
                public:
                    // Magic number is excluded, because this will only be populated if the header had a valid magic number
                    uint8_t parity; //!< The header parity.
                    uint16_t payloadLength; //!< The payload length.
                    uint16_t resetValue; //!< The reset value.
                    uint16_t lowerSeqNum; //!< The lower 16 bits of the sequence number.
                    uint16_t lowerOpCode; //!< The lower 16 bits of the opcode.

                    /**
                     * @brief Constructor.
                     */
                    Header();

                    /**
                     * @brief Clears the contents of this header.
                     */
                    void reset();
            };

            /**
             * @brief An unpacked message received from the MSP430 watchdog.
             */
            class Message
            {
                public:
                    Header parsedHeader; //!< The unpacked header, once successfully received and parsed.

                    uint8_t* dataBuffer; //!< The buffer in which payload data will be stored.
                    size_t dataBufferLen; //!< The max size of `dataBuffer`.

                    /**
                     * The number of bytes in `dataBuffer` that actually contain payload data at this time.
                     */
                    size_t accumulatedDataSize;

                    /**
                     * @brief Constructor.
                     *
                     * @param buffer The buffer used to hold the payload of the message.
                     * @param bufferLen The maximum size of `buffer`.
                     */
                    explicit Message(uint8_t* buffer, size_t bufferLen);

                    /**
                     * @brief Clears the structure. Note that this does not clear `dataBuffer` or `dataBufferLen`.
                     *
                     * This does zero-out the contents of `dataBuffer`, but this message will continue to use
                     * the same buffer location even after being reset.
                     */
                    void reset();
            };

            /**
             * @brief Constructor.
             */
            WatchDogMpsm();

            /**
             * @brief Destructor.
             */
            ~WatchDogMpsm();

            /**
             * @brief Gets the status of receiving and parsing the header, and also the specifics of the next DMA
             *        transfer that should be done to fully receive the header.
             *
             * If PHS_NEED_MORE_DATA is returned, the next DMA transfer should be performed using the destination
             * and size returned with the corresponding return parameters.
             *
             * If PHS_PARSED_VALID_HEADER is returned, then `nextTransferSize` will be set to zero. If the payload
             * size field in the parsed header is non-zero, then `getDataDmaDetails()` should be called. Otherwise,
             * if the payload size field in the parsed header is zero, the contents of `msg` should be used until
             * we are done with them. Then, the `msg` object should be reset (by calling its `reset()` method) and
             * then this function should be called again.
             *
             * If PHS_PARSED_HEADER_BAD_PARITY is returned, then `nextTransferSize` should be non-zero. In this case,
             * `msg.parsedHeader` will contain the header that was parsed but failed its parity check. However,
             * `nextTransferSize` will not be zero. The contents of `msg` can be used as desired, and then we can
             * try to receive the next valid header by using the destination and size returned with the original call
             * (i.e., `msg` doesn't need to be reset, and this function doesn't need to be called again).
             *
             * @param msg The message struct whose header is being retrieved.
             * @param nextTransferDestination A return parameter that will be set to the next location to which data
             *                                should be written.
             * @param nextTransferSize A return parameter that will be set to the next size of data that should be
             *                         written.
             *
             * @return The status of parsing the header.
             */
            ParseHeaderStatus getHeaderDmaDetails(WatchDogMpsm::Message& msg,
                                                  uint8_t** nextTransferDestination,
                                                  unsigned& nextTranferSize);

            /**
             * @brief Notifies this object that the DMA transfer requested according to the return parameters of
             *        the last `getHeaderDmaDetails()` call has completed.
             *
             * @param msg The message struct whose header is being retrieved.
             * @param destination The destination returned by the previous call to `getHeaderDmaDetails()`.
             * @param size The size returned by the previous call to `getHeaderDmaDetails()`.
             */
            void notifyHeaderDmaComplete(WatchDogMpsm::Message& msg,
                                         const uint8_t* destination,
                                         unsigned size);

            /**
             * @brief Gets the status of receiving the payload, and also the specifics of the next DMA
             *        transfer that should be done to fully receive the payload.
             *
             * If PDS_NEED_MORE_DATA is returned, the next DMA transfer should be performed using the destination
             * and size returned with the corresponding return parameters.
             *
             * If PDS_PARSED_ALL_DATA is returned, then `size` will be set to zero. The contents of `msg` should be
             * used until we are done with them. Then, the `msg` object should be reset (by calling its `reset()`
             * method) and then `getHeaderDmaDetails()` should be called again.
             *
             * @param msg The message struct whose payload is being retrieved.
             * @param destination A return parameter that will be set to the next location to which data
             *                    should be written.
             * @param size A return parameter that will be set to the next size of data that should be written.
             *
             * @return The status of parsing the payload.
             */
            ParseDataStatus getDataDmaDetails(WatchDogMpsm::Message& msg,
                                              uint8_t** destination,
                                              unsigned& size);
            /**
             * @brief Notifies this object that the DMA transfer requested according to the return parameters of
             *        the last `getDataDmaDetails()` call has completed.
             *
             * @param msg The message struct whose payload is being retrieved.
             * @param size The size returned by the previous call to `getDataDmaDetails()`.
             */
            void notifyDataDmaComplete(WatchDogMpsm::Message& msg, unsigned size);

        private:
            struct PrivateImplementation;
            PrivateImplementation* m_impl;
    };
} // end namespace CubeRover

#endif // #ifndef WatchDogInterface_WatchDogMpsm_HPP
