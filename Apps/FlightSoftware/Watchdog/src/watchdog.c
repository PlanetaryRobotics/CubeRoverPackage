/**
 * This file contains the code that does most of the watching of
 * the rest of the subsystems
 * <b>watchdog_monitor() should only ever be called in MISSION mode!</b>
 * There is no real watchdogging to do in lander connected mode.
 *
 * The way this file is laid out is that there are a number of ISRs. Some ISRs are taking in
 * kicks and setting a flag, and the other ISR is connected to a timer, which will check if the flags
 * are set, and clear them if they are.
 */

#include <msp430.h>
#include <stdint.h>
#include "include/flags.h"
#include "include/drivers/adc.h"
#include "include/drivers/uart.h"
#include "include/drivers/bsp.h"
#include "include/comms/ip_udp.h"
#include "include/comms/i2c_sensors.h"
#include "include/ground_cmd.h"
#include "include/watchdog.h"

volatile uint16_t watchdog_flags;

/**
 * Set up the ISRs for the watchdog
 */
int watchdog_init() {
    /* trigger on P1.0 Hi/lo edge */
    P1IE |= BIT0;                    // P1.0 interrupt enabled
    P1IES |= BIT0;                   // P1.0 Hi/lo edge
    P1IFG &= ~BIT0;                  // P1.0 IFG cleared initally

    /* timer setup */
    TA0CTL = TASSEL_1 | ID_0 | TAIE;           // ACLK/1, interrupt on overflow
    TA0EX0 = TAIDEX_0;                  // ??/1
    TA0CCR0 =  1000;                           // 12.5 Hz (pretty sure its not)
    TA0CTL |= MC_2;                 // Start timer in continuous mode

    // set up PWM for heater control
    TB0CCR0 = 10000;
    TB0CCTL2 = OUTMOD_7;
    TB0CTL = TBSSEL__SMCLK | MC__UP | TBCLR;
    PWM_limit = 8500;                       // make sure PWM does not exceed ~90% to keep power use low

    return 0;
}

/**
 * Perform the watchdog monitoring steps here
 *
 * Function called every ~5s
 */
int watchdog_monitor() {
    /* temporarily disable interrupts */
    __bic_SR_register(GIE);


    /* check that kicks have been received */
    if (watchdog_flags & WDFLAG_RADIO_KICK) {
        /* radio kick received, all ok! */
        watchdog_flags ^= WDFLAG_RADIO_KICK;
    } else {
        /* MISSING radio kick! don't bother resetting, though */
    }

    /* unreset wifi chip */
    if (watchdog_flags & WDFLAG_UNRESET_RADIO2) {
        releaseRadioReset();
        watchdog_flags ^= WDFLAG_UNRESET_RADIO2;
    }

    /* unreset wifi chip */
    if (watchdog_flags & WDFLAG_UNRESET_RADIO1) {
        watchdog_flags |= WDFLAG_UNRESET_RADIO2;
        watchdog_flags ^= WDFLAG_UNRESET_RADIO1;
    }

    /* unreset hercules */
    if (watchdog_flags & WDFLAG_UNRESET_HERCULES) {
        releaseHerculesReset();
        watchdog_flags ^= WDFLAG_UNRESET_HERCULES;
    }

    /* unreset motor 1 */
    if (watchdog_flags & WDFLAG_UNRESET_MOTOR1) {
        releaseMotor1Reset();
        watchdog_flags ^= WDFLAG_UNRESET_MOTOR1;
    }

    /* unreset motor 2 */
    if (watchdog_flags & WDFLAG_UNRESET_MOTOR2) {
        releaseMotor2Reset();
        watchdog_flags ^= WDFLAG_UNRESET_MOTOR2;
    }

    /* unreset motor 3 */
    if (watchdog_flags & WDFLAG_UNRESET_MOTOR3) {
        releaseMotor3Reset();
        watchdog_flags ^= WDFLAG_UNRESET_MOTOR3;
    }

    /* unreset motor 4 */
    if (watchdog_flags & WDFLAG_UNRESET_MOTOR4) {
        releaseMotor4Reset();
        watchdog_flags ^= WDFLAG_UNRESET_MOTOR4;
    }

    /* unreset FPGA */
    if (watchdog_flags & WDFLAG_UNRESET_FPGA) {
        releaseFPGAReset();
        watchdog_flags ^= WDFLAG_UNRESET_FPGA;
    }

    /* bring 3V3 on again */
    if (watchdog_flags & WDFLAG_UNRESET_3V3) {
        enable3V3PowerRail();
        watchdog_flags ^= WDFLAG_UNRESET_3V3;
    }

    /* turn 24V on again */
    if (watchdog_flags & WDFLAG_UNRESET_24V) {
        enable24VPowerRail();
        watchdog_flags ^= WDFLAG_UNRESET_24V;
    }

    /* check ADC values */
    if (watchdog_flags & WDFLAG_ADC_READY) {
        /* ensure ADC values are in spec */
        watchdog_flags ^= WDFLAG_ADC_READY;
    }

    /* check if hercules has given a valid kick */
    if (watchdog_flags & WDFLAG_HERCULES_KICK) {
        watchdog_flags ^= WDFLAG_HERCULES_KICK;
    } else {
        if (watchdog_opts & WDOPT_MONITOR_HERCULES) {
            // reset the hercules
            setHerculesReset();
            watchdog_flags |= WDFLAG_UNRESET_HERCULES;
            // if the issue was due to a comms breakdown, reset the comms state
            uart0_rx_mode = UA0_RX_HEADER;
        } else {
            // missed a kick, but don't reset the hercules.
        }
    }

    /* re-enable interrupts */
    __bis_SR_register(GIE);
    return 0;
}

void watchdog_send_hercules_telem(I2C_Sensors__Readings *i2cReadings) {
    // send the hercules its telemetry
    // Build telemetry packet for Hercules
    unsigned char telbuf[16] = {0};
    if (rovstate == RS_KEEPALIVE || rovstate == RS_SERVICE) {
        // no power on 2V5, 2V8, battery
        telbuf[6] = (uint8_t)(adc_values[ADC_LANDER_LEVEL_IDX]);
        telbuf[7] = (uint8_t)(adc_values[ADC_LANDER_LEVEL_IDX] >> 8);
        telbuf[8] = (uint8_t)(adc_values[ADC_TEMP_IDX] >> 4);
    } else if (rovstate == RS_MISSION) {
        telbuf[0] = (uint8_t)(adc_values[ADC_2V5_LEVEL_IDX]);
        telbuf[1] = (uint8_t)(adc_values[ADC_2V5_LEVEL_IDX] >> 8);
        telbuf[2] = (uint8_t)(adc_values[ADC_2V8_LEVEL_IDX]);
        telbuf[3] = (uint8_t)(adc_values[ADC_2V8_LEVEL_IDX] >> 8);
        telbuf[4] = (uint8_t)(adc_values[ADC_BATT_LEVEL_IDX]);
        telbuf[5] = (uint8_t)(adc_values[ADC_BATT_LEVEL_IDX] >> 8);
        // [6,7] won't be able to read lander power in mission mode
        telbuf[8] = (uint8_t)(adc_values[ADC_TEMP_IDX] >> 4);
        telbuf[9] = hasDeployed;
        telbuf[10] = (uint8_t)(i2cReadings->raw_battery_charge[0]);
        telbuf[11] = (uint8_t)(i2cReadings->raw_battery_charge[1]);
        telbuf[12] = (uint8_t)(i2cReadings->raw_battery_current[0]);
        telbuf[13] = (uint8_t)(i2cReadings->raw_battery_current[1]);
        telbuf[14] = (uint8_t)(i2cReadings->raw_battery_voltage[0]);
        telbuf[15] = (uint8_t)(i2cReadings->raw_battery_voltage[1]);
    }
    uart0_tx_nonblocking(16, telbuf);
}

/**
 * Function called whenever the watchdog needs to handle a byte from the hercules
 *
 * Assumes checksum valid.
 */
void watchdog_handle_hercules(I2C_Sensors__Readings *i2cReadings) {
    // valid hercules message received
    watchdog_flags |= WDFLAG_HERCULES_KICK;

    /* handle watchdog reset command */
    handle_watchdog_reset_cmd(uart0_rx_header[6]);

    /* deal with incoming udp packet from hercules */
    if (uart0_rx_len != 0) {
        /* add this packet to the IP/UDP stack send buffer */
        ipudp_send_packet(uart0rx.buf, uart0_rx_len); // @suppress("Invalid arguments")
    }

    /* send udp data back to the hercules if necessary */
    if (hercbuf.used > 0) {
        /* write out our buffer */
        /* update header length first */
        uart0_rx_header[4] = hercbuf.used & 0xFF;
        uart0_rx_header[5] = (hercbuf.used >> 8) & 0xFF;
    } else {
        /* update header length first */
        uart0_rx_header[4] = 0;
        uart0_rx_header[5] = 0;
    }

    /* recompute parity */
    uint8_t parity = 0xDC; /* sum of 0x21, 0xB0, and 0x0B */
    /* skip parity byte (i + 3) in summation */
    parity += uart0_rx_header[4] + uart0_rx_header[5];
    parity += uart0_rx_header[6] + uart0_rx_header[7];
    /* bitwise NOT to compute parity */
    parity = ~parity;
    /* write out new parity byte */
    uart0_rx_header[3] = parity;
    /* echo back watchdog command header updated version */
    uart0_tx_nonblocking(8, uart0_rx_header); // @suppress("Invalid arguments")

    /* payload of the header */
    if (hercbuf.used > 0) {
        /* send udp data */
        uart0_tx_nonblocking(hercbuf.used, hercbuf.buf); // @suppress("Invalid arguments")
        hercbuf.used = 0;
    } else {
        /* send telem */
        watchdog_send_hercules_telem(i2cReadings);
    }

    /* tell the interrupt handler to start processing bytes again */
    uart0_rx_mode = UA0_RX_HEADER;
}

void heaterControl(){
    // voltage, where LSB = 0.0008056640625V
    unsigned short therm_reading = adc_values[ADC_TEMP_IDX];

    if(therm_reading > heater_on_val){
        //   start heating when temperature drops below -5 C
        loop_flags |= FLAG_TEMP_LOW;
    } else if(therm_reading < heater_off_val){
        //   stop heating when temperature reaches 0 C
        loop_flags |= FLAG_TEMP_HIGH;
    }

    uint32_t PWM_cycle = 0;
    // P controller output
    // setpoint is slightly above desired temp because otherwise it will get stuck slightly below it
    if(loop_flags & FLAG_TEMP_LOW){
        PWM_cycle = Kp_heater * (therm_reading - heater_setpoint);
    }


    // cannot have duty cycle greater than clock
    if(PWM_cycle > PWM_limit){
        PWM_cycle = PWM_limit;
    }

    if(rovstate == RS_KEEPALIVE){
        TB0CCR2 = PWM_cycle;
    } else {
        // don't run heaters when not on lander
        TB0CCR2=0;
    }

}


/*
 * Pins that need an ISR:
 *
 * Power:
 * - P2.7 is 1V2 PowerGood
 * - P4.4 is 1V8 PowerGood
 * - P4.5 is 3V3 PowerGood (which PowerGood? Watchdog's or main systems'?)
 * - P4.7 is 5V0 PowerGood
 *
 * Kicks:
 * - P1.0 is Radio Kick
 * - P1.3 is Watchdog Int (Kick?)
 *
 * Things that might want an ISR:
 * - Watchdog timer itself
 *      it is currently disabled. Do we want to use it?
 * - Misc. timer
 *      in LPM, we want to wake up periodically to check power systems, etc.
 *      only; this can be done with a timer to wake every X seconds
 */

/* Port 1 handler */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt void port1_isr_handler(void) {
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT1_VECTOR))) port1_isr_handler (void) {
#else
#error Compiler not supported!
#endif
    switch(__even_in_range(P1IV, P1IV__P1IFG7)) {
        case P1IV__P1IFG0: // P1.0: Radio Kick
            watchdog_flags |= WDFLAG_RADIO_KICK;
            // exit LPM
            __bic_SR_register(DEFAULT_LPM);
            break;
        default: // default: ignore
            break;
    }
}

/* Port 3 handler */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT3_VECTOR
__interrupt void port3_isr_handler(void) {
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT3_VECTOR))) port3_isr_handler (void) {
#else
#error Compiler not supported!
#endif
    switch(__even_in_range(P3IV, P3IV__P3IFG7)) {
        default: // default: ignore
            break;
    }
}


/* Timer 1 handler */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_A1_VECTOR
__interrupt void Timer0_A1_ISR(void) {
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer0_A1_ISR (void) {
#else
#error Compiler not supported!
#endif
    switch (__even_in_range(TA0IV, TAIV__TAIFG)) {
        case TAIV__TAIFG: /* timer tick overflowed */
            loop_flags |= FLAG_TIMER_TICK;
            // exit LPM
            __bic_SR_register(DEFAULT_LPM);
            break;
        default: break;
    }
}

