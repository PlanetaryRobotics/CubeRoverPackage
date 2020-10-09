// ======================================================================
// \title  HealthComponentImpl.cpp
// \author abby
// \brief  cpp file for Health component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================


#include <Svc/Health/HealthComponentImpl.hpp>
#include "Fw/Types/BasicTypes.hpp"

namespace Svc {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  HealthComponentImpl ::
#if FW_OBJECT_NAMES == 1
    HealthComponentImpl(
        const char *const compName
    ) :
      HealthComponentBase(compName)
#else
    HealthComponentImpl(void)
#endif
  {

  }

  void HealthComponentImpl ::
    init(
        const NATIVE_INT_TYPE queueDepth,
        const NATIVE_INT_TYPE instance
    )
  {
    HealthComponentBase::init(queueDepth, instance);
  }

  HealthComponentImpl ::
    ~HealthComponentImpl(void)
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void HealthComponentImpl ::
    PingReturn_handler(
        const NATIVE_INT_TYPE portNum,
        U32 key
    )
  {
    // TODO
  }

  void HealthComponentImpl ::
    Run_handler(
        const NATIVE_INT_TYPE portNum,
        NATIVE_UINT_TYPE context
    )
  {
    // TODO
  }

  // ----------------------------------------------------------------------
  // Command handler implementations
  // ----------------------------------------------------------------------

  void HealthComponentImpl ::
    HLTH_ENABLE_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        HealthEnabled enable
    )
  {
    // TODO
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void HealthComponentImpl ::
    HLTH_PING_ENABLE_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        const Fw::CmdStringArg& entry,
        PingEnabled enable
    )
  {
    // TODO
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void HealthComponentImpl ::
    HLTH_CHNG_PING_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        const Fw::CmdStringArg& entry,
        U32 warningValue,
        U32 fatalValue
    )
  {
    // TODO
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void HealthComponentImpl::doOtherChecks(void) {
        // empty
  }

} // end namespace Svc
