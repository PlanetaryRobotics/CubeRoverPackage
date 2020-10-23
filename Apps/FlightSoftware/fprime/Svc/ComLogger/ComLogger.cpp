// ----------------------------------------------------------------------
//
// ComLogger.cpp
//
// ----------------------------------------------------------------------

#include <Svc/ComLogger/ComLogger.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/SerialBuffer.hpp>
#include <Os/ValidateFile.hpp>
#include <iostream>
#include <stdio.h>

using namespace std;

namespace Svc {

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
      this->writeHashFile();

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
    U16 size = size32 & 0xFFFF

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

  void ComLoggerComponentImpl ::
    SendAllLogs_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq
    )
  {
    // TODO
    // Get logs from flash

    //Create a ComBuffer called data to store logs read from flash
    Fw::ComBuffer data;

    // Open the file if it there is not one open:
    if( CLOSED == this->fileMode ){
      this->openFile();
    }

    // If file is Open, then we read to the ComBuffer Data
    if( OPEN == this->fileMode ) {
      this->readFiletoComBuffer(&data, this->byteCount);
    }

    // Put logs into ground output buffer and send them out
    // *NOTE* All logs should still be in serialized format since they were stored in serialized format
    if (this->isConnected_GndOut_OutputPort(0)) {
            this->GndOut_out(0, data,0);
        }

    // Send Output buffer to ground
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void ComLoggerComponentImpl ::
    SendSetofLogs_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        U32 start,
        U32 end
    )
  {
    // TODO
    // Get specific logs from flash

    //Create a ComBuffer called data to store logs read from flash, deserialized_buffer for deserializing, output_data for the specific logs we need
    Fw::ComBuffer data;
    //Fw::ComBuffer deserial_data;
    Fw::ComBuffer output_data;

    // Open the file if it there is not one open:
    if( CLOSED == this->fileMode ){
      this->openFile();
    }

    // If file is Open, then we read to the ComBuffer Data
    if( OPEN == this->fileMode ) {
      this->readFiletoComBuffer(&data, this->byteCount);
    }

    // Deserialize Data from Flash so we can read it
    data.deserialize(data);

    // Go through all logs and search for Logs between start and end
    U8 current_address = data.getBuffAddr();
    U8 ending_add = data.getBuffAddr() + data.getBuffLength();

    for(current_address; current_address+= sizeof(Fw::LogPacket); current_address >= ending_add)
    {
      // Convert address at current_address to a LogPacket
      Fw::LogPacket singluar_log = (Fw::LogPacket) current_address;
      //Get the TimeTag then Seconds from the LogPacket
      U32 singular_log_time = singluar_log.getTimeTag().getSeconds();

      if(singular_log_time >= start && singular_log_time <= end)
      {
        //copy data from deserialized_data into output_data
      }
    }
    // Serialize output_data so we can send it to ground
    output_data.serialize(output_data);

    // Must figure out how we know what times logs are at
    // What is the current table of files and how are they stored?
    // Do I need to associate a time to a file or no? How can I find specific times?


    // Put logs into ground output buffer and send them out
    if (this->isConnected_GndOut_OutputPort(0)) {
            this->GndOut_out(0, output_data, 0);
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
    bytesCopied = snprintf((char*) this->fileName, sizeof(this->fileName), "%s_%d_%d_%06d.com", 
      this->filePrefix, (U32) timestamp.getTimeBase(), timestamp.getSeconds(), timestamp.getUSeconds());

    // "A return value of size or more means that the output was truncated"
    // See here: http://linux.die.net/man/3/snprintf
    FW_ASSERT( bytesCopied < sizeof(this->fileName) );

    // Create sha filename:
    bytesCopied = snprintf((char*) this->hashFileName, sizeof(this->hashFileName), "%s_%d_%d_%06d.com%s", 
      this->filePrefix, (U32) timestamp.getTimeBase(), timestamp.getSeconds(), timestamp.getUSeconds(), Utils::Hash::getFileExtensionString());
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
      this->writeHashFile();

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
}

  bool ComLogger :: 
    readFromFile(
      void* buffer,
      U16 length
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
      U16 size
    )
  {
    // Read file to buffer:
    writeToFile(data.getBuffAddr(), size);
  };