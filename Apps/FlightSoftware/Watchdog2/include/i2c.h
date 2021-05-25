/**
 * @file i2c.h
 * @brief File for interfacing with I2C protocol hardware module
 * File for interfacing with I2C protocol hardware module.
 */
#ifndef __I2C_INC
#define __I2C_INC

#include <msp430.h>
#include "buffer.h"

extern int8_t raw_battery_charge[2];
extern int8_t raw_battery_voltage[2];
extern int8_t raw_battery_current[2];
extern int8_t raw_fuel_gauge_temp[2];

extern uint8_t batt_charge_telem;
extern uint8_t batt_curr_telem;

#define I2C_SLAVE_ADDR      100 //0b1100100 //i2c address of LTC2944IDD#PBF fuel gauge

// every piece of i2c data for fuel gauge is 2 bytes (some info is 4 bytes split into MSB and LSB though)
#define I2C_RX_BUFFER_MAX_SIZE      1
#define I2C_TX_BUFFER_MAX_SIZE      1

#define I2C_MAX_DATA_SIZE           1

typedef enum I2cRegisterIds_Enum{

    STATUS = 0,                 //read only
    CONTROL = 1,
    ACCUMULATED_CHARGE_MSB = 2,
    ACCUMULATED_CHARGE_LSB = 3,
    CHARGE_THRESHOLD_HIGH_MSB = 4,
    CHARGE_THRESHOLD_HIGH_LSB = 5,
    CHARGE_THRESHOLD_LOW_MSB = 6,
    CHARGE_THRESHOLD_LOW_LSB = 7,
    VOLTAGE_MSB = 8,                //read only
    VOLTAGE_LSB = 9,                //read only
    VOLTAGE_THRESHOLD_HIGH_MSB = 10,
    VOLTAGE_THRESHOLD_HIGH_LSB = 11,
    VOLTAGE_THRESHOLD_LOW_MSB = 12,
    VOLTAGE_THRESHOLD_LOW_LSB = 13,
    CURRENT_MSB = 14,                //read only
    CURRENT_LSB = 15,                //read only
    CURRENT_THRESHOLD_HIGH_MSB = 16,
    CURRENT_THRESHOLD_HIGH_LSB = 17,
    CURRENT_THRESHOLD_LOW_MSB = 18,
    CURRENT_THRESHOLD_LOW_LSB = 19,
    TEMPERATURE_MSB = 20,           //read only
    TEMPERATURE_LSB = 21,           //read only
    TEMPERATURE_THRESHOLD_HIGH = 22,
    TEMPERATURE_THRESHOLD_LOW = 23,
    MAX_NB_CMDS = 24

}I2cRegisterIds;


typedef enum I2C_ModeEnum{
    IDLE_MODE,
    NACK_MODE,
    TX_REG_ADDRESS_MODE,
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    SWITCH_TO_RX_MODE,
    SWITHC_TO_TX_MODE,
    TIMEOUT_MODE
} I2C_Mode;


extern void (*i2c_rx_handler)(uint16_t len, struct buffer *buf);

I2C_Mode I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);
I2C_Mode I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t count);
void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count);

/*
 * Need to call updateGaugeReadings() to update the registers for voltage, current and temperature on
 *   the fuel gauge.
 * NOTE: conversion takes ~33ms, registers are updated at end of conversion
 */
void updateGaugeReadings();

/*
 *   won't update readings but current consumption is ~0.15uA
 *      power back on using updateGaugeReadings()
 */
void fuelGaugeLowPower();
void readBatteryCharge();
void readBatteryVoltage();
void readBatteryCurrent();
void readGaugeTemp();
void initializeFuelGauge();
void readFuelGaugeControlRegister();


/**
 * @brief Initialize I2C hardware.
 * Sets up the interrupts and whatnot for I2C.
 */
void i2c_init();

void i2c_tx_blocking(uint16_t len, unsigned char *buf);


#endif /* __I2C_INC */
