/*
 * Code for interfacing with UART protocol hardware module.
 *
 * UART communication uses eUSCI_A0 (Hercules) and eUSCI_A1 (Lander).
 *
 * 9600 baud, 1 stop bit, no parity
 *
 * References: SLAU367P (https://www.ti.com/lit/ug/slau367p/slau367p.pdf)
 *
 * Author: mschnur
 */

#include <msp430.h>
#include "include/drivers/uart.h"
#include "include/utils/ring_buffer.h"
#include "include/common.h"
#include "include/flags.h"

//###########################################################
// Private types
//###########################################################

typedef struct UART__Registers
{
    uint16_t * const UCAxTXBUF;
    uint16_t * const UCAxRXBUF;
    uint16_t * const UCAxIV;
    uint16_t * const UCAxIE;
    uint16_t * const UCAxIFG;

} UART__Registers;

struct UART__State
{
    BOOL initialized;
    UART__Registers* registers;

    RingBuffer* txRingBuff;
    RingBuffer* rxRingBuff;
}

//###########################################################
// Private globals and constants
//###########################################################

static UART__Registers uart0Registers = {
    .UCAxTXBUF = &UCA0TXBUF,
    .UCAxRXBUF = &UCA0RXBUF,
    .UCAxIV = &UCA0IV,
    .UCAxIE = &UCA0IE,
    .UCAxIFG = &UCA0IFG
};

static UART__Registers uart1Registers = {
    .UCAxTXBUF = &UCA1TXBUF,
    .UCAxRXBUF = &UCA1RXBUF,
    .UCAxIV = &UCA1IV,
    .UCAxIE = &UCA1IE,
    .UCAxIFG = &UCA1IFG
};

static UART__State uart0State = {
    .initialized = FALSE,
    .registers = &uart0Registers;
    .txRingBuff = NULL,
    .rxRingBuff = NULL
};

static UART__State uart1State = {
    .initialized = FALSE,
    .registers = &uart1Registers;
    .txRingBuff = NULL,
    .rxRingBuff = NULL
};

//###########################################################
// Private function declarations
//###########################################################

void UART__clockInit(void);
UART__Status UART__initState(UART__State* state, UART__Buffers* buffers);
void UART__uart0Init(void);
void UART__uart1Init(void);

void UART__interruptHandler(UART__State* uartState);

//###########################################################
// Public function definitions
//###########################################################

UART__Status UART__init(UART__Config* config,
                        UART__State** uart0StateOut,
                        UART__State** uart1StateOut)
{
    if (NULL == config
        || NULL == uart0StateOut
        || NULL == uart1StateOut) {
        return UART__STATUS__ERROR_NULL;
    }

    if (uart0State.initialized && uart0State.initialized) {
        return UART__STATUS__ERROR_ALREADY_INITIALIZED;
    }

    // First, initialize the clocks. This could be before or after the state structures,
    // but should be before initializing the uarts themselves
    UART__clockInit();


    // Second, initialize the state structures for both uarts
    UART__Status uartStatus = UART__initState(&uart0State, &(config->uart0Buffers));

    if (UART__STATUS__SUCCESS != uartStatus) {
        return uartStatus;
    }

    uartStatus = UART__initState(&uart1State, &(config->uart1Buffers));

    if (UART__STATUS__SUCCESS != uartStatus) {
        return uartStatus;
    }

    // Third, initialize the uarts themselves
    UART__uart0Init();
    UART__uart1Init();

    *uart0StateOut = &uart0State;
    *uart1StateOut = &uart1State;

    return UART__STATUS__SUCCESS;
 }


UART__Status UART__transmit(UART__State* uartState, const uint8_t* data, size_t dataLen)
{
    if (NULL == uartState || NULL == data) {
        return UART__STATUS__ERROR_NULL;
    }

    if (!(uartState->initialized)) {
        return UART__STATUS__ERROR_NOT_INITIALIZED;
    }

    if (0 == dataLen) {
        return UART__STATUS__ERROR_ZERO_LENGTH_DATA;
    }

    size_t numFree = 0;

    // Disable the tx interrupt for this uart while we get the number of free bytes in the ring buffer
    uint16_t existingTxInterruptBitState = *(uartState->registers->UCAxIE) & UCTXIE;
    *(uartState->registers->UCAxIE) &= ~UCTXIE;

    numFree = RingBuffer__freeCount(uartState->txRingBuff);

    // Re-enable the tx interrupt only if it was previously enabled
    *(uartState->registers->UCAxIE) |= existingTxInterruptBitState;

    if (dataLen > numFree) {
        return UART__STATUS__ERROR_NOT_ENOUGH_SPACE;
    }

    // We have enough space in the ring buffer for all of our buffer data, so
    // push it all into the buffer
    for (size_t i = 0U; i < dataLen; ++i) {
        RingBuffer__Status rbStatus = RingBuffer__put(uartState->txRingBuff, data[i]);

        if (RB__STATUS__SUCCESS != rbStatus) {
            return UART__STATUS__ERROR_RB_PUT_FAILURE;
        }
    }

    // We've written data to the tx ring buffer, so make sure that the tx interrupt is enabled
    *(uartState->registers->UCAxIE) |= UCTXIE;

    return UART__STATUS__SUCCESS;
}

UART__Status UART__receive(UART__State* uartState,
                           uint8_t* data,
                           size_t dataLen,
                           size_t* numReceived)
{
    if (NULL == uartState || NULL == data || NULL == numReceived) {
        return UART__STATUS__ERROR_NULL;
    }

    if (!(uartState->initialized)) {
        return UART__STATUS__ERROR_NOT_INITIALIZED;
    }

    // Make sure numReceived starts at zero
    *numReceived = 0U;

    // We have enough space in the ring buffer for all of our buffer data, so
    // push it all into the buffer
    for (size_t i = 0U; i < dataLen; ++i) {
        RingBuffer__Status rbStatus = RingBuffer__get(uartState->rxRingBuff, data + i);

        if (RB__STATUS__SUCCESS == rbStatus) {
            // we got another byte
            (*numReceived)++;
        } else if (RB__STATUS__ERROR_EMPTY == rbStatus) {
            // Have gotten all bytes that have been received so far
            break;
        } else {
            return UART__STATUS__ERROR_RB_GET_FAILURE;
        }
    }

    return UART__STATUS__SUCCESS;
}

//###########################################################
// Private function definitions
//###########################################################

/**
 * Initialize clocks for UART. necessary and should only be called once, at boot.
 */
void UART__clockInit()
{
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;           // Set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers
    CSCTL0_H = 0;                           // Lock CS registers
}

UART__Status UART__initState(UART__State* state, UART__Buffers* buffers)
{
    if (NULL == state
        || NULL == buffers
        || NULL == buffers->txBuffer
        || NULL == buffers->rxBuffer) {
        return UART__STATUS__ERROR_NULL;
    }

    // Shouldn't be possible to only have one uart initialized but not the other,
    // but make sure we don't don't re-iniitalize one anyway
    if (!(state->initialized)) {
        RingBuffer__Status rbStatus = RingBuffer__init(&(state->txRingBuff),
                                                       buffers->txBuffer,
                                                       buffers->txBufferSize);

        if (RB__STATUS__SUCCESS != rbStatus) {
            return UART__STATUS__ERROR_RB_INIT_FAILURE;
        }

        rbStatus = RingBuffer__init(&(state->rxRingBuff),
                                    buffers->rxBuffer,
                                    buffers->rxBufferSize);

        if (RB__STATUS__SUCCESS != rbStatus) {
            return UART__STATUS__ERROR_RB_INIT_FAILURE;
        }

        state->initialized = TRUE;
    }

    return UART__STATUS__SUCCESS;
}

/**
 * Initialize UART0 (Hercules <-> watchdog)
 */
void UART__uart0Init()
{
    // Put eUSCI_A0 in reset
    UCA0CTLW0 = UCSWRST;

    /* Setup for eUSCI_A0 and eUSCI_A1 */
    /* On the MSP430FR5994, pin P2.0 is used for TX and pin P2.1 is used for RX
     * (ref: pg 92 of datasheet) */
    /* P2.0 TX: x = 0; P2SEL1.x = 1, P2SEL0.x = 0 */
    /* P2.1 RX: x = 1; P2SEL1.x = 1, P2SEL0.x = 0 */

    /* set P2SEL0.1, and P2SEL0.0 to 0 */
    P2SEL0 &= ~(BIT0 | BIT1);
    /* set P2SEL1.1, and P2SEL1.0 to 1 */
    P2SEL1 |= (BIT0 | BIT1);

    // Use SMCLK as BRCLK
    UCA0CTLW0 |= UCSSEL__SMCLK;

    /*
    By leaving all other fields as default in UCA0CTLW0, we are using the following configuration:
    - Parity disabled
    - LSB first (in the RX and TX shift registers)
    - 8-bit data
    - One stop bit
    - eUSCI_A0 in UART mode
    - Asynchronous mode
    - Regular UART mode (no multiprocessor mode or automatic baud-rate detection)
    - Erroneous characters rejected and corresponding interrupt disabled
    - Receive break character interrupt disabled
    - Not dormant
    - Next frame to be transmitted is data
    - Next frame to be transmitted is not a break
    */

    // Baud Rate calculation (Section 30.3.10, SLAU367P)
    // N = (Baud rate clock frequency) / baud rate = 8000000 / 9600 = 833.3333
    // N > 16, so we will use oversampling baud-rate generation mode (as TI recommends)
    UCA0MCTLW = UCOS16; // Enables oversampling baud-rate generation mode
    // UCBRx = int(N / 16) = int(52.08333)
    // UCBRx = 52
    UCA0BRW = 52U; // Note: UCBRSx takes up the full 16 bits of UCAxBRW
    // UCBRFx = int([(N/16) - int(N / 16)] * 16) = int([52.08333 - 52] * 16) = int(1.3333)
    // UCBRFx = 1
    UCA0MCTLW |= UCBRF_1;
    // UCBRSx = 0x04 (per Table 30-4, SLAU367P)
    // However, per table 30-5 (SLAU367P), for a BRCLK frequency of 8000000 and a baud rate of 9600,
    //  the USBRSx value of 0x49 results in the lowest error (as determined by a search algorithm).
    //  Therefore, we use 0x49 instead of 0x04.
    UCA0MCTLW |= 0x4900U;  // Note: UCBRSx is the top 8 bits of UCAxMCTLW

    // Release eUSCI_A0 reset
    UCA0CTLW0 &= ~UCSWRST;
    // Enable USCI_A0 RX interrupt
    UCA0IE |= UCRXIE;
}

/**
 * Initialize UART1 (Lander <-> watchdog)
 */
void UART__uart1Init() {
    uart1tx.idx = 0;
    uart1tx.used = 0;
    uart1tx.locked = 0;
    uart1rx.idx = 0;
    uart1rx.used = 0;

    // Put eUSCI_A1 in reset
    UCA1CTLW0 = UCSWRST;

    /* Setup for eUSCI_A1 */
    /* On the MSP430FR5994, pin P2.5 is used for TX and pin P2.6 is used for RX
     * (ref: pg 95 of datasheet) */
    /* P2.5 TX: x = 5; P2SEL1.x = 1, P2SEL0.x = 0 */
    /* P2.6 RX: x = 6; P2SEL1.x = 1, P2SEL0.x = 0 */

    /* set P2SEL0.5, P2SEL0.6 to 0 */
    P2SEL0 &= ~(BIT5 | BIT6);
    /* set P2SEL1.5, P2SEL1.6 to 1 */
    P2SEL1 |= (BIT5 | BIT6);

    // Use SMCLK as BRCLK
    UCA1CTLW0 |= UCSSEL__SMCLK;

    /*
    By leaving all other fields as default in UCA1CTLW0, we are using the following configuration:
    - Parity disabled
    - LSB first (in the RX and TX shift registers)
    - 8-bit data
    - One stop bit
    - eUSCI_A1 in UART mode
    - Asynchronous mode
    - Regular UART mode (no multiprocessor mode or automatic baud-rate detection)
    - Erroneous characters rejected and corresponding interrupt disabled
    - Receive break character interrupt disabled
    - Not dormant
    - Next frame to be transmitted is data
    - Next frame to be transmitted is not a break
    */

    // Baud Rate calculation (Section 30.3.10, SLAU367P)
    // N = (Baud rate clock frequency) / (baud rate) = 8000000 / 9600 = 833.3333
    // N > 16, so we will use oversampling baud-rate generation mode (as TI recommends)
    UCA1MCTLW = UCOS16; // Enables oversampling baud-rate generation mode
    // UCBRx = int(N / 16) = int(52.08333)
    // UCBRx = 52
    UCA1BRW = 52U; // Note: UCBRSx takes up the full 16 bits of UCAxBRW
    // UCBRFx = int([(N/16) - int(N / 16)] * 16) = int([52.08333 - 52] * 16) = int(1.3333)
    // UCBRFx = 1
    UCA1MCTLW |= UCBRF_1;
    // UCBRSx = 0x04 (per Table 30-4, SLAU367P)
    // However, per table 30-5 (SLAU367P), for a BRCLK frequency of 8000000 and a baud rate of 9600,
    //  the USBRSx value of 0x49 results in the lowest error (as determined by a search algorithm).
    //  Therefore, we use 0x49 instead of 0x04.
    UCA1MCTLW |= 0x4900U;  // Note: UCBRSx is the top 8 bits of UCAxMCTLW

    // Release eUSCI_A1 reset
    UCA1CTLW0 &= ~UCSWRST;
    // Enable USCI_A1 RX interrupt
    UCA1IE |= UCRXIE;
}

void UART__interruptHandler(UART__State* uartState)
{
    if (NULL == uartState) {
        return;
    }

    uint8_t data = 0U;
    RingBuffer__Status rbStatus = RB__STATUS__SUCCESS;

    /* two possibilities; rx or tx */
    switch(__even_in_range(*(uartState->registers->UCAxIV), USCI_UART_UCTXCPTIFG)) {
        case USCI_UART_UCTXIFG: /* transmitted byte successfully */
            rbStatus = RingBuffer__get(uartState->txRingBuff, &data);

            if (RB__STATUS__SUCCESS == rbStatus) {
                // We got another byte to send, so put it in the tx buffer
                *(uartState->registers->UCAxTXBUF) = data;
            } else if (RB__STATUS__ERROR_EMPTY == rbStatus) {
                // There are no more bytes to send, so disable TX interrupt for this uart
                *(uartState->registers->UCAxIE) &= ~UCTXIE;
            } else {
                // An error occurred.
                // TODO: handling? logging?
            }
            break;

        case USCI_UART_UCRXIFG: /* received new byte */
            /* get the received character */
            data = *(uartState->registers->UCAxRXBUF);

            // Note: calling this means that if the buffer is full and we get new data,
            // we'll drop the new data instead of overwriting the oldest data in the
            // buffer. If we'd rather overwrite the old data, this will need to be updated.
            rbStatus = RingBuffer__put(uartState->rxRingBuff, data);

            if (RB__STATUS__ERROR_FULL == rbStatus) {
                // The ring buffer is full, so data will be dropped.
                // TODO: handling? logging?
            } else if (RB__STATUS__SUCCESS != rbStatus) {
                // An error occurred.
                // TODO: handling? logging?
            }

            // We're done with the byte we just received, so clear the receive interrupt flag
            // for this uart
            *(uartState->registers->UCAxIFG) &= ~UCRXIFG;
            break;

        default: /* some other possibilities */
            break;
    }
}

//###########################################################
// Interrupt handlers
//###########################################################

/* uart0 rx interrupt handler (Hercules) */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
    #pragma vector=EUSCI_A0_VECTOR
    __interrupt void USCI_A0_ISR(void) {
#elif defined(__GNUC__)
    void __attribute__ ((interrupt(EUSCI_A0_VECTOR))) USCI_A0_ISR(void) {
#else
    #error Compiler not supported!
#endif

    UART__interruptHandler(&uart0State);
}


/* UCA1 interrupt handler (watchdog SLIP) */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
    #pragma vector=EUSCI_A1_VECTOR
    __interrupt void USCI_A1_ISR(void) {
#elif defined(__GNUC__)
    void __attribute__ ((interrupt(EUSCI_A1_VECTOR))) USCI_A1_ISR (void) {
#else
    #error Compiler not supported!
#endif

    UART__interruptHandler(&uart1State);
}
