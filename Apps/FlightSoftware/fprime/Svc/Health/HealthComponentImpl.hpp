// ======================================================================
// \title  HealthComponentImpl.hpp
// \author abby
// \brief  hpp file for Health component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef Health_HPP
#define Health_HPP

#include "Svc/Health/HealthComponentAc.hpp"

namespace Svc {

  class HealthComponentImpl :
    public HealthComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

      //! Construct object Health
      //!
      HealthComponentImpl(
#if FW_OBJECT_NAMES == 1
          const char *const compName /*!< The component name*/
#else
          void
#endif
      );

      //! Initialize object Health
      //!
      void init(
          const NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
          const NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
      );

      void doOtherChecks();

      //! Destroy object Health
      //!
      ~HealthComponentImpl(void);

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for PingReturn
      //!
      void PingReturn_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          U32 key /*!< Value to return to pinger*/
      );

      //! Handler implementation for Run
      //!
      void Run_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          NATIVE_UINT_TYPE context /*!< The call order*/
      );

    PRIVATE:

      // ----------------------------------------------------------------------
      // Command handler implementations
      // ----------------------------------------------------------------------

      //! Implementation for HLTH_ENABLE command handler
      //! A command to enable or disable health checks
      void HLTH_ENABLE_cmdHandler(
          const FwOpcodeType opCode, /*!< The opcode*/
          const U32 cmdSeq, /*!< The command sequence number*/
          HealthEnabled enable /*!< whether or not health checks are enabled*/
      );

      //! Implementation for HLTH_PING_ENABLE command handler
      //! Ignore a particular ping entry
      void HLTH_PING_ENABLE_cmdHandler(
          const FwOpcodeType opCode, /*!< The opcode*/
          const U32 cmdSeq, /*!< The command sequence number*/
          const Fw::CmdStringArg& entry, /*!< The entry to enable/disable*/
          PingEnabled enable /*!< whether or not a port is pinged*/
      );

      //! Implementation for HLTH_CHNG_PING command handler
      //! Change ping value
      void HLTH_CHNG_PING_cmdHandler(
          const FwOpcodeType opCode, /*!< The opcode*/
          const U32 cmdSeq, /*!< The command sequence number*/
          const Fw::CmdStringArg& entry, /*!< The entry to modify*/
          U32 warningValue, /*!< Ping warning threshold*/
          U32 fatalValue /*!< Ping fatal threshold*/
      );


    };

} // end namespace Svc

#endif
