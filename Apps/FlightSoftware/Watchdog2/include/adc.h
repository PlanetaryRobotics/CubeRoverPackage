/**
 * @file adc.h
 * @brief File for interfacing with ADC hardware module
 * File for interfacing with Analog-to-Digital (ADC) hardware module.
 * Used to monitor power levels
 */
#ifndef __ADC_INC
#define __ADC_INC

#include "buffer.h"

extern volatile unsigned short adc_values[3];

/**
 * @brief Initialize ADC hardware.
 * Sets up the interrupts and whatnot for ADC.
 */
void adc_init();

void adc_setup_lander();
void adc_setup_mission();
void adc_check_voltage_levels();

/**
 * @brief Take one ADC sample
 */
void adc_sample();

#define ADC_2V5_LEVEL_IDX 0
#define ADC_2V8_LEVEL_IDX 1
#define ADC_BATT_LEVEL_IDX 2
#define ADC_LANDER_LEVEL_IDX 0
#define ADC_TEMP_IDX 1


#endif /* __ADC_INC */
