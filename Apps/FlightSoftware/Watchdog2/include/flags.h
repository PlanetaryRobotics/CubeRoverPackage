/**
 * @file flags.h
 * @brief File outlining the flags that are used to communicate with the main loop
 */
#ifndef __FLAGS_INC
#define __FLAGS_INC

#include <stdint.h>

#define FLAG_UART0_RX_PACKET 0x1
#define FLAG_UART1_RX_PACKET 0x2
#define FLAG_I2C_RX_PACKET   0x4
#define FLAG_TIMER_TICK      0x8
#define FLAG_TEMP_LOW        0x10
#define FLAG_TEMP_HIGH       0x20
#define FLAG_POWER_ISSUE     0x40

__volatile extern uint16_t loop_flags;

#define WDFLAG_RADIO_KICK 0x1
#define WDFLAG_ADC_READY 0x2
#define WDFLAG_UNRESET_RADIO1 0x4
#define WDFLAG_UNRESET_RADIO2 0x8
#define WDFLAG_UNRESET_HERCULES 0x10
#define WDFLAG_UNRESET_MOTOR1 0x20
#define WDFLAG_UNRESET_MOTOR2 0x40
#define WDFLAG_UNRESET_MOTOR3 0x80
#define WDFLAG_UNRESET_MOTOR4 0x100
#define WDFLAG_UNRESET_FPGA   0x200
#define WDFLAG_UNRESET_3V3    0x400
#define WDFLAG_UNRESET_24V    0x800
#define WDFLAG_HERCULES_KICK  0x1000


#define DEFAULT_LPM LPM2_bits

__volatile extern uint16_t watchdog_flags;

enum rover_state {
    RS_SLEEP, // about to lose power
    RS_KEEPALIVE, // connected to the lander 28V, send heartbeats
    RS_SERVICE, // temporary state to act as a buffer between mission and service
    RS_MISSION, // power everything up. woot!
    RS_FAULT // oops
};
extern enum rover_state rovstate;

#endif /* __FLAGS_INC */
