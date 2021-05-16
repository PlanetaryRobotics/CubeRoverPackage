// ----------------------------------------------------------------------
//
// ComLogger.hpp
//
// ----------------------------------------------------------------------

#ifndef CubeRover_ComLogger_HPP
#define CubeRover_ComLogger_HPP

#include "CubeRover/ComLogger/ComLoggerComponentAc.hpp"
#include <Os/Mutex.hpp>
#include <Fw/Types/Assert.hpp>
#include <limits.h>
#include <stdio.h>
#include <cstdarg>
#include <Os/FreeRTOS/lfs.h>

#define MAX_FILENAME_SIZE 8
#define MAX_LOG_FILE_SIZE 256 //Choosen as page size, could be increased/decreased as needed
#define MAX_FILE_SIZE 0x10000 // Choosen as block size, could be increased/decreased as needed

namespace CubeRover {

  class ComLogger :
    public ComLoggerComponentBase
  {
      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

    ComLoggerComponentImpl(
#if FW_OBJECT_NAMES == 1
          const char *const compName /*!< The component name*/
#else
          void
#endif
      );

    public:

      void init(
          //NATIVE_INT_TYPE queueDepth, //!< The queue depth
          NATIVE_INT_TYPE instance //!< The instance number
      );

      ComLogger(void);

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
      //void pingIn_handler(
          //const NATIVE_INT_TYPE portNum, /*!< The port number*/
          //U32 key /*!< Value to return to pinger*/
      //);

      //! Implementation for SendLog command handler
      //! Sends a log from flash to Ground
      void SendLog_cmdHandler(
          const FwOpcodeType opCode, /*!< The opcode*/
          const U32 cmdSeq, /*!< The command sequence number*/
          const Fw::CmdStringArg& prefix, /*!< The prefix for a flash saved file*/
          U32 time /*!< The time (seconds) for when a file was opened*/
      );

      // ----------------------------------------------------------------------
      // Constants:
      // ----------------------------------------------------------------------
      // The maximum size of a filename
      S25fl064l flash_chip;

      lfs_t lfs;
      lfs_file_t file;

      const struct lfs_config cfg = {
        .read = lfs_read,
        .prog = lfs_prog,
        .erase = lfs_erase,
        .sync = lfs_sync,

        .context = flash_chip,  // Set the correct flash chip instanciation
        .read_size = 8, // Min read size
        .prog_size = 8, // Min write size, Flash allows for single bit programming, making minimum 8 bytes to match read
        .block_size = 0x10000, // 64KB
        .block_count = 128, // 128 blocks of 64KB each (from FLASH datasheet), ~8MB total? Doesn't make sense as we have 64MB, maybe we have 8 sections of 128 blocks each?
        .cache_size = 256, // Kinda guessed? Just made it the max possible page that we can save
        .lookahead_size = 0, // no idea, don't think flash has one so set to zero?
        .block_cycles = -1,  // disable wear-leveling
        .name_max = 8,  // Max file name is 3 byte char + '_' 1 bytes + U32 (4 byte) time (seconds) = 8 bytes
        .file_max = MAX_FILE_SIZE // Max file size allowed
      };

      // ----------------------------------------------------------------------
      // Internal state:
      // ----------------------------------------------------------------------
      enum FileMode {
          CLOSED = 0,
          OPEN = 1
      };

      enum FileType{
          log = 0,
          cam = 1,
          ukn = 2
      };

      // Total number of bytes read
      U32 bytes_read;
      // Total number of bytes written
      U32 bytes_written;

      //keep track of earliest file
      U32 file_start;
      U32 file_start_add;
      //keep track of most recent file
      U32 file_end;
      U32 file_end_add;

      FileMode fileMode;
      FileType fileType;
      U32 fileByteCount;

      // Reading Buffer
      U8 m_read_buffer[MAX_FILE_SIZE];
      
      // ----------------------------------------------------------------------
      // File functions:
      // ---------------------------------------------------------------------- 
      void openFile(
        char prefix [3]
      );

      bool closeFile(
      );
      // ----------------------------------------------------------------------
      // Helper functions:
      // ---------------------------------------------------------------------- 
      void writeToFile(
        void* data, 
        U32 length,
        char prefix [3],
        U32 time
      );
      // Overloaded version with no prefix or time
      void writeToFile(
        void* data, 
        U32 length,
      );

      U32 readFromFile(
        void* buffer,
        char prefix [3],
        U32 time
      );

      FileType prefixToType(
        char prefix [3]
      );

      int lfs_read(
        const struct lfs_config *cfg, 
        lfs_block_t block,
        lfs_offset offset,
        void *buffer,
        lfs_size_t size
      );

      int lfs_prog(
        const struct lfs_config *cfg, 
        lfs_block_t block,
        lfs_offset offset,
        void *buffer,
        lfs_size_t size
      );

      int lfs_erase(
        const struct lfs_config *cfg, 
        lfs_block_t block
      );

      int lfs_sync(
        const struct lfs_config *cfg
      );
  };
};

#endif
