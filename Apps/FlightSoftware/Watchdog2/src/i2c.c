#include "include/i2c.h"

// TODO: integrate this code with the rest of the system

int8_t raw_battery_charge[2];
int8_t raw_battery_voltage[2];
int8_t raw_battery_current[2];
int8_t raw_fuel_gauge_temp[2];

uint8_t batt_charge_telem;
uint8_t batt_curr_telem;

uint8_t fuel_gauge_write_control_reg;
uint8_t fuel_gauge_read_control_reg;

/*
 * File for interfacing with I2C protocol hardware module
 *
 * I2C communication uses eUSCI_B0 (power circuitry)
 */

/* Used to track the state of the software state machine*/
I2C_Mode MasterMode = IDLE_MODE;

/* The Register Address/Command to use*/
uint8_t TransmitRegAddr = 0;

// initialize buffers and their corresponding byte counters
uint8_t ReceiveBuffer[I2C_RX_BUFFER_MAX_SIZE] = {0};
uint8_t RXByteCtr = 0;
uint8_t ReceiveIndex = 0;
uint8_t TransmitBuffer[I2C_TX_BUFFER_MAX_SIZE] = {0};
uint8_t TXByteCtr = 0;
uint8_t TransmitIndex = 0;

/* uart0 rx handler (will only be called after SLIP packet fully received) */
void (*i2c_rx_handler)(uint16_t len, struct buffer *buf) = (void *)(0);


/* init function */
void i2c_init() {
    // Configure i2c interface
    P1SEL1 |= BIT6; // P1.6 SDA
    P1SEL1 |= BIT7; // P1.7 SCL

    // pins for using eusci_b2 (only ones available on launchpad)
//    P7SEL0 |= BIT0 | BIT1;
//    P7SEL1 &= ~(BIT0 | BIT1);

    UCB0CTLW0 = UCSWRST;                      // Enable SW reset
    UCB0CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK | UCSYNC; // I2C master mode, SMCLK
    UCB0BRW = 160;                            // fSCL = SMCLK/160 = ~100kHz
    UCB0I2CSA = I2C_SLAVE_ADDR;                   // Slave Address
    UCB0CTLW0 &= ~UCSWRST;                    // Clear SW reset, resume operation
    UCB0IE |= UCNACKIE;
}



/* For slave device with dev_addr, read the data specified in slaves reg_addr.
 * The received data is available in ReceiveBuffer
 *
 * dev_addr: The slave device address.
 *           Example: I2C_SLAVE_ADDR
 * reg_addr: The register or command to send to the slave.
 *           Example: CMD_TYPE_0_SLAVE
 * count: The length of data to read
 *           Example: TYPE_0_LENGTH
 *  */
I2C_Mode I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t count)
{
    /* Initialize state machine */
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;
    RXByteCtr = count;
    TXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

    /* Initialize slave address and interrupts */
    UCB0I2CSA = dev_addr;
    UCB0IFG &= ~(UCTXIFG + UCRXIFG);       // Clear any pending interrupts
    UCB0IE &= ~UCRXIE;                       // Disable RX interrupt
    UCB0IE |= UCTXIE;                        // Enable TX interrupt

    UCB0CTLW0 |= UCTR + UCTXSTT;             // I2C TX, start condition

    __bis_SR_register(GIE);              // Enable interrupts
    __delay_cycles(10000);               // give fuel gauge time to respond
    return MasterMode;

}

/* For slave device with dev_addr, writes the data specified in *reg_data
 *
 * dev_addr: The slave device address.
 *           Example: I2C_SLAVE_ADDR
 * reg_addr: The register or command to send to the slave.
 *           Example: CMD_TYPE_0_MASTER
 * *reg_data: The buffer to write
 *           Example: MasterType0
 * count: The length of *reg_data
 *           Example: TYPE_0_LENGTH
 *  */
I2C_Mode I2C_Master_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count)
{
    /* Initialize state machine */
    MasterMode = TX_REG_ADDRESS_MODE;
    TransmitRegAddr = reg_addr;

    //Copy register data to TransmitBuffer
    CopyArray(reg_data, TransmitBuffer, count);

    TXByteCtr = count;
    RXByteCtr = 0;
    ReceiveIndex = 0;
    TransmitIndex = 0;

    /* Initialize slave address and interrupts */
    UCB0I2CSA = dev_addr;
    UCB0IFG &= ~(UCTXIFG + UCRXIFG);       // Clear any pending interrupts
    UCB0IE &= ~UCRXIE;                       // Disable RX interrupt
    UCB0IE |= UCTXIE;                        // Enable TX interrupt

    UCB0CTLW0 |= UCTR + UCTXSTT;             // I2C TX, start condition

    __bis_SR_register(GIE);              // Enable interrupts
    __delay_cycles(10000);               // give fuel gauge some time to respond
    return MasterMode;
}

void CopyArray(uint8_t *source, uint8_t *dest, uint8_t count)
{
    uint8_t copyIndex = 0;
    for (copyIndex = 0; copyIndex < count; copyIndex++)
    {
        dest[copyIndex] = source[copyIndex];
    }
}

void updateGaugeReadings(){
    // record new measurements in fuel gauge
    readBatteryCharge();
    readBatteryVoltage();
    readBatteryCurrent();
    readGaugeTemp(); //TODO: probably don't need this one
}

void fuelGaugeLowPower(){
    // shut off all analog parts of fuel gauge circuit by setting LSB of control register to 1
    // set 2 MSB to 00 to put in sleep mode
    fuel_gauge_write_control_reg = 0b00101001;
    I2C_Master_WriteReg(I2C_SLAVE_ADDR, CONTROL, &fuel_gauge_write_control_reg, 1);
}

void readBatteryCharge(){
    I2C_Master_ReadReg(I2C_SLAVE_ADDR, ACCUMULATED_CHARGE_LSB, I2C_RX_BUFFER_MAX_SIZE);
    CopyArray((uint8_t*)ReceiveBuffer, (uint8_t*)&raw_battery_charge[1], I2C_RX_BUFFER_MAX_SIZE);
    I2C_Master_ReadReg(I2C_SLAVE_ADDR, ACCUMULATED_CHARGE_MSB, I2C_RX_BUFFER_MAX_SIZE);
    CopyArray((uint8_t*)ReceiveBuffer, (uint8_t*)&raw_battery_charge, I2C_RX_BUFFER_MAX_SIZE);

    // scale battery charge to fill most of 7 bit range available for telemetry
    batt_charge_telem = (uint8_t)( (uint16_t)(raw_battery_charge[1] + (raw_battery_charge[0] << 8)) >> 10) * 3;

}

void readBatteryVoltage(){
    I2C_Master_ReadReg(I2C_SLAVE_ADDR, VOLTAGE_LSB, 1);
    CopyArray((uint8_t*)ReceiveBuffer, (uint8_t*)&raw_battery_voltage[1], 1);
    I2C_Master_ReadReg(I2C_SLAVE_ADDR, VOLTAGE_MSB, 1);
    CopyArray((uint8_t*)ReceiveBuffer, (uint8_t*)&raw_battery_voltage, 1);
}

void readBatteryCurrent(){
    I2C_Master_ReadReg(I2C_SLAVE_ADDR, CURRENT_LSB, I2C_RX_BUFFER_MAX_SIZE);
    CopyArray((uint8_t*)ReceiveBuffer, (uint8_t*)&raw_battery_current[1], I2C_RX_BUFFER_MAX_SIZE);
    I2C_Master_ReadReg(I2C_SLAVE_ADDR, CURRENT_MSB, I2C_RX_BUFFER_MAX_SIZE);
    CopyArray((uint8_t*)ReceiveBuffer, (uint8_t*)&raw_battery_current, I2C_RX_BUFFER_MAX_SIZE);

    // scale current reading to maximize 7 bits allocated for current telemetry
    uint16_t BCurr_tmp = (uint16_t)(32767 - raw_battery_current[1] - (raw_battery_current[0] << 8));
    if(BCurr_tmp > 17407) {
        //exceeds maximum value of 0.6 A
        batt_curr_telem = 255;
    } else {
        batt_curr_telem = (uint8_t)( BCurr_tmp >> 7 );
    }
}


void readGaugeTemp(){
    I2C_Master_ReadReg(I2C_SLAVE_ADDR, TEMPERATURE_LSB, I2C_RX_BUFFER_MAX_SIZE);
    CopyArray((uint8_t*)ReceiveBuffer, (uint8_t*)&raw_fuel_gauge_temp[1], I2C_RX_BUFFER_MAX_SIZE);
    I2C_Master_ReadReg(I2C_SLAVE_ADDR, TEMPERATURE_MSB, I2C_RX_BUFFER_MAX_SIZE);
    CopyArray((uint8_t*)ReceiveBuffer, (uint8_t*)&raw_fuel_gauge_temp, I2C_RX_BUFFER_MAX_SIZE);
}

void readFuelGaugeControlRegister(){
    I2C_Master_ReadReg(I2C_SLAVE_ADDR, CONTROL, 1);
    fuel_gauge_read_control_reg = ReceiveBuffer[0];
}

void initializeFuelGauge(){

    // initialize charge register with maximum battery capacity (see data sheet for conversion from 3500 mAh, M is 1048)
    uint8_t init_tx_buffer = 0xA0;
    I2C_Master_WriteReg(I2C_SLAVE_ADDR, ACCUMULATED_CHARGE_MSB, &init_tx_buffer, I2C_TX_BUFFER_MAX_SIZE);
    init_tx_buffer = 0xD8;
    I2C_Master_WriteReg(I2C_SLAVE_ADDR, ACCUMULATED_CHARGE_LSB, &init_tx_buffer, I2C_TX_BUFFER_MAX_SIZE);


    // set ADC to read voltage/curr/temp once and then wait for next measurement request
    fuel_gauge_write_control_reg = 0b10101000;
    // set control_reg[7:6] to 01 do one conversion, 10 to convert every 10s,
    //      set to 00 to sleep, set to 11 to continuously convert
    // set control_reg[5:3] to 101 for M of 1024 for coulomb counter (see datasheet)
    // control_ref[2:1] not used on SBC (pin its related to is floating)
    // must leave control_reg[0] to 0
    I2C_Master_WriteReg(I2C_SLAVE_ADDR, CONTROL, &fuel_gauge_write_control_reg, 1);
}

//******************************************************************************
// I2C Interrupt ***************************************************************
//******************************************************************************

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCI_B0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  //Must read from UCB0RXBUF
  uint8_t rx_val = 0;
  switch(__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG))
  {
    case USCI_NONE:          break;         // Vector 0: No interrupts
    case USCI_I2C_UCALIFG:   break;         // Vector 2: ALIFG
    case USCI_I2C_UCNACKIFG:                // Vector 4: NACKIFG
      break;
    case USCI_I2C_UCSTTIFG:  break;         // Vector 6: STTIFG
    case USCI_I2C_UCSTPIFG:  break;         // Vector 8: STPIFG
    case USCI_I2C_UCRXIFG3:  break;         // Vector 10: RXIFG3
    case USCI_I2C_UCTXIFG3:  break;         // Vector 12: TXIFG3
    case USCI_I2C_UCRXIFG2:  break;         // Vector 14: RXIFG2
    case USCI_I2C_UCTXIFG2:  break;         // Vector 16: TXIFG2
    case USCI_I2C_UCRXIFG1:  break;         // Vector 18: RXIFG1
    case USCI_I2C_UCTXIFG1:  break;         // Vector 20: TXIFG1
    case USCI_I2C_UCRXIFG0:                 // Vector 22: RXIFG0
        rx_val = UCB0RXBUF;
        if (RXByteCtr)
        {
          ReceiveBuffer[ReceiveIndex++] = rx_val;
          RXByteCtr--;
        }

        if (RXByteCtr == 1)
        {
          UCB0CTLW0 |= UCTXSTP;
        }
        else if (RXByteCtr == 0)
        {
          UCB0IE &= ~UCRXIE;
          MasterMode = IDLE_MODE;
//          __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
        }
        break;
    case USCI_I2C_UCTXIFG0:                 // Vector 24: TXIFG0
        switch (MasterMode)
        {
          case TX_REG_ADDRESS_MODE:
              UCB0TXBUF = TransmitRegAddr;
              if (RXByteCtr)
                  MasterMode = SWITCH_TO_RX_MODE;   // Need to start receiving now
              else
                  MasterMode = TX_DATA_MODE;        // Continue to transmision with the data in Transmit Buffer
              break;

          case SWITCH_TO_RX_MODE:
              UCB0IE |= UCRXIE;              // Enable RX interrupt
              UCB0IE &= ~UCTXIE;             // Disable TX interrupt
              UCB0CTLW0 &= ~UCTR;            // Switch to receiver
              MasterMode = RX_DATA_MODE;    // State state is to receive data
              UCB0CTLW0 |= UCTXSTT;          // Send repeated start
              if (RXByteCtr == 1)
              {
                  //Must send stop since this is the N-1 byte
                  while((UCB0CTLW0 & UCTXSTT));
                  UCB0CTLW0 |= UCTXSTP;      // Send stop condition
              }
              break;

          case TX_DATA_MODE:
              if (TXByteCtr)
              {
                  UCB0TXBUF = TransmitBuffer[TransmitIndex++];
                  TXByteCtr--;
              }
              else
              {
                  //Done with transmission
                  UCB0CTLW0 |= UCTXSTP;     // Send stop condition
                  MasterMode = IDLE_MODE;
                  UCB0IE &= ~UCTXIE;                       // disable TX interrupt
//                  __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
              }
              break;

          default:
              __no_operation();
              break;
        }
        break;
    default: break;
  }
}
