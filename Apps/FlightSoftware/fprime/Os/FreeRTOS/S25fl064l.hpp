#ifndef __S25FL064L_HPP__
#define __S25FL064L_HPP__

#include <spi.h>
#include <string.h>
#include <gio.h>
#include <math.h>

class S25fl064l{
  #define SPI_RX_BUFFER_MAX_LENGTH 261    // Worst case: 256 data payload bytes + 1 byte command + 4 bytes addressing
  #define SPI_TX_BUFFER_MAX_LENGTH 261

  #define SPI_REG spiREG3
  #define CS_SPI_PORT spiPORT3
  #define CS_SPI_BIT  0


  enum AddressLengthFormat{
    ADDRESS_LENGTH_3_BYTES  = 3,
    ADDRESS_LENGTH_4_BYTES  = 4
  };

  #define DEFAULT_DUMMY_CYCLES 8
  #define MAX_DUMMY_CYCLES     16

  struct StatusRegister1Bits{
    uint8_t wip:1;
    uint8_t wel:1;
    uint8_t bp:3;
    uint8_t tbProt:1;
    uint8_t sec:1;
    uint8_t srp0:1;
  };

  typedef union StatusRegister1{
    uint8_t all;
    StatusRegister1Bits bit;
  }StatusRegister1;

  struct StatusRegister2Bits{
    uint8_t ps:1;
    uint8_t es:1;
    uint8_t rsv:3;
    uint8_t p_err:1;
    uint8_t e_err:1;
    uint8_t rsv2:1;
  };

  typedef union StatusRegister2{
    uint8_t all;
    StatusRegister2Bits bit;
  }StatusRegister2;

  struct ConfigurationRegister1Bits{
    uint8_t srp1_d:1;
    uint8_t quad:1;
    uint8_t lb:4;
    uint8_t cmp:1;
    uint8_t sus:1;
  };

  typedef union ConfigurationRegister1{
    uint8_t all;
    ConfigurationRegister1Bits bit;
  }ConfigurationRegister1;

  struct ConfigurationRegister2Bits{
    uint8_t rsv:1;
    uint8_t adp:1;
    uint8_t wps:1;
    uint8_t qpi:1;
    uint8_t rsv2:1;
    uint8_t oi:2;
    uint8_t io3r:1;
  };

  typedef union ConfigurationRegister2{
    uint8_t all;
    ConfigurationRegister2Bits bit;
  }ConfigurationRegister2;

  struct ConfigurationRegister3Bits{
    uint8_t rl:4;
    uint8_t we:1;
    uint8_t wl:2;
    uint8_t rsv:1;
  };

  typedef union ConfigurationRegister3{
    uint8_t all;
    ConfigurationRegister3Bits bit;
  }ConfigurationRegister3;

  struct IrpRegisterBits{
    uint16_t permlb:1;
    uint16_t pslmlb:1;
    uint16_t pwdmlb:1;
    uint16_t rsv:1;
    uint16_t ibllbb:1;
    uint16_t rsv2:1;
    uint16_t secrrp:1;
    uint16_t rsv3:9;
  };

  typedef union IrpRegister{
    uint16_t all;
    IrpRegisterBits bit;
  }IrpRegister;

  typedef uint64_t PasswordRegister;

  struct ProtectionStatusRegisterBits{
    uint8_t nvclock:1;
    uint8_t rsv:5;
    uint8_t secrrp:1;
    uint8_t rsv2:1;
  };

  typedef union ProtectionStatusRegister{
    uint8_t all;
    uint8_t ProtectionStatusRegisterBits;
  }ProtectionStatusRegister;


  typedef uint8_t IblAccessRegister;

  struct PointerRegionProtectionRegisterBits{
    uint32_t rsv:9;
    uint32_t prptb:1;
    uint32_t prpen:1;
    uint32_t prpall:1;
    uint32_t rsv2:4;
    uint32_t prpad:7;
    uint32_t rsv3:9;
  };

  typedef union PointerRegionProtectionRegister{
    uint32_t all;
    PointerRegionProtectionRegisterBits bit;
  }PointerRegionProtectionRegister;

  typedef uint8_t NonVolatileDataLearningDataRegister;
  typedef uint8_t VolatileDataLearningDataRegister;
  typedef uint32_t Address;
  typedef uint16_t PageNumber;
  typedef uint16_t Block;
  typedef uint16_t HalfBlock;
  typedef uint16_t Sector;

  #define PAGE_SIZE               0x100     // 256B
  #define SECTOR_SIZE             0x1000    // 4KB
  #define HALF_BLOCK_SIZE         0x8000    // 32KB
  #define BLOCK_SIZE              0x10000   // 64KB

  #define MAX_MEMORY_ADDRESS      0x7FFFFF            // 64Mb
  #define MAX_BLOCK_RANGE         128                 // There is 128 blocks (from block #0 to block #127)
  #define MAX_HALF_BLOCK_RANGE    MAX_BLOCK_RANGE*2 
  #define MAX_SECTOR_RANGE        MAX_BLOCK_RANGE*16  // There are 16 sectors per block (16*128 = 2048 sectors)

  #define ADDRESS_NOT_DEFINED     0xFFFFFFFF

  #define FLASH_MANUFACTURER_ID   0x01
  #define FLASH_DEVICE_ID_MSB     0x60
  #define FLASH_DEVICE_ID_LSB     0x17


  typedef enum FlashSpiCommands{
    // Read Device ID
    RDID =        0x9F,   // Read ID
    RSFDP =       0x5A,   // Read JEDEC Serial Flash Discoverable parameters
    RDQID =       0xAF,   // Read Quad ID
    RUID =        0x4B,   // Read Unique ID
    // Register access
    RDSR1 =       0x05,   // Read Status Register 1
    RDSR2 =       0x07,   // Read Status Register 2
    RDCR1 =       0x35,   // Read Control Register 1
    RDCR2 =       0x15,   // Read Control Register 2
    RDCR3 =       0x33,   // Read Control Register 3
    RDAR =        0x65,   // Rad  any register 
    WRR =         0x01,   // Write Register (Status -1 and configuration -1,2,3)
    WRDI =        0x04,   // Write Disable
    WREN =        0x06,   // Write enable for non volatile data change
    WRENV =       0x50,   // Write enable for volatile status and configuration registers
    WRAR =        0x71,   // Write any register
    CLSR =        0x30,   // Clear status register
    _4BEN =       0xB7,   // Enter 4 byte address mode
    _4BEX =       0xE9,   // Exit 4 byte address mode
    SBEL =        0x77,   // Set burst length
    QPIEN =       0x38,   // Enter QPI
    QPIEX =       0xF5,   // Exit QPI
    DLPRD =       0x41,   // Data learning pattern read
    PDLRNV =      0x43,   // Program NV data learning register
    WDLRV =       0x4A,   // Write volatile data learning center
    // Read flash array
    _4READ =      0x13,   // Read
    _4FAST_READ = 0x0C,   // Read
    FAST_READ =   0x0B,   // Fast read
    READ =        0x03,   // Fast read
    _4DOR =       0x3C,   // Dual output read
    _4QOR =       0x6C,   // Dual output read
    DOR =         0x3B,   // Dual output read
    QOR =         0x6B,   // Quad output read
    QIOR =        0xEB,   // Quad I/O read
    DIOR =        0xBB,   // Dual I/O read
    _4DIOR =      0xBC,   // Dual I/O read
    _4QIOR =      0xEC,   // Quad I/O read
    _4DDRQIOR =   0xEE,   // DDR Quad I/O read
    DDRQIOR =     0xED,   // DDR Quad I/O read
    // Program flash array
    PP =          0x02,   // Page program
    _4PP =        0x12,   // Page program
    QPP =         0x32,   // Quad page program
    _4QPP =       0x34,   // Quad page program
    // Erase flash array
    SE =          0x20,   // Sector erase
    _4SE =        0x21,   // Sector erase
    HBE =         0x52,   // Half-block erase
    _4HBE =       0x54,   // Half-block erase
    BE =          0xD8,   // Block erase
    _4BE =        0xDC,   // Block erase
    CE =          0x60,   // Chip erase
    // Erase / Program / Suspend / Resume
    EPS =         0x75,   // Erase / program suspend
    EPR =         0x7A,   // Erase / program resume
    // Individual and region protection
    SECRE =       0x44,   // Security region erase
    SECRP =       0x42,   // Security region program
    SECRR =       0x48,   // Security region read
    // Array protection
    IBLRD =       0x3D,   // IBL read
    _4IBLRD =     0xE0,   // IBL read
    IBL =         0x36,   // IBL lock
    _4IBL =       0xE1,   // IBL lock
    _4IBUL =      0xE2,   // IBL unblock
    IBUL =        0x39,   // IBL unblock
    GBL =         0x7E,   // Global IBL block
    GBUL =        0x98,   // Global IBL unblock
    SPRP =        0xFB,   // Set pointer region protection
    _4SPRP =      0xE3,   // Set pointer region protection
    // Reset
    RSTEN =       0x66,   // Software reset enable
    RST =         0x99,   // Software reset
    MBR =         0xFF,   // Mode bit reset
    // Deep power down
    DPD =         0xB9,   // Deep power down
    RES =         0xAB    // Release from deep power down / device id
  }FlashSpiCommands;

  public:
    typedef struct MemAlloc{
      Address startAddress;
      uint32_t reservedSize;
    }MemAlloc;

  typedef enum S25fl064lError{
    S25FL064L_NO_ERROR = 0,
    S25FL064L_UNEXPECTED_ERROR = -1,
    S25FL064L_INCORRECT_FLASH_MEMORY = -2,
    S25FL064L_WRONG_DATA_SIZE = -3,
    S25FL064L_FAIL_ERASE_CHIP = -4,
    S25FL064L_FAIL_BLOCK_ERASE = -5,
    S25FL064L_FAIL_PAGE_PROGRAM = -6,
    S25FL064L_FAIL_SECTOR_ERASE = -7,
    S25FL064L_FAIL_MEM_ALLOCATION = -8,
    S25FL064L_FAIL_WRITE_DATA_FLASH = -9,
    S25FL064L_FAIL_HALF_BLOCK_ERASE = -10
  }S25fl064lError;

  //-------------------------------------------------------------------------------------------
  // Functions specific to external memory
  public:
    S25fl064l();
    ~S25fl064l();
    S25fl064lError writeDataToFlash(S25fl064l::MemAlloc *alloc,
                               const uint32_t offset,
                               uint8_t *data,
                               const uint16_t dataSize);
    S25fl064lError readDataFromFlash(S25fl064l::MemAlloc *alloc,
                                const uint32_t offset,
                                uint8_t *data,
                                const uint16_t dataSize);
    S25fl064lError setupDevice();

  private:
    S25fl064lError flashSpiReadData(const S25fl064l::FlashSpiCommands cmd,
                            uint8_t *rxData,
                            const uint16_t dataReadLength,
                            S25fl064l::Address address = ADDRESS_NOT_DEFINED); 

    S25fl064lError flashSpiWriteData(const S25fl064l::FlashSpiCommands cmd,
                                uint8_t *txData = (uint8_t *)(NULL),
                                const uint16_t dataWriteLength = 0,
                                S25fl064l::Address address = ADDRESS_NOT_DEFINED); 
    uint16_t getAddressLengthByte(const S25fl064l::FlashSpiCommands cmd);
    S25fl064lError allocateFlashMemory(S25fl064l::MemAlloc *alloc,
                                  const uint32_t size);
    S25fl064lError sectorErase(const S25fl064l::Sector sector);
    S25fl064lError halfBlockErase(const S25fl064l::HalfBlock halfBlock);
    S25fl064lError blockErase(const S25fl064l::Block block);
    S25fl064lError chipErase();
    S25fl064lError resetDevice();
    S25fl064lError programEraseResume();
    S25fl064lError programEraseSuspend();
    S25fl064lError pageProgram(S25fl064l::Address address,
                          uint8_t *txData,
                          const uint16_t size);
    uint8_t getReadDummyCycles(const S25fl064l::FlashSpiCommands cmd);
    
  private:
    S25fl064l::AddressLengthFormat m_addressLengthFormat;

    //memory pointer that keeps track of the last allocated memory pointer
    S25fl064l::Address m_memAllocPointer;

    spiDAT1_t m_flashDataConfig;
    spiBASE_t *m_flashSpi;

    bool m_setup;
    uint32_t m_readLatencyCycles;
    uint16_t m_spiRxBuff[SPI_RX_BUFFER_MAX_LENGTH];
    uint16_t m_spiTxBuff[SPI_TX_BUFFER_MAX_LENGTH];
    uint8_t m_writeScratchpad[PAGE_SIZE];
    uint8_t m_sectorBackup[SECTOR_SIZE];
    uint8_t m_dummyBuffer[MAX_DUMMY_CYCLES];
}; // end of class

#endif //__S25FL064L_HPP__
