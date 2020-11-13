#include <Fw/Cfg/Config.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Os/File.hpp>
#include <Fw/Types/Assert.hpp>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <Utils/Hash/libcrc/lib_crc.h> // borrow CRC

#ifdef __cplusplus
}
#endif // __cplusplus

#include <cerrno>
#include <sys/types.h>
#include <limits>
#include <string.h>
#include <stdio.h>


//#define DEBUG_PRINT(x,...) printf(x,##__VA_ARGS__); fflush(stdout)
#define DEBUG_PRINT(x,...)
  

namespace Os {

  /**
   * @brief      Constructs a new instance.
   */
  File::File() {
    m_headerMemAlloc.startAddress = 0; // the look-up table is located at the beginning of the flash memory array
    m_headerMemAlloc.reservedSize = PAGE_SIZE;
  }


  /**
   * @brief      Destroys the object.
   */
  File::~File() {
  }


  /**
   * @brief      Flash memory error to status
   *
   * @param[in]  err   The error
   *
   * @return     File status
   */
  File::Status File::flashMemErrToStatus(const S25fl064l::S25fl064lError err){
    switch(err){
    case S25fl064l::S25FL064L_NO_ERROR:
      return OP_OK;
    case S25fl064l::S25FL064L_WRONG_DATA_SIZE:
      return BAD_SIZE;
    case S25fl064l::S25FL064L_UNEXPECTED_ERROR:
    case S25fl064l::S25FL064L_INCORRECT_FLASH_MEMORY:
    case S25fl064l::S25FL064L_FAIL_ERASE_CHIP:
    case S25fl064l::S25FL064L_FAIL_BLOCK_ERASE:
    case S25fl064l::S25FL064L_FAIL_PAGE_PROGRAM:
    case S25fl064l::S25FL064L_FAIL_SECTOR_ERASE:
    case S25fl064l::S25FL064L_FAIL_MEM_ALLOCATION:
    case S25fl064l::S25FL064L_FAIL_WRITE_DATA_FLASH:
    case S25fl064l::S25FL064L_FAIL_HALF_BLOCK_ERASE:
    default:
      return OTHER_ERROR;
    }
  }

  /**
   * @brief      Open the file or create a new file 
   *
   * @param[in]  fileName  The file name
   * @param[in]  mode      The mode
   *
   * @return     The file status
   */
  File::Status File::open(const char* fileName, File::Mode mode) {
    File::Status stat = OP_OK;
    S25fl064l::S25fl064lError err;

    // Allocate memory space to store information about file to open
    uint8_t readBuffer[SYSTEM_FILE_ENTRY_SIZE];
    uint8_t filefound = false;

    FileName readFileName;
    StartAddress startAddress = 0;
    FileSize fileSize = 0;
    FileSize newFileDefaultSize = NEW_DEFAULT_FILE_SIZE;

    // Check that the device can be accessed.
    m_flashMem.setupDevice();

    // First we read the data from the flash header This is a very very very
    // simple file system specific to the implementation of CubeRover. Header of
    // the flash memory contains: 
    // Address 0x0: name of file [16 bytes]
    // Address 0x10: file start address [4 bytes]
    // Address 0x14: file current location [4 bytes]
    // Address 0x18: file size [4 bytes]
    // Address 0x20: name of file #2 [16 bytes] , etc.
  
    // Look through the memory look-up table and create / open file depending on mode requested  
    for(int i=0; i < MAX_NUMBER_OF_FILES; i++){
      // read from the flash memory
      err = m_flashMem.readDataFromFlash(&m_headerMemAlloc,
                                    i*sizeof(readBuffer),
                                    readBuffer,
                                    sizeof(readBuffer));

      stat = flashMemErrToStatus(err);
      if(stat != OP_OK) return stat;

      // first extract the file name from the buffer
      memcpy(readFileName, readBuffer, SIZE_OF_FILENAME);
      
      // check that a file exist
      if(strncmp(readFileName, fileName, SIZE_OF_FILENAME)){
        // file doesn't match the name
        // if mode is OPEN_CREATE and title is blank (i.e. 0xFFFFFFFF)
        // then create new file
        if(readFileName[0] == 0xFF /* default memory value when flash erased, i.e. no file exists */){
          // if mode is OPEN_CREATE, then create a new file
          if(mode == File::Mode::OPEN_CREATE){
            // allocate and prepare buffer
            uint8_t writeBuffer[SYSTEM_FILE_ENTRY_SIZE];
            
            memcpy((void *)writeBuffer,
                   (void *)fileName,
                   sizeof(FileName)); // write file name

            memcpy((void *)(writeBuffer + sizeof(FileName)),
                   (void *)startAddress,
                   sizeof(StartAddress)); // write start address

            // initialize file pointer to start at start address for a new file
            memcpy((void *)(writeBuffer + sizeof(FileName) + sizeof(StartAddress)),
                   (void *)startAddress,
                   sizeof(FilePointer));

            // preallocate memory space for the new file
            memcpy((void *)(writeBuffer + sizeof(FileName) + sizeof(StartAddress) + sizeof(FilePointer)),
                   (void *)newFileDefaultSize,
                   sizeof(FileSize));

            // Initialize properties 
            this->m_fileIndex = i;
            memcpy((void *)this->m_fileName,
                   (void *)fileName,
                   sizeof(FileName));

            this->m_fileStartAddress = startAddress;
            this->m_fileOffsetPointer = 0;
            this->m_fileSize = newFileDefaultSize;

            // update system file look-up table
            err = m_flashMem.writeDataToFlash(&m_headerMemAlloc, /* start of reserved memory space */
                                              this->m_fileIndex*SYSTEM_FILE_ENTRY_SIZE, /* offset */
                                              writeBuffer,  /* buffer */ 
                                              sizeof(writeBuffer)); /* size of buffer */

            stat = flashMemErrToStatus(err);
            if(stat != OP_OK) return stat;

            // create a new file. By default Flash memory will FF when blank, linux file have '\0' when blank, to mimic that behavior,
            // we initialize the memory to '\0' after updating the system file look-up table
            S25fl064l::MemAlloc tmpAlloc;
            tmpAlloc.startAddress = startAddress;
            tmpAlloc.reservedSize = newFileDefaultSize;
            err = m_flashMem.writeDataToFlash(&tmpAlloc, /* start of reserved memory space */
                                              0,  /* offset */
                                              NULL, /* when set to NULL, it will populate flash with '\0' charcater */
                                              this->m_fileSize); /* fill requested file with '\0' charcater */

            stat = flashMemErrToStatus(err);
            if(stat != OP_OK) return stat;

            // break the loop has the file has been created
            filefound = true; // file is created so "found"
            break;
          }
        }
        else{ /* a file exist but name does not match, go to next entry */
          // save start adresss of current file
          memcpy((void *)startAddress,
                 (void *)readBuffer[sizeof(FileName)],
                 sizeof(StartAddress));
          // save current file size
          memcpy((void *)fileSize,
                 (void *)readBuffer[sizeof(FileName) + sizeof(StartAddress) + sizeof(FilePointer)],
                 sizeof(FileSize));

          startAddress += fileSize; // increment startAddress, used for the next look iteration if a new
                                    // file need to be created
        }
        continue; // go to next entry
      }
      else{ // file name matches
        filefound = true;
        // populate file properties : name, start address of the pre-allocated
        // memory, current read/write pointer and file size
      
        memcpy((void *)this->m_fileName,
               (void *)readBuffer,
               sizeof(FileName));

        memcpy((void *)this->m_fileStartAddress,
               (void *)(readBuffer + sizeof(FileName)),
               sizeof(StartAddress));

        memcpy((void *)this->m_fileOffsetPointer,
               (void *)(readBuffer + sizeof(FileName) + sizeof(StartAddress)),
               sizeof(FilePointer));

        memcpy((void *)this->m_fileSize,
               (void *)(readBuffer + sizeof(FileName) + sizeof(StartAddress) + sizeof(FilePointer)),
               sizeof(FileSize));
      }
    }

    if(!filefound){
      return File::Status::DOESNT_EXIST;
    }

    this->m_lastError = errno;
    this->m_mode = mode;
    return stat;
  }

  /**
   * @brief      Preallocate memory space
   *
   * @param[in]  offset  The offset
   * @param[in]  len     The length
   *
   * @return     The file status
   */
  File::Status File::prealloc(NATIVE_INT_TYPE offset, NATIVE_INT_TYPE len) {
    // make sure it has been opened
    if(OPEN_NO_MODE == this->m_mode) {
        return NOT_OPENED;
    }

    File::Status fileStatus = OP_OK;
    return fileStatus;
  }


  /**
   * @brief      Update file pointer by adding / substracting an offset of bytes.
   *
   * @param[in]  offset    The offset
   * @param[in]  absolute  The absolute
   *
   * @return     The file status
   */
  File::Status File::seek(NATIVE_INT_TYPE offset, bool absolute) {
    Status stat = OP_OK;

    if(OPEN_NO_MODE == this->m_mode) {
      return NOT_OPENED;
    }

    // take the current file pointer and update it
    this->m_fileOffsetPointer = (absolute) ? offset : this->m_fileOffsetPointer + offset;
    
    // update look-up table file current pointer and save it
    m_flashMem.writeDataToFlash(&m_headerMemAlloc, /* start of allocated memory address */
                                this->m_fileIndex*SYSTEM_FILE_ENTRY_SIZE + sizeof(FileName) + sizeof(StartAddress), /* offset */
                                (uint8_t *)&this->m_fileOffsetPointer, /* data */
                                sizeof(this->m_fileOffsetPointer)); /* size of data */
    return stat;
  }


  /**
   * @brief      Read from flash memory
   *
   * @param      buffer       The buffer
   * @param      size         The size
   * @param[in]  waitForFull  The wait for full
   *
   * @return     The file status
   */
  File::Status File::read(void * buffer, NATIVE_INT_TYPE &size, bool waitForFull) {
    Status stat = OP_OK;

    if(this->m_fileOffsetPointer + size > m_fileSize){
      size = 0;
      stat = OTHER_ERROR;
      return stat;
    }

    // read the data from flash
    S25fl064l::MemAlloc tmpAlloc;
    tmpAlloc.startAddress = this->m_fileStartAddress;
    tmpAlloc.reservedSize = this->m_fileSize;

    m_flashMem.readDataFromFlash(&tmpAlloc, /* start of allocated memory */
                                this->m_fileOffsetPointer, /*offset*/
                                (uint8_t *)buffer, /* data */
                                size); /* size of data */
    
    this->m_fileOffsetPointer += size;

    // update look-up table file current pointer and save it
    m_flashMem.writeDataToFlash(&m_headerMemAlloc, 
                                this->m_fileIndex*SYSTEM_FILE_ENTRY_SIZE + sizeof(FileName) + sizeof(StartAddress),
                                (uint8_t *)&this->m_fileOffsetPointer,
                                sizeof(this->m_fileOffsetPointer));  
    return stat;
  }


/**
 * @brief      Write to flash memory
 *
 * @param[in]  buffer       The buffer
 * @param      size         The size
 * @param[in]  waitForDone  The wait for done
 *
 * @return     The file status
 */
  File::Status File::write(const void * buffer, NATIVE_INT_TYPE &size, bool waitForDone) {
    Status stat = OP_OK;

    // Check that the data to write will overflow allocated memory
    // If so, reset file current pointer to file start address
    if(this->m_fileOffsetPointer + size > m_fileSize){
      this->m_fileOffsetPointer = this->m_fileStartAddress;
    }

    S25fl064l::MemAlloc tmpAlloc;
    tmpAlloc.startAddress = this->m_fileStartAddress;
    tmpAlloc.reservedSize = this->m_fileSize;

    m_flashMem.writeDataToFlash(&tmpAlloc, /* start of allocated memory */
                                this->m_fileOffsetPointer, /* offset */
                                (uint8_t *)buffer, /* data */
                                size);

    return stat;
  }


  // NOTE(mereweth) - see http://lkml.iu.edu/hypermail/linux/kernel/1005.2/01845.html
  // recommendation from Linus Torvalds, but doesn't seem to be that fast
  File::Status File::bulkWrite(const void * buffer, NATIVE_UINT_TYPE &totalSize,
                               NATIVE_INT_TYPE chunkSize) {

    return OP_OK;
  }


  /**
   * @brief      Flushes the object.
   *
   * @return     The file status
   */
  File::Status File::flush() {
    File::Status stat = OP_OK;
    return stat;
  }


  /**
   * @brief      close the file.
   */
  void File::close(void) {
  }


  /**
   * @brief      Gets the last error.
   *
   * @return     The last error.
   */
  NATIVE_INT_TYPE File::getLastError(void) {
    return this->m_lastError;
  }

  /**
   * @brief      Gets the last error string.
   *
   * @return     The last error string.
   */
  const char* File::getLastErrorString(void) {
    return strerror(this->m_lastError);
  }


  /**
   * @brief      Calculates the crc 32.
   *
   * @param      crc   The crc
   *
   * @return     The crc 32.
   */
  File::Status File::calculateCRC32(U32 &crc){
    return OP_OK;
  }

  /**
   * @brief      Returns the File Start Address of currently open file
   *
   * @param      none
   *
   * @return     The start address of the File
   */
  U32 File::getFileStartAddress(void)
  {
    return static_cast<U32>(this->m_fileStartAddress);
  }
}
