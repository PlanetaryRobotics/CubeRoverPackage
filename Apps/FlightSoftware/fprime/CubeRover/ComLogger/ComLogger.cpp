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
    for(U32 file_index = this->file_start; file_index <= this->file_end; ++file_index)
    {
        // Create a ComBuffer called data to store logs read from flash
        Fw::ComBuffer data;

        // Create string to search for in file
        char file_name[MAX_FILENAME_SIZE];
        sprintf(file_name, "%d", file_index);
        strcat(file_name, ".com");

        // Open the file designated by file_index
        Os::File::Status ret = file.open(file_name, Os::File::OPEN_READ);

         if( Os::File::OP_OK != ret && Os::File::DOESNT_EXIST != ret) {
            if( !openErrorOccured ) { // throttle this event, otherwise a positive 
                                      // feedback event loop can occur!
              Fw::LogStringArg logStringArg((char*) this->fileName);
              this->log_WARNING_HI_FileOpenError(ret, logStringArg);
            }
            openErrorOccured = true;
          }
          else if(Os::File::DOESNT_EXIST == ret) {
            // File doesn't exist, do nothing
            // Reset event throttle:
            openErrorOccured = false;
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
  	U32 true_start = this->file_start;
	U32 true_end = this->file_end;
	//check if start value is valid
	if(start >= this->file_start && start <= this->file_end)
		true_start = start;
	//check if start is above file end
	else if(start > this->file_end)
	{
		this->log_WARNING_LO_TimeNotAvaliable(start, end);
		//return as start time not valid
		return;
	}
	else
		this->log_WARNING_LO_TimeNotAvaliable(start, end);

	//check if end value is valid
	if(end >= this->file_start && end <= this->file_end)
		true_end = end;
	//check if end is below file start
	else if(end < this->file_start)
	{
		this->log_WARNING_LO_TimeNotAvaliable(start, end);
		//return as end time not valid
		return;
	}
	else
		this->log_WARNING_LO_TimeNotAvaliable(start, end);

    // Go through all files and send the contents to Ground
    for(U32 file_index = true_start; file_index != true_end; ++file_index)
    {
          // Create a ComBuffer called data to store logs read from flash
          Fw::ComBuffer data;

          // Create string to search for in file
          char file_name[MAX_FILENAME_SIZE];
          sprintf(file_name, "%d", file_index);
          strcat(file_name, ".com");

          // Open the file designated by file_index
          Os::File::Status ret = file.open(file_name, Os::File::OPEN_READ);

          if( Os::File::OP_OK != ret && Os::File::DOESNT_EXIST != ret) {
            if( !openErrorOccured ) { // throttle this event, otherwise a positive 
                                      // feedback event loop can occur!
              Fw::LogStringArg logStringArg((char*) this->fileName);
              this->log_WARNING_HI_FileOpenError(ret, logStringArg);
            }
            openErrorOccured = true;
          }
          else if( Os::File::DOESNT_EXIST == ret) {
            // File doesn't exist, do nothing
            // Reset event throttle:
            openErrorOccured = false;
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
    bytesCopied = snprintf((char*) this->fileName, sizeof(this->fileName), "%d.com", 
      timestamp.getSeconds());

    // "A return value of size or more means that the output was truncated"
    // See here: http://linux.die.net/man/3/snprintf
    FW_ASSERT( bytesCopied < sizeof(this->fileName) );

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

      // Set file start/end to time
      if(this->file_start == 0 && this->file_end == 0)
      {
      	this->file_start = timestamp.getSeconds();
      	this->file_start_add = file.getFileStartAddress();
      	this->file_end = timestamp.getSeconds();
      	this->file_end_add = file.getFileStartAddress();
      	//must return or will pass looped in memory code
      	return;
      }
      // Otherwise update file end 
      else
      {
      	this->file_end = timestamp.getSeconds();
      	this->file_end_add = file.getFileStartAddress();
      }

      // Check if we have looped in memory, if so then we need to update file_start to the next file
      if(this->file_start_add >= this->file_end_add)
      {
      	U32 next_file_start = (this->file_start)++;
      	// Create string to search for in file
        char file_name[MAX_FILENAME_SIZE];
        sprintf(file_name, "%d", next_file_start);
        strcat(file_name, ".com");

		// Loop through files until we find the next file that exists
      	while(file.open(file_name, Os::File::OPEN_READ) == Os::File::DOESNT_EXIST)
      	{
      		next_file_start++;
      		sprintf(file_name, "%d", next_file_start);
        	strcat(file_name, ".com");
      	}

      	// Update file_start
      	this->file_start = next_file_start;
      	this->file_start_add = file.getFileStartAddress();

      	// Reopen file we wanted
      	file.open((char*) this->fileName, Os::File::OPEN_WRITE);
      }
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
  };
}
