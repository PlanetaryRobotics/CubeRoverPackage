// ----------------------------------------------------------------------
//
// ComLogger.hpp
//
// ----------------------------------------------------------------------

#ifndef CubeRover_ComLogger_HPP
#define CubeRover_ComLogger_HPP

#include "CubeRover/ComLogger/ComLoggerComponentAc.hpp"
#include <Os/File.hpp>
#include <Os/Mutex.hpp>
#include <Fw/Types/Assert.hpp>
#include <Utils/Hash/Hash.hpp>

#include <limits.h>
#ifdef __TI_ARM__   // TI ARM doesn't define limits.h
#define NAME_MAX 255
#define PATH_MAX 4096
#define MAX_FILE_SIZE 100
#define MAX_NUM_FILES 2
#endif
#include <stdio.h>
#include <cstdarg>

namespace CubeRover {

  class ComLogger :
    public ComLoggerComponentBase
  {
      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

    public:

      // CONSTRUCTOR:
      // filePrefix: string to prepend the file name with, ie. "thermal_telemetry"
      // maxFileSize: the maximum size a file should reach before being closed and a new one opened
      // storeBufferLength: if true, store the length of each com buffer before storing the buffer itself,
      //                    otherwise just store the com buffer. false might be advantageous in a system
      //                    where you can ensure that all buffers given to the ComLogger are the same size
      //                    in which case you do not need the overhead. Or you store an id which you can
      //                    match to an expected size on the ground during post processing.
#if FW_OBJECT_NAMES == 1
      ComLogger(const char* compName, const char* filePrefix, U32 maxFileSize, bool storeBufferLength=true);
#else
      ComLogger(const char* filePrefix, U32 maxFileSize, bool storeBufferLength=true);
#endif

      void init(
          NATIVE_INT_TYPE queueDepth, //!< The queue depth
          NATIVE_INT_TYPE instance //!< The instance number
      );

      ~ComLogger(void);

      // ----------------------------------------------------------------------
      // Handler implementations
      // ----------------------------------------------------------------------

    PRIVATE:

      void comIn_handler(
          NATIVE_INT_TYPE portNum,
          Fw::ComBuffer &data,
          U32 context
      );

      void CloseFile_cmdHandler(
          FwOpcodeType opCode,
          U32 cmdSeq
      );

      //! Handler implementation for pingIn
      //!
      void pingIn_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          U32 key /*!< Value to return to pinger*/
      );

      //! Implementation for SendAllLogs command handler
      //! Sends all logs from flash to Ground
      void SendAllLogs_cmdHandler(
          const FwOpcodeType opCode, /*!< The opcode*/
          const U32 cmdSeq /*!< The command sequence number*/
      );

      //! Implementation for SendSetofLogs command handler
      //! Sends a set of logs from flash to Ground
      void SendSetofLogs_cmdHandler(
          const FwOpcodeType opCode, /*!< The opcode*/
          const U32 cmdSeq, /*!< The command sequence number*/
          U32 start, /*!< The start time for a log*/
          U32 end /*!< The end time for a log*/
      );

      // ----------------------------------------------------------------------
      // Constants:
      // ----------------------------------------------------------------------
      // The maximum size of a filename
      enum { 
        MAX_FILENAME_SIZE = NAME_MAX, // as defined in limits.h
        MAX_PATH_SIZE = PATH_MAX
      };

      // The filename data:
      U8 filePrefix[MAX_FILENAME_SIZE + MAX_PATH_SIZE];
      U32 maxFileSize = MAX_FILE_SIZE;

      // ----------------------------------------------------------------------
      // Internal state:
      // ----------------------------------------------------------------------
      enum FileMode {
          CLOSED = 0,
          OPEN = 1
      };

      //create struct to make an array of filenames
      /*struct fileList{
        U8 fileName[MAX_FILENAME_SIZE + MAX_PATH_SIZE];
      };*/

      //create array of file names
      //struct fileList file_loc[MAX_NUM_FILES];

      //keep track of earliest file
      U32 file_start;
      //keep track of most recent file
      U32 file_end;

      FileMode fileMode;
      Os::File file;
      U8 fileName[MAX_FILENAME_SIZE + MAX_PATH_SIZE];
      //U8 hashFileName[MAX_FILENAME_SIZE + MAX_PATH_SIZE];
      U32 byteCount;
      bool writeErrorOccured;
      bool readErrorOccured;
      bool openErrorOccured;
      bool storeBufferLength;
      
      // ----------------------------------------------------------------------
      // File functions:
      // ---------------------------------------------------------------------- 
      void openFile(
      );

      void closeFile(
      );

      void writeComBufferToFile(
        Fw::ComBuffer &data,
        U16 size
      );

      // ----------------------------------------------------------------------
      // Helper functions:
      // ---------------------------------------------------------------------- 
      bool writeToFile(
        void* data, 
        U16 length
      );
/*
      void writeHashFile(
      );
*/
      void readFiletoComBuffer(
        Fw::ComBuffer &data,
        U32 size
      );

      bool readFromFile(
        void* buffer,
        U32 length
      );

      void parseSeconds(
        char temp_buffer[MAX_FILENAME_SIZE + MAX_PATH_SIZE],
        U8 fileName[MAX_FILENAME_SIZE + MAX_PATH_SIZE]
      );
  };
};

#endif
