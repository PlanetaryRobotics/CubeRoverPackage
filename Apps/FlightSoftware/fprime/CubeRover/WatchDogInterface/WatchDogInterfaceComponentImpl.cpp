// ======================================================================
// \title  WatchDogInterfaceComponentImpl.cpp
// \author alec
// \brief  cpp file for WatchDogInterface component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================


#include <CubeRover/WatchDogInterface/WatchDogInterfaceComponentImpl.hpp>
#include "Fw/Types/BasicTypes.hpp"
#include <stdio.h>
#include <string.h>

namespace CubeRover {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  WatchDogInterfaceComponentImpl ::
#if FW_OBJECT_NAMES == 1
    WatchDogInterfaceComponentImpl(
        const char *const compName
    ) :
      WatchDogInterfaceComponentBase(compName)
#else
    WatchDogInterfaceComponentImpl(void)
#endif
  {

  }

  void WatchDogInterfaceComponentImpl ::
    init(
        const NATIVE_INT_TYPE queueDepth,
        const NATIVE_INT_TYPE instance
    )
  {
    WatchDogInterfaceComponentBase::init(queueDepth, instance);
  }

  WatchDogInterfaceComponentImpl ::
    ~WatchDogInterfaceComponentImpl(void)
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void WatchDogInterfaceComponentImpl ::
    PingIn_handler(
        const NATIVE_INT_TYPE portNum,
        U32 key
    )
  {
    this->PingOut_out(0, key);
  }

  void WatchDogInterfaceComponentImpl ::
    Run_handler(
        const NATIVE_INT_TYPE portNum,
        NATIVE_UINT_TYPE context
    )
  {
    // This run handler happens every 1-100 hz (overview says 100 hz, specific says 1 hz)
    // Sends U16 to the watchdog as defined in design document. Checks stroke value as to what to send watchdog
    // May need to wait to send command everytime it runs as could be a lot of commands
	// Create watchdog stroke equal to 1 as first bit 1 to tell watchdog scheduled, rest bits 0 to not reset anything
    U8 watchdog_stroke = 0x00;
    // Send first byte of stroke
    linSend(linREG, (uint8_t *)&watchdog_stroke);
    // Check for Response from MSP430 Watchdog (done twice for each byte)
    U16 watchdog_reponse_1;
    linGetData(linREG, (uint8_t *)&watchdog_reponse_1);
    // Check for timeout of response
    U32 comm_error = linGetStatusFlag(linREG);
    if(comm_error == LIN_TO_INT)
    {
    	this->log_WARNING_HI_WatchDogTimedOut();
    }
    // Check that response is the same as what was sent
    if(watchdog_reponse_1 != watchdog_stroke)
    {
    	this->log_WARNING_HI_WatchDogMSP430IncorrectResp();
    }
	// Check for any watchdog errors (NOTE: not sure that comm_error should be zero if no errors)
    if(comm_error)
    {
    	this->log_WARNING_HI_WatchDogCommError(comm_error);
    }
  }

  void WatchDogInterfaceComponentImpl ::
    WdogStroke_handler(
        const NATIVE_INT_TYPE portNum,
        U32 code
    )
  {
    // TODO
    // Happens everytime Health sends a watchdog code to Watchdog interface
    // Watchdog code will have info on what components status is (MUST FIGURE OUT HOW CODE WORKS)
  }

  // ----------------------------------------------------------------------
  // Command handler implementations
  // ----------------------------------------------------------------------

  void WatchDogInterfaceComponentImpl ::
    Reset_Specific_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        U8 reset_value
    )
  {
  	// Send Activity Log/tlm to know watchdog recieved command
  	char command_type[32] = "Reset Specific:";
  	char reset_val_char[8];
  	sprintf(reset_val_char, "%u", reset_value);
  	strcat(command_type, reset_val_char);
  	Fw::LogStringArg command_type_log = command_type;
  	Fw::TlmString command_type_tlm = command_type;
  	this->log_ACTIVITY_HI_WatchDogCmdReceived(command_type_log);
  	this->tlmWrite_LAST_COMMAND(command_type_tlm);

    // Sends a command to watchdog to reset specified devices. Can be hardware through watchdog or component

    // If reset_value is greater than 0x20, we are resetting a software component
    if(reset_value > 0x1B)
    {
      // Reset Components (UNDECIDED YET)
      // TODO
    }
    // If reset_value less than or equal to 0x20, we are resetting hardware
    else
    {
      	// Copy first byte of reset value
      	U8 watchdog_reset_byte1 = reset_value;
      	// Send watchdog_reset to MSP430 watchdog
      	linSend(linREG, (uint8_t *)&watchdog_reset_byte1);
		// Check for first byte Response from MSP430 Watchdog
	    U8 watchdog_reponse_byte1;
	    linGetData(linREG, (uint8_t *)&watchdog_reponse_byte1);
	    // Check for timeout of response
	    U32 comm_error = linGetStatusFlag(linREG);
	    if(comm_error == LIN_TO_INT)
	    {
	    	this->log_WARNING_HI_WatchDogTimedOut();
	    }
	    // Check that response is the same as what was sent
	    if(watchdog_reponse_byte1 != watchdog_reset_byte1)
	    {
	    	this->log_WARNING_HI_WatchDogMSP430IncorrectResp();
	    }
		// Check for any watchdog errors (NOTE: not sure that comm_error should be zero if no errors)
	    if(comm_error)
	    {
	    	this->log_WARNING_HI_WatchDogCommError(comm_error);
	    }
    }
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

} // end namespace CubeRover
