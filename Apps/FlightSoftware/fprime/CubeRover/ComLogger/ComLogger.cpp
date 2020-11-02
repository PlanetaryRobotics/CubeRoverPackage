// ----------------------------------------------------------------------
//
// ComLogger.cpp
//
// ----------------------------------------------------------------------

#include <CubeRover/ComLogger/ComLogger.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/SerialBuffer.hpp>
#include <Os/ValidateFile.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


namespace CubeRover {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction 
  // ----------------------------------------------------------------------

  ComLogger ::
#if FW_OBJECT_NAMES == 1
    ComLogger(const char* compName, const char* incomingFilePrefix, U32 maxFileSize, bool storeBufferLength) :
    ComLoggerComponentBase(compName), 
#else
    ComLogger(const char* incomingFilePrefix, U32 maxFileSize, bool storeBufferLength) :
#endif
    maxFileSize(maxFileSize),
    fileMode(CLOSED), 
    byteCount(0),
    writeErrorOccured(false),
    readErrorOccured(false),
    openErrorOccured(false),
    storeBufferLength(storeBufferLength)
  {
    if( this->storeBufferLength ) {
      FW_ASSERT(maxFileSize > sizeof(U16), maxFileSize); // must be a positive integer greater than buffer length size
    }
    else {
      FW_ASSERT(maxFileSize > sizeof(0), maxFileSize); // must be a positive integer
    }
    FW_ASSERT((NATIVE_UINT_TYPE)strnlen(incomingFilePrefix, sizeof(this->filePrefix)) < sizeof(this->filePrefix), 
      (NATIVE_UINT_TYPE) strnlen(incomingFilePrefix, sizeof(this->filePrefix)), (NATIVE_UINT_TYPE) sizeof(this->filePrefix)); // ensure that file prefix is not too big

    // Set the file prefix:
    memset(this->filePrefix, 0, sizeof(this->filePrefix)); // probably unnecesary, but I am paranoid.
    U8* dest = (U8*) strncpy((char*) this->filePrefix, incomingFilePrefix, sizeof(this->filePrefix));
    FW_ASSERT(dest == this->filePrefix, reinterpret_cast<U64>(dest), reinterpret_cast<U64>(this->filePrefix));
    this->file_start = 0;
    this->file_end = 0;
  }

  void ComLogger :: 
    init(
      NATIVE_INT_TYPE queueDepth, //!< The queue depth
      NATIVE_INT_TYPE instance //!< The instance number
    )
  {
    ComLoggerComponentBase::init(queueDepth, instance);
  }

  ComLogger ::
    ~ComLogger(void)
  {
    // Close file:
    // this->closeFile();
    // NOTE: the above did not work because we don't want to issue an event
    // in the destructor. This can cause "virtual method called" segmentation 
    // faults.
    // So I am copying part of that function here.
    if( OPEN == this->fileMode ) {
      // Close file:
      this->file.close();

      // Write out the hash file to disk:
      //this->writeHashFile();

      // Update mode:
      this->fileMode = CLOSED;

      // Send event:
      //Fw::LogStringArg logStringArg((char*) fileName);
      //this->log_DIAGNOSTIC_FileClosed(logStringArg);
    }
  }

  // ----------------------------------------------------------------------
  // Handler implementations
  // ----------------------------------------------------------------------

  void ComLogger ::
    comIn_handler(
        NATIVE_INT_TYPE portNum,
        Fw::ComBuffer &data,
        U32 context
    )
  {
    FW_ASSERT(portNum == 0);

    // Get length of buffer:
    U32 size32 = data.getBuffLength();
    // ComLogger only writes 16-bit sizes to save space 
    // on disk:
    FW_ASSERT(size32 < 65536, size32);
    U16 size = size32 & 0xFFFF;

    // Close the file if it will be too big:
    if( OPEN == this->fileMode ) {
      U32 projectedByteCount = this->byteCount + size;
      if( this->storeBufferLength ) {
        projectedByteCount += sizeof(size);
      }
      if( projectedByteCount > this->maxFileSize ) {
        this->closeFile();
      }
    }

    // Open the file if it there is not one open:
    if( CLOSED == this->fileMode ){
      this->openFile();
    }

    // Write to the file if it is open:
    if( OPEN == this->fileMode ) {
      this->writeComBufferToFile(data, size);
    }
  }

  void ComLogger :: 
    CloseFile_cmdHandler(
      FwOpcodeType opCode,
      U32 cmdSeq
    )
  {
    this->closeFile();
    this->cmdResponse_out(opCode, cmdSeq, Fw::COMMAND_OK);
  }

  void ComLogger ::
    SendAllLogs_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq
    )
  {
    // Get logs from flash

    // Go through all files and send the contents to Ground
    for(U32 file_index = this->file_start; file_index != this->file_end; file_index++)
    {
        //check if file_index is larger than array
        if(file_index >= MAX_NUM_FILES)
          file_index = 0; 

        // Create a ComBuffer called data to store logs read from flash
        Fw::ComBuffer data;

        // Open the file designated by file_index
        Os::File::Status ret = file.open((char*) this->file_loc[file_index].fileName, Os::File::OPEN_READ);

        if( Os::File::OP_OK != ret ) {
          if( !openErrorOccured ) { // throttle this event, otherwise a positive 
                                    // feedback event loop can occur!
            Fw::LogStringArg logStringArg((char*) this->fileName);
            this->log_WARNING_HI_FileOpenError(ret, logStringArg);
          }
          openErrorOccured = true;
        } 
        else {
          // Reset event throttle:
          openErrorOccured = false;

          // If file is Open, then we read to the ComBuffer Data
          this->readFiletoComBuffer(data, this->maxFileSize);
          
          // Put logs into ground output buffer and send them out
          // *NOTE* All logs should still be in serialized format since they were stored in serialized format
          if (this->isConnected_GndOut_OutputPort(0)) {
                  this->GndOut_out(0, data,0);
              }
        }
    }
    // Send Output buffer to ground
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void ComLogger ::
    SendSetofLogs_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        U32 start,
        U32 end
    )
  {
    char time_buff[MAX_FILENAME_SIZE + MAX_PATH_SIZE];
    // Parse time for earliest log
    parseSeconds(time_buff, this->file_loc[this->file_start].fileName);
    // Convert time to U32
    U32 start_time = static_cast<U32> (atoi(time_buff));

    //clear time_buff
    memset(time_buff, 0, sizeof(time_buff));

    // Parse time for most recent log
    parseSeconds(time_buff, this->file_loc[this->file_end].fileName);
    // Convert time to U32
    U32 end_time = static_cast<U32> (atoi(time_buff));

    if(start < start_time || end > end_time)
    {
      Fw::LogStringArg logStringArg1((char*) start);
      Fw::LogStringArg logStringArg2((char*) end);
      this->log_WARNING_LO_TimeNotAvaliable(logStringArg1, logStringArg2);
    }

    // Go through all files and send the contents to Ground
    for(U32 file_index = this->file_start; file_index != this->file_end; file_index++)
    {
        //check if file_index is larger than array
        if(file_index >= MAX_NUM_FILES)
          file_index = 0; 
        
        //clear time_buff
        memset(time_buff, 0, sizeof(time_buff));

        // Parse time for current index log
        parseSeconds(time_buff, this->file_loc[file_index].fileName);
        // Convert time to U32
        U32 index_time = static_cast<U32> (atoi(time_buff));

        //check if index is within start and end 
        if(index_time >= start && index_time <= end)
        {
          // Create a ComBuffer called data to store logs read from flash
          Fw::ComBuffer data;

          // Open the file designated by file_index
          Os::File::Status ret = file.open((char*) this->file_loc[file_index].fileName, Os::File::OPEN_READ);

          if( Os::File::OP_OK != ret ) {
            if( !openErrorOccured ) { // throttle this event, otherwise a positive 
                                      // feedback event loop can occur!
              Fw::LogStringArg logStringArg((char*) this->fileName);
              this->log_WARNING_HI_FileOpenError(ret, logStringArg);
            }
            openErrorOccured = true;
          } 
          else {
            // Reset event throttle:
            openErrorOccured = false;

            // If file is Open, then we read to the ComBuffer Data
            this->readFiletoComBuffer(data, this->maxFileSize);
            
            // Put logs into ground output buffer and send them out
            // *NOTE* All logs should still be in serialized format since they were stored in serialized format
            if (this->isConnected_GndOut_OutputPort(0)) {
                    this->GndOut_out(0, data,0);
                }
          }
        }
    }

    // Send Output buffer to ground
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void ComLogger ::
    pingIn_handler(
        const NATIVE_INT_TYPE portNum,
        U32 key
    )
  {
      // return key
      this->pingOut_out(0,key);
  }

  void ComLogger ::
    openFile(
    )
  {
    FW_ASSERT( CLOSED == this->fileMode );

    U32 bytesCopied;

    // Create filename:
    Fw::Time timestamp = getTime();
    memset(this->fileName, 0, sizeof(this->fileName));
    bytesCopied = snprintf((char*) this->fileName, sizeof(this->fileName), "%d_%d.com", 
      timestamp.getSeconds(), (U32) timestamp.getTimeBase());

    // "A return value of size or more means that the output was truncated"
    // See here: http://linux.die.net/man/3/snprintf
    FW_ASSERT( bytesCopied < sizeof(this->fileName) );

    // Create sha filename:
    bytesCopied = snprintf((char*) this->hashFileName, sizeof(this->hashFileName), "%d_%d.com%s", 
      timestamp.getSeconds(), (U32) timestamp.getTimeBase(), Utils::Hash::getFileExtensionString());
    FW_ASSERT( bytesCopied < sizeof(this->hashFileName) );

    Os::File::Status ret = file.open((char*) this->fileName, Os::File::OPEN_WRITE);
    if( Os::File::OP_OK != ret ) {
      if( !openErrorOccured ) { // throttle this event, otherwise a positive 
                                // feedback event loop can occur!
        Fw::LogStringArg logStringArg((char*) this->fileName);
        this->log_WARNING_HI_FileOpenError(ret, logStringArg);
      }
      openErrorOccured = true;
    } else {
      // Reset event throttle:
      openErrorOccured = false;

      // Reset byte count:
      this->byteCount = 0;

      // Set mode:
      this->fileMode = OPEN; 

      // Copy fileName into file location list at the file_end spot 
      memcpy(&this->file_loc[this->file_end].fileName, &this->fileName, sizeof(this->fileName));

      //increment file_end for next open file
      this->file_end++;

      //check if file_end is greater than the array, if so, we set it back to zero
      if(this->file_end >= MAX_NUM_FILES)
          this->file_end = 0;

      //check if file_end and file_start are equal, increment file_start
      if(this->file_start == this->file_end)
          this->file_start++;

      //check if file_start is greater than the array
      if(this->file_start >= MAX_NUM_FILES)
          this->file_start = 0;
    }    
  }

  void ComLogger ::
    closeFile(
    )
  {
    if( OPEN == this->fileMode ) {
      // Close file:
      this->file.close();

      // Write out the hash file to disk:
      //this->writeHashFile();

      // Update mode:
      this->fileMode = CLOSED;

      // Send event:
      Fw::LogStringArg logStringArg((char*) this->fileName);
      this->log_DIAGNOSTIC_FileClosed(logStringArg);
    }
  }

  void ComLogger ::
    writeComBufferToFile(
      Fw::ComBuffer &data,
      U16 size
    )
  {
    if( this->storeBufferLength ) {
      U8 buffer[sizeof(size)];
      Fw::SerialBuffer serialLength(&buffer[0], sizeof(size)); 
      serialLength.serialize(size);
      if(writeToFile(serialLength.getBuffAddr(), serialLength.getBuffLength())) {
        this->byteCount += serialLength.getBuffLength();
      }
      else {
        return;
      }
    }

    // Write buffer to file:
    if(writeToFile(data.getBuffAddr(), size)) {
      this->byteCount += size;
    }
  }

  bool ComLogger ::
    writeToFile(
      void* data, 
      U16 length
    )
  {
    NATIVE_INT_TYPE size = length;
    Os::File::Status ret = file.write(data, size);
    if( Os::File::OP_OK != ret || size != (NATIVE_INT_TYPE) length ) {
      if( !writeErrorOccured ) { // throttle this event, otherwise a positive 
                                 // feedback event loop can occur!
        Fw::LogStringArg logStringArg((char*) this->fileName);
        this->log_WARNING_HI_FileWriteError(ret, size, length, logStringArg);
      }
      writeErrorOccured = true;
      return false;
    }

    writeErrorOccured = false;
    return true;
  }
/*
  void ComLogger :: 
    writeHashFile(
    )
  {
    Os::ValidateFile::Status validateStatus;
    validateStatus = Os::ValidateFile::createValidation((char*) this->fileName, (char*)this->hashFileName);
    if( Os::ValidateFile::VALIDATION_OK != validateStatus ) {
      Fw::LogStringArg logStringArg1((char*) this->fileName);
      Fw::LogStringArg logStringArg2((char*) this->hashFileName);
      this->log_WARNING_LO_FileValidationError(logStringArg1, logStringArg2, validateStatus);
    }
  }
*/

  bool ComLogger :: 
    readFromFile(
      void* buffer,
      U32 length
    )
  {
    NATIVE_INT_TYPE size = length;
    Os::File::Status ret = file.read(buffer, size);
    if( Os::File::OP_OK != ret || size != (NATIVE_INT_TYPE) length ) {
      if( !readErrorOccured ) { // throttle this event, otherwise a positive 
                                 // feedback event loop can occur!
        Fw::LogStringArg logStringArg((char*) this->fileName);
        this->log_WARNING_HI_FileReadError(ret, size, logStringArg);
      }
      readErrorOccured = true;
      return false;
    }

    readErrorOccured = false;
    return true;
  }

  void ComLogger ::
    readFiletoComBuffer(
      Fw::ComBuffer &data,
      U32 size
    )
  {
    // Read file to buffer:
    readFromFile(data.getBuffAddr(), size);
  }

  void ComLogger ::
      parseSeconds(
        char temp_buffer[MAX_FILENAME_SIZE + MAX_PATH_SIZE],
        U8 fileName[MAX_FILENAME_SIZE + MAX_PATH_SIZE]
      )
  {
      for(unsigned int i = 0; i < sizeof(fileName); i++)
      {
          char temp_char = (char)fileName[i];
          if(temp_char == '_')
              return;
          else
              temp_buffer[i] = temp_char;
      }
  };
}
