#include "S25fl064l.hpp"

S25fl064l :: S25fl064l(){
  // External Flash SPI data configuration
  // Clock configuration need to be:
  // Clock polarity unchecked
  // Clock phase checked
  m_flashDataConfig.CS_HOLD = false;
  m_flashDataConfig.DFSEL = SPI_FMT_0;
  m_flashDataConfig.WDEL = false;
  m_flashDataConfig.CSNR = 0;

  // Refer to datasheet, default number of dummy cycles between a SDI and SDO
  // is set by default to 8 clock cycles
  m_readLatencyCycles = DEFAULT_DUMMY_CYCLES;

  m_flashSpi = (spiBASE_t *)NULL;

  m_memAllocPointer = 0;
  // default setting of the external memory
  m_addressLengthFormat = S25fl064l::ADDRESS_LENGTH_3_BYTES; 
}

// Destructor: DO NOT USE
S25fl064l :: ~S25fl064l(){}

/**
 * @brief      The fucntion sets up the flash memory component
 *
 * @return     The device error code
 */
S25fl064l::S25fl064lError S25fl064l :: setupDevice(){
  S25fl064lError err = S25FL064L_NO_ERROR;

  m_flashSpi = SPI_REG;

  // Check that the correct device is connected to the MCU
  uint8_t id[3];
  err = flashSpiReadData(S25fl064l::RDID, id, sizeof(id));

  if(err != S25FL064L_NO_ERROR)
      return err;

  // Check that the device is connected and correct
  if(id[0] != FLASH_MANUFACTURER_ID ||
     id[1] != FLASH_DEVICE_ID_MSB ||
     id[2] != FLASH_DEVICE_ID_LSB)
      return S25FL064L_INCORRECT_FLASH_MEMORY;

  // Check read latency cycles. Read latency cycles are requried for some read
  // operation on the device.
  S25fl064l::ConfigurationRegister3 configRegister3;
  err = flashSpiReadData(S25fl064l::RDCR3, &configRegister3.all, 1);
  if(err != S25FL064L_NO_ERROR){
    return err;
  }
  m_readLatencyCycles = configRegister3.bit.rl;

  return err;
}

   /**
    * @brief      Gets the address length byte.
    *
    * @param[in]  cmd   The command
    *
    * @return     The address length byte.
    */
uint16_t S25fl064l :: getAddressLengthByte(const S25fl064l::FlashSpiCommands cmd){
  switch(cmd){
    case S25fl064l::RDID:   // Read ID
    case S25fl064l::RDQID:   // Read Quad ID
    case S25fl064l::RUID:   // Read Unique ID
    case S25fl064l::RDSR1:   // Read Status Register 1
    case S25fl064l::RDSR2:   // Read Status Register 2
    case S25fl064l::RDCR1:   // Read Control Register 1
    case S25fl064l::RDCR2:   // Read Control Register 2
    case S25fl064l::RDCR3:   // Read Control Register 3
    case S25fl064l::WRR:     // Write Register (Status -1 and configuration -1,2,3)
    case S25fl064l::WRDI:   // Write Disable
    case S25fl064l::WREN:   // Write enable for non volatile data change
    case S25fl064l::WRENV:   // Write enable for volatile status and configuration registers
    case S25fl064l::CLSR:   // Clear status register
    case S25fl064l::_4BEN:   // Enter 4 byte address mode
    case S25fl064l::_4BEX:   // Exit 4 byte address mode
    case S25fl064l::SBEL:   // Set burst length
    case S25fl064l::QPIEN:   // Enter QPI
    case S25fl064l::QPIEX:   // Exit QPI
    case S25fl064l::DLPRD:   // Data learning pattern read
    case S25fl064l::PDLRNV:   // Program NV data learning register
    case S25fl064l::WDLRV:   // Write volatile data learning center
    case S25fl064l::CE:   // Chip erase
    case S25fl064l::EPS:   // Erase / program suspend
    case S25fl064l::EPR:   // Erase / program resume
    case S25fl064l::GBL:   // Global IBL block
    case S25fl064l::GBUL:   // Global IBL unblock
    case S25fl064l::RSTEN:        // Software reset enable
    case S25fl064l::RST:        // Software reset
    case S25fl064l::MBR:        // Mode bit reset
    case S25fl064l::DPD:        // Deep power down
    case S25fl064l::RES:     //from deep power down / device idcase 
      return 0;
    case S25fl064l::RSFDP:   // Read JEDEC Serial Flash Discoverable parameters
    case S25fl064l::RDAR: 
    case S25fl064l::WRAR:   // Write any register
    case S25fl064l::READ:   // Read
    case S25fl064l::FAST_READ:   // Fast read
    case S25fl064l::DOR:   // Dual output read
    case S25fl064l::QOR:   // Quad output read
    case S25fl064l::QIOR:   // Quad I/O read
    case S25fl064l::DIOR:   // Dual I/O read
    case S25fl064l::DDRQIOR:   // DDR Quad I/O read
    case S25fl064l::PP:   // Page program
    case S25fl064l::QPP:   // Quad page program
    case S25fl064l::SE:   // Sector erase
    case S25fl064l::HBE:   // Half-block erase
    case S25fl064l::BE:   // Block erase
    case S25fl064l::SECRE:   // Security region erase
    case S25fl064l::SECRP:   // Security region program
    case S25fl064l::SECRR:   // Security region read
    case S25fl064l::IBLRD:   // IBL read
    case S25fl064l::IBL:   // IBL lock
    case S25fl064l::IBUL:   // IBL unblock
    case S25fl064l::SPRP:   // Set pointer region protection
      return m_addressLengthFormat;
    case S25fl064l::_4READ:   // Read
    case S25fl064l::_4FAST_READ:   // Read
    case S25fl064l::_4DOR:   // Dual output read
    case S25fl064l::_4QOR:   // Dual output read
    case S25fl064l::_4DIOR:   // Dual I/O read
    case S25fl064l::_4QIOR:   // Quad I/O read
    case S25fl064l::_4DDRQIOR:   // DDR Quad I/O read
    case S25fl064l::_4PP:   // Page program
    case S25fl064l::_4QPP:   // Quad page program
    case S25fl064l::_4SE:   // Sector erase
    case S25fl064l::_4HBE:   // Half-block erase
    case S25fl064l::_4BE:   // Block erase
    case S25fl064l::_4IBLRD:   // IBL read
    case S25fl064l::_4IBUL:   // IBL unblock
    case S25fl064l::_4IBL:   // IBL lock
    case S25fl064l::_4SPRP:   // Set pointer region protection
      return 4;
    default:
      return 0;
  }
}

/**
* @brief      Gets the read dummy cycles.
*
* @param[in]  cmd   The command
*
* @return     The read dummy cycles.
*/
uint8_t S25fl064l :: getReadDummyCycles(const S25fl064l::FlashSpiCommands cmd){
  switch(cmd){
    case S25fl064l::RDID:   // Read ID
    case S25fl064l::RDQID:   // Read Quad ID
    case S25fl064l::RUID:   // Read Unique ID
    case S25fl064l::RDSR1:   // Read Status Register 1
    case S25fl064l::RDSR2:   // Read Status Register 2
    case S25fl064l::RDCR1:   // Read Control Register 1
    case S25fl064l::RDCR2:   // Read Control Register 2
    case S25fl064l::RDCR3:   // Read Control Register 3
    case S25fl064l::WRR:     // Write Register (Status -1 and configuration -1,2,3)
    case S25fl064l::WRDI:   // Write Disable
    case S25fl064l::WREN:   // Write enable for non volatile data change
    case S25fl064l::WRENV:   // Write enable for volatile status and configuration registers
    case S25fl064l::CLSR:   // Clear status register
    case S25fl064l::_4BEN:   // Enter 4 byte address mode
    case S25fl064l::_4BEX:   // Exit 4 byte address mode
    case S25fl064l::SBEL:   // Set burst length
    case S25fl064l::QPIEN:   // Enter QPI
    case S25fl064l::QPIEX:   // Exit QPI
    case S25fl064l::PDLRNV:   // Program NV data learning register
    case S25fl064l::WDLRV:   // Write volatile data learning center
    case S25fl064l::CE:   // Chip erase
    case S25fl064l::EPS:   // Erase / program suspend
    case S25fl064l::EPR:   // Erase / program resume
    case S25fl064l::GBL:   // Global IBL block
    case S25fl064l::GBUL:   // Global IBL unblock
    case S25fl064l::RSTEN:        // Software reset enable
    case S25fl064l::RST:        // Software reset
    case S25fl064l::MBR:        // Mode bit reset
    case S25fl064l::DPD:        // Deep power down
    case S25fl064l::RES:     //from deep power down / device idcase
    case S25fl064l::WRAR:   // Write any register
    case S25fl064l::READ:   // Read
    case S25fl064l::PP:   // Page program
    case S25fl064l::QPP:   // Quad page program
    case S25fl064l::SE:   // Sector erase
    case S25fl064l::HBE:   // Half-block erase
    case S25fl064l::BE:   // Block erase
    case S25fl064l::SECRE:   // Security region erase
    case S25fl064l::SECRP:   // Security region program
    case S25fl064l::IBLRD:   // IBL read
    case S25fl064l::IBL:   // IBL lock
    case S25fl064l::IBUL:   // IBL unblock
    case S25fl064l::SPRP:   // Set pointer region protection
    case S25fl064l::_4READ:   // Read
    case S25fl064l::_4PP:   // Page program
    case S25fl064l::_4QPP:   // Quad page program
    case S25fl064l::_4SE:   // Sector erase
    case S25fl064l::_4HBE:   // Half-block erase
    case S25fl064l::_4BE:   // Block erase
    case S25fl064l::_4IBLRD:   // IBL read
    case S25fl064l::_4IBUL:   // IBL unblock
    case S25fl064l::_4IBL:   // IBL lock
    case S25fl064l::_4SPRP:   // Set pointer region protection
    case S25fl064l::DLPRD:   // Data learning pattern read
      return 0;
    case S25fl064l::RSFDP:   // Read JEDEC Serial Flash Discoverable parameters
    case S25fl064l::SECRR:   // Security region read
    case S25fl064l::RDAR:
    case S25fl064l::DDRQIOR:   // DDR Quad I/O read
    case S25fl064l::_4DDRQIOR:   // DDR Quad I/O read
    case S25fl064l::_4QIOR:   // Quad I/O read
    case S25fl064l::QIOR:   // Quad I/O read
    case S25fl064l::DIOR:   // Dual I/O read
    case S25fl064l::_4DIOR:   // Dual I/O read
    case S25fl064l::DOR:   // Dual output read
    case S25fl064l::_4DOR:   // Dual output read
    case S25fl064l::QOR:   // Quad output read
    case S25fl064l::_4QOR:   // Dual output read
    case S25fl064l::FAST_READ:   // Fast read
    case S25fl064l::_4FAST_READ:   // Read
      return m_readLatencyCycles;
    default:
      return 0;
  }
}


/**
 * @brief      Read spi register from external flash
 *
 * @param[in]  cmd           The command
 * @param      rxData        The receive data
 * @param[in]  sizeOfRxData  The size of the receive data
 * @param[in]  address       The address
 * @param[in]  dataReadLength  The data read length
 * @param[in]  regStartAddr    The register start address
 * @param[in]  length          The length
 *
 * @return     The device error code
 */
S25fl064l::S25fl064lError S25fl064l :: flashSpiReadData(const S25fl064l::FlashSpiCommands cmd,
                                                          uint8_t *rxData,
                                                          const uint16_t sizeOfRxData,
                                                          const S25fl064l::Address address){
  uint16_t addressLength;
  uint32_t totalBytesToTransmit;
  uint32_t totalBytesToRead;
  uint32_t i;

  if(rxData == NULL){
    return S25FL064L_UNEXPECTED_ERROR;
  }

  totalBytesToRead = sizeOfRxData;

  // at first, we just send command + any dummy cycles (converted to number of bytes)
  totalBytesToTransmit = 1;
  m_spiTxBuff[0] = (uint8_t) cmd;

  addressLength = getAddressLengthByte(cmd);
  totalBytesToTransmit += addressLength;

  // Address must be defined if addressLength greater than 0
  if(addressLength > 0){
    if(address == ADDRESS_NOT_DEFINED){
      return S25FL064L_UNEXPECTED_ERROR;
    }

  // Add dummy cycles required for some commands
  // Convert dummy cycles to number of bytes
  // HERCULES SPI ONLY GENERATE NUMBER OF DUMMY CYCLES MULTIPLE OF 8
  // Default value set by S25FL064 is 8
  totalBytesToTransmit += getReadDummyCycles(cmd) >> 3;

  if(totalBytesToTransmit > SPI_TX_BUFFER_MAX_LENGTH)
      return S25FL064L_WRONG_DATA_SIZE;

   // copy address to Tx buffer
   for(i=0; i<addressLength; i++){
       m_spiTxBuff[i+1] = address >> i*8 & 0xff;
   }
  }

  // Set CS low
  gioSetBit(CS_SPI_PORT, CS_SPI_BIT, 0);

  // Send transmission data
  spiTransmitData(m_flashSpi, &m_flashDataConfig, totalBytesToTransmit, (uint16_t *)&m_spiTxBuff);

  while(totalBytesToRead > 0){
      // min(totalBytesToRead, SPI_RX_BUFFER_MAX_LENGTH)
      uint16_t bytesToRead = (totalBytesToRead < SPI_RX_BUFFER_MAX_LENGTH) ? 
                              totalBytesToRead : SPI_RX_BUFFER_MAX_LENGTH;
      spiReceiveData(m_flashSpi, &m_flashDataConfig, bytesToRead, (uint16_t *)m_spiRxBuff);

      // remove bytes to read
      totalBytesToRead -= bytesToRead;

      // copy bytes from rx buffer to destination buffer
      for(i=0; i<bytesToRead; i++){
          *rxData = m_spiRxBuff[i] & 0xff;
          rxData++;
      }
  }

  // Set CS high
  gioSetBit(CS_SPI_PORT, CS_SPI_BIT, 1);

  return S25FL064L_NO_ERROR;
}


/**
 * @brief      Write data flash device over SPI
 *
 * @param[in]  cmd     The command
 * @param      txData  The transmit data
 * @param[in]  length  The length
 *
 * @return     The device error code
 */
S25fl064l::S25fl064lError S25fl064l :: flashSpiWriteData(const S25fl064l::FlashSpiCommands cmd,
                                                              uint8_t *txData,
                                                              const uint16_t sizeOfTxData,
                                                              S25fl064l::Address address){
  uint16_t addressLength;
  uint16_t totalBytesToTransmit; 
  uint16_t i;

  if(txData == NULL && sizeOfTxData > 0){
    return S25FL064L_UNEXPECTED_ERROR;
  }

  addressLength = getAddressLengthByte(cmd);
  m_spiTxBuff[0] = (uint8_t) cmd;
  totalBytesToTransmit = sizeOfTxData + 1 /*command*/ + addressLength;

  if(addressLength > 0){
    if(address == ADDRESS_NOT_DEFINED){
      return S25FL064L_UNEXPECTED_ERROR;
    }

    if(totalBytesToTransmit > SPI_TX_BUFFER_MAX_LENGTH){
      return S25FL064L_WRONG_DATA_SIZE;
    }

    // copy address to Tx buffer
    for(i=0; i<addressLength; i++){
        m_spiTxBuff[i+1] = address >> i*8 & 0xff;
    }

    if(txData != NULL && sizeOfTxData > 0){
      // Copy data to transmit buffer
      // copy address to Tx buffer
      for(i=0; i<sizeOfTxData; i++){
        m_spiTxBuff[i+1+addressLength] = *txData & 0xff;
        txData++;
      }
    }
  }

  // Set CS low
  gioSetBit(CS_SPI_PORT, CS_SPI_BIT, 0);

  spiTransmitData(m_flashSpi, &m_flashDataConfig, totalBytesToTransmit, (uint16_t *)&m_spiTxBuff);

  // Set CS high
  gioSetBit(CS_SPI_PORT, CS_SPI_BIT, 1);

  return S25FL064L_NO_ERROR;
}

/**
 * @brief      Suspend an erase or program operation
 *
 * @return     The device error code
 */
S25fl064l::S25fl064lError S25fl064l :: programEraseSuspend(){
  S25fl064l::StatusRegister1 status1;
  S25fl064l::StatusRegister2 status2;
  S25fl064lError err;

  // Read status register to check if program/erase is on-going
  err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  err = flashSpiReadData(S25fl064l::RDSR2, &status2.all, 1);
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Check if erase or program are in progress, if not, there is nothing to
  // suspend Check if an erase suspend or program suspend is already
  // happening, if so, don't do anything.
  if(status1.bit.wip == 0 && (status2.bit.es || status2.bit.ps)){
    return S25FL064L_NO_ERROR;
  }

  // Send sector erase The flash spi write doesn't require to write any data,
  // only address is required
  err = flashSpiWriteData(S25fl064l::EPS);

  return err;
}


/**
 * @brief      Resume programming / erase
 *
 * @return     The device error code
 */
S25fl064l::S25fl064lError S25fl064l :: programEraseResume(){
  S25fl064l::StatusRegister1 status1;
  S25fl064l::StatusRegister2 status2;
  S25fl064lError err;

  // Read status register to check if program/erase is on-going
  err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  err = flashSpiReadData(S25fl064l::RDSR2, &status2.all, 1);
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // If nothing is in progress or suspended, return immediately
  if(status1.bit.wip == 0 && status2.bit.es == 0 && status2.bit.ps == 0){
    return S25FL064L_NO_ERROR;
  }

  // Send sector erase The flash spi write doesn't require to write any data,
  // only address is required
  err = flashSpiWriteData(S25fl064l::EPR);

  return err;
}

/**
 * @brief      Perform a soft reset of the device
 *
 * @return     The device error code
 */
S25fl064l::S25fl064lError S25fl064l :: resetDevice(){
  S25fl064lError err;

  // Enable reset command
  err = flashSpiWriteData(S25fl064l::RSTEN);
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Perform a soft reset of the device
  err = flashSpiWriteData(S25fl064l::RST);

  return err;
}

/**
 * @brief      Erase the chip memory
 *
 * @return     The device error
 */
S25fl064l::S25fl064lError S25fl064l :: chipErase(){
  S25fl064l::StatusRegister1 status1;
  S25fl064l::StatusRegister2 status2;
  S25fl064lError err;
  uint32_t tries = __INT_MAX;

  // Enable writing to the device
  err = flashSpiWriteData(S25fl064l::WREN);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Send sector erase
  // The flash spi write doesn't require to write any data, only address is required
  err = flashSpiWriteData(S25fl064l::CE);

  // Read status register to check that write is enabled
  err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
  
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Write Enable Latch must be set at that point
  if(status1.bit.wel == 0){
    return S25FL064L_FAIL_ERASE_CHIP;
  }

  // Poll the device to check the "work in progress" flag from the status register 1
  while(tries > 0){
    err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
    if(err != S25FL064L_NO_ERROR){
      return err;
    }

    if(status1.bit.wip == 1){
      tries--;
    }
    else{
      break;
    }
  }

  if(tries == 0){
    return S25FL064L_FAIL_ERASE_CHIP;
  }

  // Check if an error occured at completion of the programming
  err = flashSpiReadData(S25fl064l::RDSR2, &status2.all, 1);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  if(status2.bit.e_err){
    return S25FL064L_FAIL_PAGE_PROGRAM;
  }

  // Disable writing to the device
  err = flashSpiWriteData(S25fl064l::WRDI);

  return err;
}

/**
 * @brief      Erase memry block
 *
 * @param[in]  block  The block of memory to erase
 *
 * @return     The device error
 */
S25fl064l::S25fl064lError S25fl064l :: blockErase(const S25fl064l::Block block){
  S25fl064l::Address address;
  S25fl064l::StatusRegister1 status1;
  S25fl064l::StatusRegister2 status2;
  S25fl064lError err;
  uint32_t tries = __INT_MAX;

  if(block > MAX_BLOCK_RANGE){
    return S25FL064L_UNEXPECTED_ERROR;
  }

  // Enable writing to the device
  err = flashSpiWriteData(S25fl064l::WREN);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  address = block * BLOCK_SIZE;

  // Send sector erase
  // The flash spi write doesn't require to write any data, only address is required
  err = flashSpiWriteData(S25fl064l::BE, (uint8_t *)NULL, 0, address);

  // Read status register to check that write is enabled
  err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
  
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Write Enable Latch must be set at that point
  if(status1.bit.wel == 0){
    return S25FL064L_FAIL_SECTOR_ERASE;
  }

  // Poll the device to check the "work in progress" flag from the status register 1
  while(tries > 0){
    err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
    if(err != S25FL064L_NO_ERROR){
      return err;
    }

    if(status1.bit.wip == 1){
      tries--;
    }
    else{
      break;
    }
  }

  if(tries == 0){
    return S25FL064L_FAIL_BLOCK_ERASE;
  }

  // Check if an error occured at completion of the programming
  err = flashSpiReadData(S25fl064l::RDSR2, &status2.all, 1);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  if(status2.bit.e_err){
    return S25FL064L_FAIL_PAGE_PROGRAM;
  }

  // Disable writing to the device
  err = flashSpiWriteData(S25fl064l::WRDI);

  return err;
}
  
/**
 * @brief      Erase half-block of memory
 *
 * @param[in]  block  The block
 *
 * @return     The device error
 */
S25fl064l::S25fl064lError S25fl064l :: halfBlockErase(const S25fl064l::HalfBlock halfBlock){
  S25fl064l::Address address;
  S25fl064l::StatusRegister1 status1;
  S25fl064l::StatusRegister2 status2;
  S25fl064lError err;
  uint32_t tries = __INT_MAX;

  if(halfBlock > MAX_HALF_BLOCK_RANGE){
    return S25FL064L_UNEXPECTED_ERROR;
  }

  // Enable writing to the device
  err = flashSpiWriteData(S25fl064l::WREN);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  address = halfBlock * HALF_BLOCK_SIZE;

  // Send sector erase
  // The flash spi write doesn't require to write any data, only address is required
  err = flashSpiWriteData(S25fl064l::HBE, (uint8_t *)NULL, 0, address);

  // Read status register to check that write is enabled
  err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
  
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Write Enable Latch must be set at that point
  if(status1.bit.wel == 0){
    return S25FL064L_FAIL_HALF_BLOCK_ERASE;
  }

  // Poll the device to check the "work in progress" flag from the status register 1
  while(tries > 0){
    err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
    if(err != S25FL064L_NO_ERROR){
      return err;
    }

    if(status1.bit.wip == 1){
      tries--;
    }
    else{
      break;
    }
  }

  if(tries == 0){
    return S25FL064L_FAIL_HALF_BLOCK_ERASE;
  }

  // Check if an error occured at completion of the erase
  err = flashSpiReadData(S25fl064l::RDSR2, &status2.all, 1);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  if(status2.bit.e_err){
    return S25FL064L_FAIL_PAGE_PROGRAM;
  }

  // Disable writing to the device
  err = flashSpiWriteData(S25fl064l::WRDI);

  return err;
}

/**
 * @brief      Erase a sector
 *
 * @param[in]  sector  The sector
 *
 * @return     Return a device error
 */
S25fl064l::S25fl064lError S25fl064l :: sectorErase(const S25fl064l::Sector sector){
  S25fl064l::Address address;
  S25fl064l::StatusRegister1 status1;
  S25fl064l::StatusRegister2 status2;
  S25fl064lError err;
  uint32_t tries = __INT_MAX;

  if(sector > MAX_SECTOR_RANGE){
    return S25FL064L_UNEXPECTED_ERROR;
  }

  // Enable writing to the device
  err = flashSpiWriteData(S25fl064l::WREN);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Read status register to check that write is enabled
  err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
  
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Write Enable Latch must be set at that point
  if(status1.bit.wel == 0){
    return S25FL064L_FAIL_SECTOR_ERASE;
  }

  // Send sector erase
  // The flash spi write doesn't require to write any data, only address is required
  address = sector * SECTOR_SIZE;
  err = flashSpiWriteData(S25fl064l::SE, (uint8_t *)NULL, 0, address);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Poll the device to check the "work in progress" flag from the status register 1
  while(tries > 0){
    err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
    if(err != S25FL064L_NO_ERROR){
      return err;
    }

    if(status1.bit.wip == 1){
      tries--;
    }
    else{
      break;
    }
  }

  if(tries == 0){
    return S25FL064L_FAIL_SECTOR_ERASE;
  }

  // Check if an error occured at completion of the erase
  err = flashSpiReadData(S25fl064l::RDSR2, &status2.all, 1);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  if(status2.bit.e_err){
    return S25FL064L_FAIL_PAGE_PROGRAM;
  }

  // Disable writing to the device
  err = flashSpiWriteData(S25fl064l::WRDI);

  return err;
}


/**
 * @brief      Allocate flash memory. This is a simple implementation of a
 *             malloc function. It reserves memory aligned with sectors and
 *             blocks. It update the reserved size argument and update address
 *             pointer with a valid address or NULL and an error code if
 *             address cannot be reserved.
 *
 * @param[in]  size   The size
 * @param      alloc  The allocated memory structure
 *
 * @return     The device error
 */
S25fl064l::S25fl064lError S25fl064l :: allocateFlashMemory(S25fl064l::MemAlloc *alloc,
                                                                const uint32_t size){

  S25fl064l::Address tmp = m_memAllocPointer;

  // Reset reserved memory to zero
  alloc->reservedSize = 0;

  // Calculate new address pointer by PAGE_SIZE increment
  tmp += (size / PAGE_SIZE) * PAGE_SIZE;
  tmp += (size % PAGE_SIZE) ? PAGE_SIZE : 0;

  // Check that new memory allocation pointer is within memory range 
  if(tmp > MAX_MEMORY_ADDRESS){
    return S25FL064L_FAIL_MEM_ALLOCATION;
  }

  //Update address with valid address pointer
  alloc->startAddress = m_memAllocPointer;

  // Update the reserved size (larger or equal to size requested)
  alloc->reservedSize = tmp - m_memAllocPointer;

  //Increment address pointer for next memory allocation
  m_memAllocPointer += tmp;

  return S25FL064L_NO_ERROR;
}


/**
 * @brief      Writes a data to flash.
 *
 * @param      alloc     The allocate
 * @param[in]  offset    The offset from the start address of the memory
 *                       allocation
 * @param      data      The data
 * @param[in]  dataSize  The data size
 *
 * @return     The device error
 */
S25fl064l::S25fl064lError S25fl064l :: writeDataToFlash(S25fl064l::MemAlloc *alloc,
                                                             const uint32_t offset,
                                                             uint8_t *data,
                                                             const uint16_t dataSize){
  S25fl064lError err;
  S25fl064l::Address sectorAddress;
  uint16_t i;
  uint16_t sectorOverlaps = 0;
  uint16_t totalBytesToWrite = dataSize;

  // Check if memory allocation is valid
  if(alloc->reservedSize == 0){
    return S25FL064L_FAIL_WRITE_DATA_FLASH;
  }

  // Check if desired data write will fit in device memory range
  if(alloc->startAddress + offset + dataSize > MAX_MEMORY_ADDRESS){
    return S25FL064L_FAIL_WRITE_DATA_FLASH;
  }

  // Check if data to write fits allocated size
  if(dataSize > alloc->reservedSize){
    return S25FL064L_FAIL_WRITE_DATA_FLASH;
  }

  //loop here until there is data to write or an error occurs
  while(1){
    // Calculate the block address that belong to data to write.
    // Go to next sector if the data to write overlaps sectors
    sectorAddress = (alloc->startAddress + offset) / SECTOR_SIZE * (SECTOR_SIZE + sectorOverlaps);

    // Back-up sector content to a back-up buffer
    err = flashSpiReadData( S25fl064l::READ,
                            m_sectorBackup,
                            sizeof(m_sectorBackup),
                            sectorAddress);

    if(err != S25FL064L_NO_ERROR){
        return err;
    }

    // Erase sector address
    err = sectorErase((S25fl064l::Sector) sectorAddress);

    if(err != S25FL064L_NO_ERROR){
        return err;
    }

    // Copy data to scratchpad
    // Only copy a number of bytes to not overlap sectors
    uint16_t bytesToCopy = (totalBytesToWrite <= sectorAddress + SECTOR_SIZE - (alloc->startAddress + offset)) ?
                            totalBytesToWrite : sectorAddress + SECTOR_SIZE - (alloc->startAddress + offset);


    // Overwrite data in the m_sectorBackup. Don't copy data that overlap
    // sectors
    // If data is NULL, then fill it with NULL character
    if(data != NULL){
      memcpy(m_sectorBackup + alloc->startAddress + offset + (sectorOverlaps*SECTOR_SIZE) - sectorAddress,
             data,
             bytesToCopy);
    }
    else{
      for(uint16_t i=0; i<bytesToCopy; i++){
        m_sectorBackup[alloc->startAddress + offset + (sectorOverlaps*SECTOR_SIZE) - sectorAddress + i] = '\0';
      }
    }

    // Program the whole sector using page programming with updated data
    // Programming is done aligned with page addresses
    for(i=0; i< SECTOR_SIZE / PAGE_SIZE; i++){
      err = pageProgram(alloc->startAddress+ i*PAGE_SIZE, m_sectorBackup + i*PAGE_SIZE, PAGE_SIZE);
      if(err != S25FL064L_NO_ERROR){
        return err;
      }
    }

    //Substract the number of byte programmed from the number of bytes
    totalBytesToWrite -= bytesToCopy;

    // if there is still some bytes to write, go to next memory sector
    if(totalBytesToWrite > 0){
      sectorOverlaps++;
    }
    else{
      break;  // write completed
    }
  } // end of while loop

  return S25FL064L_NO_ERROR;
}


/**
 * @brief      Reads a data from flash.
 *
 * @param      alloc     The allocate
 * @param[in]  offset    The offset
 * @param      data      The data
 * @param[in]  dataSize  The data size
 *
 * @return     The device error
 */
S25fl064l::S25fl064lError S25fl064l :: readDataFromFlash(S25fl064l::MemAlloc *alloc,
                                                              const uint32_t offset,
                                                              uint8_t *data,
                                                              const uint16_t dataSize){
  S25fl064lError err;
  uint16_t totalBytesToRead = dataSize;
  uint16_t overflow = 0;

  while(totalBytesToRead){
    uint16_t bytesToRead = (totalBytesToRead < PAGE_SIZE) ? totalBytesToRead : PAGE_SIZE;

    // Read data from flash memory
    // Send the READ command
    err = flashSpiReadData( S25fl064l::READ,
                            data + PAGE_SIZE * overflow,
                            bytesToRead,
                            alloc->startAddress + PAGE_SIZE * overflow + offset);

    totalBytesToRead -= bytesToRead;
    overflow++;
  }

  return err;
}

/**
 * @brief      Program a page
 *
 * @param[in]  address  The address
 * @param      txData   The transmit data
 * @param[in]  size     The size
 *
 * @return     The error code.
 */
S25fl064l::S25fl064lError S25fl064l :: pageProgram(S25fl064l::Address address,
                                                        uint8_t *txData,
                                                        const uint16_t size){
  S25fl064l::StatusRegister1 status1;
  S25fl064l::StatusRegister2 status2;
  S25fl064lError err;
  uint32_t tries = __INT_MAX;

  // the size of a page is 256 bytes
  if(size > PAGE_SIZE){
    return S25FL064L_UNEXPECTED_ERROR;
  }

  // Check if pointer to transmit buffer exists
  if(txData == NULL){
    return S25FL064L_UNEXPECTED_ERROR;
  }

  // Enable writing to the device
  err = flashSpiWriteData(S25fl064l::WREN);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Read status register
  err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
  
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Write Enable Latch must be set at that point
  if(status1.bit.wel == 0){
    return S25FL064L_FAIL_PAGE_PROGRAM;
  }

  // Send data to perform page programming
  err = flashSpiWriteData(S25fl064l::PP, txData, size, address);
  
  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  // Poll the device to check the "work in progress" flag from the status
  // register 1
  while(tries > 0){
    err = flashSpiReadData(S25fl064l::RDSR1, &status1.all, 1);
    if(err != S25FL064L_NO_ERROR){
      return err;
    }

    if(status1.bit.wip == 1){
      tries--;
    }
    else{
      break;
    }
  }

  // Check if an error occured at completion of the programming
  err = flashSpiReadData(S25fl064l::RDSR2, &status2.all, 1);

  if(err != S25FL064L_NO_ERROR){
    return err;
  }

  if(status2.bit.p_err){
    return S25FL064L_FAIL_PAGE_PROGRAM;
  }

  // Disable writing to the device
  err = flashSpiWriteData(S25fl064l::WRDI);

  return err;
}
