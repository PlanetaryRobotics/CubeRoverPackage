// ====================================================================== 
// \title  PingReceiverImpl.hpp
// \author tim
// \brief  hpp file for PingReceiver component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
// 
// ====================================================================== 

#ifndef PingReceiver_HPP
#define PingReceiver_HPP

#include "CubeRover/PingReceiver/PingReceiverComponentAc.hpp"

namespace CubeRover{

  class PingReceiverComponentImpl :
    public PingReceiverComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

      //! Construct object PingReceiver
      //!
      PingReceiverComponentImpl(
#if FW_OBJECT_NAMES == 1
          const char *const compName /*!< The component name*/
#else
          void
#endif
      );

      //! Initialize object PingReceiver
      //!
      void init(
          const NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
          const NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
      );

      //! Destroy object PingReceiver
      //!
      ~PingReceiverComponentImpl(void);

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for PingIn
      //!
      void PingIn_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          U32 key /*!< Value to return to pinger*/
      );

      void PR_StopPings_cmdHandler(
              FwOpcodeType opCode, /*!< The opcode*/
              U32 cmdSeq /*!< The command sequence number*/
          );

      bool m_inhibitPings;
      U32 m_pingsRecvd;


    };

} // end namespace Ref

#endif
