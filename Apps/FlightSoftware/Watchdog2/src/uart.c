/*
 * Code for interfacing with UART protocol hardware module.
 *
 * UART communication uses eUSCI_A0 (Hercules) and eUSCI_A1 (Lander).
 *
 * 9600 baud, 1 stop bit, no parity
 */
#include <msp430.h>
#include "include/uart.h"
#include "include/flags.h"

/* end byte */
#define SLIP_END 0xC0
/* escape byte */
#define SLIP_ESC 0xDB
/* escaped end */
#define SLIP_ESC_END 0xDC
/* escaped escape */
#define SLIP_ESC_ESC 0xDD

/* ========================== interrupt handlers =========================== */

/* uart0 rx interrupt handler (Hercules) */
volatile uint8_t uart0_rx_mode = UA0_RX_HEADER;
volatile uint8_t uart0_rx_header[8] = {0};
volatile uint16_t uart0_rx_len = 0;
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=EUSCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void) {
#elif defined(__GNUC__)
void __attribute__ ((interrupt(EUSCI_A0_VECTOR))) USCI_A0_ISR (void) {
#else
#error Compiler not supported!
#endif
    unsigned char rcv;

    /* two possibilities; rx or tx */
    switch(__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG)) {
    case USCI_UART_UCTXIFG: /* transmitted byte successfully */
        /* decrement the number of bytes used */
        uart0tx.used--;

        if (uart0tx.used == 0) {
            /* done sending after this byte; clear IFG */
            UCA0IE &= ~UCTXIE;
        }

        /* send the next byte */
        UCA0TXBUF = uart0tx.buf[uart0tx.idx++];
        if (uart0tx.idx >= BUFFER_SIZE) uart0tx.idx = 0;
        break;
    case USCI_UART_UCRXIFG: /* received new byte */
        /* get the received character */
        rcv = UCA0RXBUF;

        /* check what mode we're in */
        // TODO: need a check to ensure we don't get stuck in UA0_RX_PROCESS_UDP forever
        if (uart0_rx_mode == UA0_RX_HEADER) {
            uart0_rx_header[7] = rcv;
            /* verify valid header w/ magic value and parity */
            uint8_t parity = 0xDC; /* sum of 0x21, 0xB0, and 0x0B */
            parity += uart0_rx_header[4] + uart0_rx_header[5];
            parity += uart0_rx_header[6] + uart0_rx_header[7];
            /* bitwise NOT to compute parity */
            parity = ~parity;
            if (uart0_rx_header[0] == 0x0B && uart0_rx_header[1] == 0xB0 && uart0_rx_header[2] == 0x21
                    && parity == uart0_rx_header[3]) {
                /* get the received packet length */
                uart0_rx_len = (uart0_rx_header[4]) | (uart0_rx_header[5] << 8);
                /* next byte to process is UDP data */
                uart0_rx_mode = UA0_RX_UDP;
                /* clear the rx buffer */
                uart0rx.idx = 0;
                uart0rx.used = 0;
                /* note if uart0_rx_len = 0, we will change to UA0_RX_PROCESS_UDP below */
            } else {
                /* no match, shuffle the bytes around and keep going */
                uart0_rx_header[0] = uart0_rx_header[1];
                uart0_rx_header[1] = uart0_rx_header[2];
                uart0_rx_header[2] = uart0_rx_header[3];
                uart0_rx_header[3] = uart0_rx_header[4];
                uart0_rx_header[4] = uart0_rx_header[5];
                uart0_rx_header[5] = uart0_rx_header[6];
                uart0_rx_header[6] = uart0_rx_header[7];
            }
        } else if (uart0_rx_mode == UA0_RX_UDP) {
            uart0rx.buf[uart0rx.idx++] = rcv;
            uart0rx.used++;

            if (uart0rx.idx >= BUFFER_SIZE) uart0rx.idx -= BUFFER_SIZE;
        } else {
            // TODO: need a check to ensure we don't get stuck in UA0_RX_PROCESS_UDP forever
            /* drop the byte */
        }

        /* check if we are done reading */
        if ((uart0_rx_mode == UA0_RX_UDP) && (uart0rx.used >= uart0_rx_len)) {
            /* note that we received a packet in main loop */
            loop_flags |= FLAG_UART0_RX_PACKET;
            /* also stop writing to buffer */
            uart0_rx_mode = UA0_RX_PROCESS_UDP;
        }

        /* clear UART_A0 receive flag */
        /* we are done here */
        UCA0IFG &= ~UCRXIFG;
        break;
    default: /* some other possibilities */
        break;
    }
}

__volatile int is_escaped = 0;
__volatile int has_started = 0;

/* UCA1 interrupt handler (watchdog SLIP) */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=EUSCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void) {
#elif defined(__GNUC__)
void __attribute__ ((interrupt(EUSCI_A1_VECTOR))) USCI_A1_ISR (void) {
#else
#error Compiler not supported!
#endif

    unsigned char rcv;

    // TODO: buffer wraparound seems weird right now.

    /* two possibilities; rx or tx */
    switch(__even_in_range(UCA1IV, USCI_UART_UCTXCPTIFG)) {
    case USCI_UART_UCTXIFG: /* transmitted byte successfully */
        /* decrement the number of bytes used */
        uart1tx.used--;

        if (uart1tx.used == 0) {
            /* done sending after this byte; clear IFG */
            UCA1IE &= ~UCTXIE;
            /* unlock the buffer */
            uart1tx.locked = 0;
        }

        /* send the next byte */
        UCA1TXBUF = uart1tx.buf[uart1tx.idx++];
        if (uart1tx.idx >= BUFFER_SIZE) uart1tx.idx = 0;
        break;
    case USCI_UART_UCRXIFG: /* received new byte */
        /* get the received character */
        rcv = UCA1RXBUF;

        if (!has_started) {
            if (rcv == SLIP_END) {
                /* start now */
                has_started = 1;
                goto end_rx;
            } else {
                goto end_rx;
            }
        }

        /* deal with escaped characters */
        if (is_escaped && rcv == SLIP_ESC_END) {
            /* push the escaped character */
            uart1rx.buf[uart1rx.idx++] = SLIP_END;
            is_escaped = 0;
            goto end_rx;
        } else if (is_escaped && rcv == SLIP_ESC_ESC) {
            /* push the escaped character */
            uart1rx.buf[uart1rx.idx++] = SLIP_ESC;
            is_escaped = 0;
            goto end_rx;
        }

        /* regular case (not escaped character) */
        switch (rcv) {
        case SLIP_END:
            /* done reading; skip storing the end byte, and signal to the main loop that we are done */
            loop_flags |= FLAG_UART1_RX_PACKET;
            has_started = 0;
            // exit LPM
            __bic_SR_register(DEFAULT_LPM);
            break;
        case SLIP_ESC:
            /* about to start escape sequence; skip storing this byte */
            is_escaped = 1;
            break;
        default:
            /* regular byte */
            uart1rx.buf[uart1rx.idx++] = rcv;
            break;
        }

        /* clear UART_A1 receive flag */
        /* we are done here */
    end_rx:
        UCA1IFG &= ~UCRXIFG;
        if (uart1rx.idx >= BUFFER_SIZE) {
            uart1rx.idx -= BUFFER_SIZE;
        }
        break;
    default: /* some other possibilities */
        break;
    }
}

/* =============================== main code ================================ */

/**
 * Initialize clocks for UART. necessary and should only be called once, at boot.
 */
void clock_init() {
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;           // Set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers
    CSCTL0_H = 0;                           // Lock CS registers
}

/**
 * Power-saving measure: disable UART0 and set pins to high impedance input
 */
void uart0_disable() {
    UCA0CTLW0 = UCSWRST;                    // Put eUSCI_A0 in reset
    //  TODO: fill this in
}

/**
 * Initialize UART0 (Hercules <-> watchdog)
 */
void uart0_init() {
    /* initally all buffers are empty */
    uart0tx.idx = 0;
    uart0tx.used = 0;
    uart0rx.idx = 0;
    uart0rx.used = 0;

    UCA0CTLW0 = UCSWRST;                    // Put eUSCI_A0 in reset

    /* Setup for eUSCI_A0 and eUSCI_A1 */
    /* On the MSP430FR5994, pin P2.0 is used for TX and pin P2.1 is used for RX
     * (ref: pg 92 of datasheet) */
    /* P2.0 TX: x = 0; P2SEL1.x = 1, P2SEL0.x = 0 */
    /* P2.1 RX: x = 1; P2SEL1.x = 1, P2SEL0.x = 0 */

    /* set P2SEL0.1, and P2SEL0.0 to 0 */
    P2SEL0 &= ~(BIT0 | BIT1);
    /* set P2SEL1.1, and P2SEL1.0 to 1 */
    P2SEL1 |= (BIT0 | BIT1);

    UCA0CTLW0 |= UCSSEL__SMCLK;             // CLK = SMCLK
    // Baud Rate calculation
    // 8000000/(16*9600) = 52.083
    // Fractional portion = 0.083
    // User's Guide Table 21-4: UCBRSx = 0x04
    // UCBRFx = int ( (52.083-52)*16) = 1
    UCA0BRW = 52;                           // 8000000/16/9600
    UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900; // ???
    UCA0CTLW0 &= ~UCSWRST;                  // Release eUSCI_A0 reset
    UCA0IE |= UCRXIE;                       // Enable USCI_A0 RX interrupt
}

/**
 * disable uart 1
 */
void uart1_disable() {
    UCA1CTLW0 = UCSWRST;                    // Put eUSCI_A1 in reset

    /* set P2SEL0.5, P2SEL0.6 to 0 */
    P2SEL0 &= ~(BIT5 | BIT6);
    /* set P2SEL1.5, P2SEL1.6 to 0 */
    P2SEL0 &= ~(BIT5 | BIT6);

    UCA1IE = 0;
    UCA1CTLW0 = 0; // clear setup
}

/**
 * Initialize UART1 (Lander <-> watchdog)
 */
void uart1_init() {
    uart1tx.idx = 0;
    uart1tx.used = 0;
    uart1tx.locked = 0;
    uart1rx.idx = 0;
    uart1rx.used = 0;

    UCA1CTLW0 = UCSWRST;                    // Put eUSCI_A1 in reset

    /* Setup for eUSCI_A1 */
    /* On the MSP430FR5994, pin P2.5 is used for TX and pin P2.6 is used for RX
     * (ref: pg 95 of datasheet) */
    /* P2.5 TX: x = 5; P2SEL1.x = 1, P2SEL0.x = 0 */
    /* P2.6 RX: x = 6; P2SEL1.x = 1, P2SEL0.x = 0 */

    /* set P2SEL0.5, P2SEL0.6 to 0 */
    P2SEL0 &= ~(BIT5 | BIT6);
    /* set P2SEL1.5, P2SEL1.6 to 1 */
    P2SEL1 |= (BIT5 | BIT6);

    UCA1CTLW0 |= UCSSEL__SMCLK;             // CLK = SMCLK
    // Baud Rate calculation
    // 8000000/(16*9600) = 52.083
    // Fractional portion = 0.083
    // User's Guide Table 21-4: UCBRSx = 0x04
    // UCBRFx = int ( (52.083-52)*16) = 1
    UCA1BRW = 52;                           // 8000000/16/9600
    UCA1MCTLW |= UCOS16 | UCBRF_1 | 0x4900; // ???
    UCA1CTLW0 &= ~UCSWRST;                  // Release eUSCI_A1 reset
    UCA1IE |= UCRXIE;                       // Enable USCI_A1 RX interrupt
}


void uart0_tx_nonblocking(uint16_t length, unsigned char *buffer) {
    uint16_t i=0;
    unsigned char b;
    uint16_t curr_idx = uart0tx.idx + uart0tx.used;

    for (i = 0; i < length; i++) {
        b = buffer[i];
        uart0tx.buf[curr_idx++] = b;
        if (curr_idx >= BUFFER_SIZE) curr_idx = 0;
        uart0tx.used++;
    }

    /* start interrupts for sending async */
    UCA0IE |= UCTXIE;
}

void uart1_tx_nonblocking(uint16_t length, unsigned char *buffer, uint8_t opts) {
    uint16_t i=0;
    unsigned char b;
    uint16_t curr_idx;

    // wait for the buffer to be unlocked
    while (uart1tx.locked) __delay_cycles(100);

    // disable interrupts to prevent race conditions
    UCA1IE &= ~UCTXIE;
    // disable interrupts
    __bic_SR_register(GIE);

    // get the current base index
    curr_idx = uart1tx.idx + uart1tx.used;

    // TODO: maybe we should do SLIP encoding in the interrupt handler instead,
    // so that we can have a deterministic MTU

    if (opts & UA1_ADD_PKT_START) {
        uart1tx.buf[curr_idx++] = SLIP_END;
        uart1tx.used += 1;
    }

    for (i = 0; i < length; i++) {
        b = buffer[i];
        /* check what byte this is */
        switch (b) {
        case SLIP_END:
            /* have to send 2 characters to escape END */
            uart1tx.buf[curr_idx++] = SLIP_ESC;
            if (curr_idx >= BUFFER_SIZE) curr_idx = 0;
            uart1tx.buf[curr_idx++] = SLIP_ESC_END;
            if (curr_idx >= BUFFER_SIZE) curr_idx = 0;
            uart1tx.used += 2;
            break;
        case SLIP_ESC:
            /* have to send 2 characters to escape ESC */
            uart1tx.buf[curr_idx++] = SLIP_ESC;
            if (curr_idx >= BUFFER_SIZE) curr_idx = 0;
            uart1tx.buf[curr_idx++] = SLIP_ESC_ESC;
            if (curr_idx >= BUFFER_SIZE) curr_idx = 0;
            uart1tx.used += 2;
            break;
        default:
            /* no need to escape or w/e */
            uart1tx.buf[curr_idx++] = b;
            if (curr_idx >= BUFFER_SIZE) curr_idx = 0;
            uart1tx.used++;
            break;
        }
    }

    if (opts & UA1_ADD_PKT_END) {
        uart1tx.buf[curr_idx++] = SLIP_END;
        uart1tx.used += 1;
    }

    /* start interrupts for sending async */
    uart1tx.locked = 1;
    __bis_SR_register(GIE);
    UCA1IE |= UCTXIE;
}


