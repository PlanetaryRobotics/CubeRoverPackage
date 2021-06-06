// ======================================================================
// \title  WatchDogInterfaceComponentImpl.cpp
// \author alec, justin, mschnur
// \brief  cpp file for WatchDogInterface component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================


#include <CubeRover/WatchDogInterface/WatchDogInterface.hpp>
#include <Drv/FreeRtosSerialDriver/FreeRtosSerialDriverComponentImpl.hpp>
#include "Fw/Types/BasicTypes.hpp"
#include <stdio.h>
#include <string.h>

#include "Include/FswPacket.hpp"
#include "sys_dma.h"
#include <App/DMA.h>
#include "gio.h"
#include "reg_spi.h"

static volatile bool dmaWriteBusy = false;


extern "C" void dmaCh1_ISR(dmaInterrupt_t inttype) {
    dmaWriteBusy = false;
}

namespace CubeRover {
    
  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  WatchDogInterfaceComponentImpl ::
#if FW_OBJECT_NAMES == 1
    WatchDogInterfaceComponentImpl(
        const char *const compName
    ) : WatchDogInterfaceComponentBase(compName)
#else
    WatchDogInterfaceComponentImpl(void)
#endif
      , m_sci(scilinREG), m_finished_initializing(false), m_txCmdArray(), m_rxTask()
    {
        m_txCmdArray.commands[COMMAND_INDEX__STROKE].opcode = static_cast<FwOpcodeType>(STROKE_OPCODE);
        m_txCmdArray.commands[COMMAND_INDEX__DOWNLINK].opcode = static_cast<FwOpcodeType>(DOWNLINK_OPCODE);
        m_txCmdArray.commands[COMMAND_INDEX__RESET_SPECIFIC].opcode =
            static_cast<uint16_t>(WatchDogInterfaceComponentBase::OPCODE_RESET_SPECIFIC);
        m_txCmdArray.commands[COMMAND_INDEX__DISENGAGE_FROM_LANDER].opcode =
            static_cast<uint16_t>(WatchDogInterfaceComponentBase::OPCODE_DISENGAGE_FROM_LANDER);
        m_txCmdArray.commands[COMMAND_INDEX__ENGAGE_FROM_LANDER].opcode =
            static_cast<uint16_t>(WatchDogInterfaceComponentBase::OPCODE_ENGAGE_FROM_LANDER);
    }
  
    void WatchDogInterfaceComponentImpl ::
      init(
          const NATIVE_INT_TYPE queueDepth,
          const NATIVE_INT_TYPE instance
      )
    {
        WatchDogInterfaceComponentBase::init(queueDepth, instance);
        sciEnterResetState(m_sci);
        sciSetBaudrate(m_sci, 9600);
        sciExitResetState(m_sci);

        // Configure and up the receivng task
        m_rxTask.registerCallback(this);
        ::Os::Task::TaskStatus taskStat = m_rxTask.start(WATCH_DOG_INTERFACE_RX_TASK_PRIORITY,
                                                         WATCH_DOG_INTERFACE_RX_TASK_STACK_SIZE,
                                                         WATCH_DOG_INTERFACE_RX_TASK_CPU_AFFINITY);
        // Assert that this will always be started successfully. If it isn't, we're screwed.
        FW_ASSERT(taskStat == Os::Task::TASK_OK, taskStat);
        
        gioSetBit(spiPORT3, deploy_bit, 0);
    
        Read_Temp();
    
        // Reset WF121
        bool success = logAndSendResetSpecific(WatchDogInterfaceComponentBase::OPCODE_RESET_SPECIFIC,
                                               0,
                                               Reset_Radio,
                                               false);
       
        if (!success) {
            // TODO: LOG ERROR
        }

        // There would normally be a 400000000 cycle delay for WF121 to finish resetting.. but that actualyl breaks things... weird
        m_finished_initializing = true;
    }
  
    WatchDogInterfaceComponentImpl ::
      ~WatchDogInterfaceComponentImpl(void)
    {
        // Do Not Use
    }
  
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------
  
    // Timed function that runs every 1Hz
    void WatchDogInterfaceComponentImpl ::
      Run_handler(
          const NATIVE_INT_TYPE portNum,
          NATIVE_UINT_TYPE context
      )
    {
        static uint16_t sequenceNumber = 0;
        
        // Update Thermistor Telemetry
        Read_Temp();

        bool success = txCommand(STROKE_OPCODE, sequenceNumber, static_cast<uint16_t>(No_Reset), nullptr, 0, true);

        if (success) {
            sequenceNumber++;
        } else {
            // TODO: Add logging error
        }

        return;
    }
  
    void WatchDogInterfaceComponentImpl ::
      CompResetRequest_handler(
          const NATIVE_INT_TYPE portNum,
          CubeRoverPorts::ResetValue reset
      )
    {
        // Depricated, not needed
    }
  
    void WatchDogInterfaceComponentImpl ::
      downlink_handler(
          const NATIVE_INT_TYPE portNum,
          Fw::Buffer &fwBuffer
      )
    {
        static uint16_t sequenceNumber = 0;

        bool success = txCommand(DOWNLINK_OPCODE,
                                 sequenceNumber,
                                 static_cast<uint16_t>(No_Reset),
#pragma diag_push
#pragma diag_suppress 1107
                                 // This casts an integer to a pointer with a smaller size than the source integer,
                                 // and CCS warns about doing this. However, in this case it is necessary and should
                                 // be fine, so I disable the warning
                                 reinterpret_cast<uint8_t*>(fwBuffer.getdata()),
#pragma diag_pop
                                 static_cast<size_t>(fwBuffer.getsize()),
                                 true);

        if (success) {
            sequenceNumber++;
        } else {
            // TODO: Add logging error
        }

        return;    
    }
    
    void WatchDogInterfaceComponentImpl ::
      PingIn_handler(
          const NATIVE_INT_TYPE portNum,
          U32 key
      )
    {
        this->PingOut_out(0, key);
    }
  
    void WatchDogInterfaceComponentImpl ::
      WdogStroke_handler(
          const NATIVE_INT_TYPE portNum,
          U32 code
      )
    {
        // Depricated, not used currently
    }
    
    // ----------------------------------------------------------------------
    // Command handler implementations
    // ----------------------------------------------------------------------

    // Function called by cmd handler to send a reset to watchdog
    void WatchDogInterfaceComponentImpl :: Reset_Specific_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          reset_values_possible reset_value
      )
    {
        logAndSendResetSpecific(opCode - this->getIdBase(), cmdSeq, reset_value, true);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Disengage_From_Lander_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          confirm_disengage confirm
      )
    {
  	    // Send Activity Log/tlm to know watchdog recieved command
  	    char command_type[24] = "Disengage From Rover";
  	    Fw::LogStringArg command_type_log = command_type;
  	    this->log_ACTIVITY_HI_WatchDogCmdReceived(command_type_log);
    
        bool success = sendResetSpecific(opCode - this->getIdBase(),
                                         cmdSeq,
                                         static_cast<reset_values_possible>(Disengage),
                                         true);

        if (success) {
            // Set Deployment Bit High
            // Deployment2 signal is on MIBSPI3NCS_4 which is setup as a GPIO pin with default 0 and no pull up/down resistor.
            // Use Bit 5 as MIBSPI3NCS_4 is the 5th (start at 0) pin from the start of SPI3 Port 
            gioSetBit(spiPORT3, deploy_bit, 1);
        }
    }
  
    void WatchDogInterfaceComponentImpl ::
      Engage_From_Lander_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq
      )
    {
        // Send Activity Log/tlm to know watchdog recieved command
        char command_type[24] = "Engage From Rover";
        Fw::LogStringArg command_type_log = command_type;
        this->log_ACTIVITY_HI_WatchDogCmdReceived(command_type_log);
    
        bool success = sendResetSpecific(opCode - this->getIdBase(), cmdSeq, HDRM_Off, true);
    
        if (success) {
            // Set Deployment Bit low
            // Deployment2 signal is on MIBSPI3NCS_4 which is setup as a GPIO pin with default 0 and no pull up/down resistor.
            // Use Bit 5 as MIBSPI3NCS_4 is the 5th (start at 0) pin from the start of SPI3 Port 
            gioSetBit(spiPORT3, deploy_bit, 0);
        }
    }

    bool WatchDogInterfaceComponentImpl::logAndSendResetSpecific(FwOpcodeType opCode,
                                                                 U32 cmdSeq,
                                                                 reset_values_possible resetValue,
                                                                 bool sendResponse)
    {
        // Send Activity Log/tlm to know watchdog recieved command
        char command_type[24] = "Reset Specific:";
        char reset_val_char[8];
        sprintf(reset_val_char, "%u", resetValue);
        strcat(command_type, reset_val_char);
        Fw::LogStringArg command_type_log = command_type;
        this->log_ACTIVITY_HI_WatchDogCmdReceived(command_type_log);
  
        return sendResetSpecific(opCode, cmdSeq, resetValue, sendResponse);
    }
  
    bool WatchDogInterfaceComponentImpl::sendResetSpecific(FwOpcodeType opCode,
                                                           U32 cmdSeq,
                                                           reset_values_possible resetValue,
                                                           bool sendResponse)
    {  
        // Check that reset_value is correct
        if (resetValue >= reset_values_possible_MAX)
        {
            this->log_WARNING_LO_WatchDogIncorrectResetValue();
            return false;
        }    

        bool success = txCommand(opCode, cmdSeq, static_cast<uint16_t>(resetValue), nullptr, 0, sendResponse);

        if (!success && sendResponse) {
            this->cmdResponse_out(opCode, cmdSeq, Fw::COMMAND_EXECUTION_ERROR);
        }

        return success;
    }

    /* Commands that Only Watchdog Processes */
    void WatchDogInterfaceComponentImpl ::
      Prepare_For_Deployment_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          confirm_prepare_for_deploy confirm
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Switch_Connection_Mode_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Set_Kp_Specific_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          U16 value
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Set_Heater_Duty_Cycle_Max_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Set_Heater_Duty_Cycle_Period_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          U16 period
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Set_Heater_Window_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          U16 adc_half_width
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Set_Heater_Setpoint_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          U16 adc_setpoint
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Switch_to_Sleep_Mode_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          confirm_sleep_mode confirm
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Switch_to_Keep_Alive_Mode_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          confirm_alive_mode confirm
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    void WatchDogInterfaceComponentImpl ::
      Switch_to_Service_Mode_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          confirm_service_mode confirm
      )
    {
        // TODO
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
    }
  
    /* End of Commands that Only Watchdog Processes*/
  
  
    bool WatchDogInterfaceComponentImpl :: Read_Temp()
    {
        // Start ADC Conversions for all thermistors
        adcStartConversion(adcREG1, adcGROUP1);
    
        // Check if all ADC Conversions are done
        // From testing tries ALMOST ALWAYS ~= 10 - 12  ==>  38 - 40 cycles to convert data.. OK to poll
        int tries = 50;
        while(--tries && !adcIsConversionComplete(adcREG1, adcGROUP1));
        
        if(tries == 0)
        {
            // Safety stop for conversion to prevent a hangup
            adcStopConversion(adcREG1, adcGROUP1);
      
            this->log_WARNING_HI_ADCThermistorError();
            return false;
        }
        else
        {
            // Conversion SHOULD end automatically once all ADC values have been converted but this should end it otherwise
            adcStopConversion(adcREG1, adcGROUP1);
            U32 num_conversions = adcGetData(adcREG1, adcGROUP1, m_thermistor_buffer);
            if(num_conversions >= number_thermistors)
            {
                this->tlmWrite_THERM_0(m_thermistor_buffer[0].value);
                this->tlmWrite_THERM_1(m_thermistor_buffer[1].value);
                this->tlmWrite_THERM_2(m_thermistor_buffer[2].value);
                this->tlmWrite_THERM_3(m_thermistor_buffer[3].value);
                this->tlmWrite_THERM_4(m_thermistor_buffer[4].value);
                this->tlmWrite_THERM_5(m_thermistor_buffer[5].value);
            }
            else
            {
                this->log_WARNING_HI_ADCThermistorError();
                return false;
            }
        }

        return true;
    }


    void WatchDogInterfaceComponentImpl::rxCallback(WatchDogMpsm::Message& msg, bool goodParity)
    {
        // If the message failed its parity check we don't even try to match it against any previously
        // sent messages. Instead, just warn about this occurring. We'll rely on timeout to eventually
        // clear up the tx command status structure for whatever tx command resulted in this
        // message with bad parity (if it was even a command to begin with)
        if (!goodParity) {
            this->log_WARNING_HI_WatchDogIncorrectResp(bad_parity);
        }
  
        // Uplink messages are different (specifically, they aren't a response to a Hercules command)
        // so we handle them separately
        if (msg.parsedHeader.lowerOpCode == UPLINK_OPCODE) {
            handleUplinkMsg(msg);
            return;
        }
  
        // Try to get the transmit status structure for the received message
        TxCommandStatus* cmdStatus = getTxCommandStatus(msg.parsedHeader.lowerOpCode);
  
        if (NULL == cmdStatus) {
            // TODO: Log error (which error?)
            return;
        }
  
        // If we got a message, but the tx command status structure indicates that it is inactive
        // (i.e. that there was no transmitted message that is waiting for a response) then we want
        // to log an error, but continue on anyway so that we handle the message if it's telemetry.
        // In this case, since the tx command status structure is already inactive there is no need
        // to change it.
        //
        // On the other hand, if the status structure indicates that it *is* active but the sequence
        // number in the status structure is different than the sequence number of the received message,
        // then our handling depends on the ordering of the sequence numbers. If the received sequence 
        // number is greater than the sequence number in the status structure (not sure how this could 
        // happen), then we will reset the status structure and send a response for it, but we'll also
        // log an error about the incorrect sequence number. If the received sequence number is less 
        // than the sequence number in the status structure, then presumably what happened is that
        // a previous command of the same type timed out while waiting for a response from WD, and 
        // Hercules sent another command of the same type. Getting this response now, for the previous
        // message, means that for some reason WD was delayed in responding. In this case, we will not
        // clear the status structure, as the hope is that the WD will send another response for the 
        // subsequent command that was sent. We'll still log an error in this case as well, but since
        // we don't know the correct sequence number to respond for the older tx message we will not
        // send a response. In either case (a sequence number mismatch of either ordering) we will
        // still handle the message (i.e. we'll process it if it is telemetry).
        //
        // In the nominal case, where everything in this response is what we would expect given the
        // contents of the tx status structure, we'll send a response for the tx command in question,
        // reset the tx status structure, and then handle the message (if it's telemetry).
        //
        // In all three of the above cases, any tx command structure manipulation will happen in the
        // section below in which the mutex is locked. However, any logging, responding to any 
        // existing command, or handling of the message will be delayed until after the locked section.
  
        bool cmdInactive = false;
        bool rxOlderSeqNum = false;
        bool rxNewerSeqNum =  false;
        FwOpcodeType txCmdOpCode = 0;
        U32 txCmdSeqNum = 0;
        bool txCmdSendResponse = false;
  
  
        { // The mutex must be unlocked before we exit this scope 
            m_txCmdArray.cmdMutex.lock();
  
            if (!cmdStatus->active) {
                // Inactive tx command
                cmdInactive = true;
            } else {
                // Store the contents of the tx command structure prior to possible reset and so we can use 
                // them outside of this block
                txCmdOpCode = cmdStatus->opcode;
                txCmdSeqNum = cmdStatus->seqNum;
                txCmdSendResponse = cmdStatus->sendResponse;
  
                uint16_t ushortTxSeqNum = static_cast<uint16_t>(txCmdSeqNum);
  
                if (msg.parsedHeader.lowerSeqNum == ushortTxSeqNum) {
                    // Matching sequence numbers. Just need to reset the tx status structure
                    cmdStatus->reset();
                } else if (msg.parsedHeader.lowerSeqNum < ushortTxSeqNum) {
                    // Received sequence number is less than (older than) the one in the tx status structure
                    rxOlderSeqNum = true;
                } else {
                    // Received sequence number is greater than (newer than) the one in the tx status structure
                    rxNewerSeqNum = true;
                    cmdStatus->reset();
                }
            }
  
            m_txCmdArray.cmdMutex.unLock();
        }
  
        if (cmdInactive) {
            // TODO: LOG ERROR (which error to log?)
        } else if (rxOlderSeqNum) {
            // TODO: LOG ERROR (which error to log?)
        } else if (rxNewerSeqNum) {
            // TODO: LOG ERROR (which error to log?)
  
            // We want to respond to the old tx message. Make sure we don't try to send a response about any
            // of our fake opcodes, and don't send a response if we didn't want to send one when we sent the message
            if (txCmdOpCode != STROKE_OPCODE
                && txCmdOpCode != DOWNLINK_OPCODE
                && txCmdSendResponse) {
                this->cmdResponse_out(txCmdOpCode, txCmdSeqNum, Fw::COMMAND_EXECUTION_ERROR);
            } 
        } else {
            // We want to respond positively about the tx message. Make sure we don't try to send a response about any
            // of our fake opcodes, and don't send a response if we didn't want to send one when we sent the message
            if (txCmdOpCode != STROKE_OPCODE
                && txCmdOpCode != DOWNLINK_OPCODE
                && txCmdSendResponse) {
                this->cmdResponse_out(txCmdOpCode, txCmdSeqNum, Fw::COMMAND_OK);
            }
        }
  
        // Now we want to handle this message if it contains telemetry. For Reset_Specific and Downlink,
        // we're only looking for an ACK so we don't need to worry about doing anything with the responses for
        // those. However, the responses for the strokes will contain telemetry, and we do want to handle that.
        if (msg.parsedHeader.lowerOpCode == STROKE_OPCODE) {
            handleTelemetryMsg(msg);
        }
  
        return;
    }
  
    void WatchDogInterfaceComponentImpl::handleUplinkMsg(WatchDogMpsm::Message& msg)
    {
        // Before anything else, make sure we have enough data (and since the payload is variable size,
        // in this case "enough" just means non-zero and not over the maximum)
        if (msg.accumulatedDataSize <= 0 || msg.accumulatedDataSize > WATCHDOG_MAX_PAYLOAD) {
            this->log_WARNING_HI_WatchDogIncorrectResp(bad_size_received);
            return;
        }
  
        // TODO: Verify that the MTU for wired connection is the same as Wifi
        Fw::Buffer uplinked_data;
        uplinked_data.setdata(reinterpret_cast<U64>(msg.dataBuffer));
        uplinked_data.setsize(static_cast<U32>(msg.accumulatedDataSize));
        uplink_out(0, uplinked_data);
    }
  
    void WatchDogInterfaceComponentImpl::handleTelemetryMsg(WatchDogMpsm::Message& msg)
    {
        // Before anything else, make sure we have enough data
        if (msg.accumulatedDataSize != sizeof(struct WatchdogTelemetry)) {
            this->log_WARNING_HI_WatchDogIncorrectResp(bad_size_received);
            return;
        }
  
        struct WatchdogTelemetry* buff = 
            reinterpret_cast<struct WatchdogTelemetry*>(msg.dataBuffer);
  
        this->tlmWrite_VOLTAGE_2_5V(buff->voltage_2V5);
        this->tlmWrite_VOLTAGE_2_8V(buff->voltage_2V8);
        this->tlmWrite_VOLTAGE_24V(buff->voltage_24V);
        this->tlmWrite_VOLTAGE_28V(buff->voltage_28V);
        this->tlmWrite_BATTERY_THERMISTOR(buff->battery_thermistor);
        this->tlmWrite_SYSTEM_STATUS(buff->sys_status);
        this->tlmWrite_BATTERY_LEVEL(buff->battery_level);
        this->tlmWrite_BATTERY_CURRENT(buff->battery_current);
        this->tlmWrite_BATTERY_VOLTAGE(buff->battery_voltage);
  
        return;
    }
    
    WatchDogInterfaceComponentImpl::TxCommandStatus*
    WatchDogInterfaceComponentImpl::getTxCommandStatusFullOpcode(FwOpcodeType opCode)
    {
        return getTxCommandStatus(static_cast<uint16_t>(opCode));
    }
  
    WatchDogInterfaceComponentImpl::TxCommandStatus*
    WatchDogInterfaceComponentImpl::getTxCommandStatus(uint16_t opCode)
    {
        static const uint16_t RESET_SPECIFIC_OPCODE_USHORT =
            static_cast<uint16_t>(WatchDogInterfaceComponentBase::OPCODE_RESET_SPECIFIC);
        static const uint16_t DISENGAGE_FROM_LANDER_USHORT =
            static_cast<uint16_t>(WatchDogInterfaceComponentBase::OPCODE_DISENGAGE_FROM_LANDER);
        static const uint16_t ENGAGE_FROM_LANDER_USHORT =
            static_cast<uint16_t>(WatchDogInterfaceComponentBase::OPCODE_ENGAGE_FROM_LANDER);
  
        size_t index = 0;
  
        switch (opCode) {
            case STROKE_OPCODE:
                index = COMMAND_INDEX__STROKE;
                break;
  
            case DOWNLINK_OPCODE:
                index = COMMAND_INDEX__DOWNLINK;
                break;
  
            case RESET_SPECIFIC_OPCODE_USHORT:
                index = COMMAND_INDEX__RESET_SPECIFIC;
                break;
  
            case DISENGAGE_FROM_LANDER_USHORT:
                index = COMMAND_INDEX__DISENGAGE_FROM_LANDER;
                break;
  
            case ENGAGE_FROM_LANDER_USHORT:
                index = COMMAND_INDEX__ENGAGE_FROM_LANDER;
                break;
  
            default:
                // Should never happen;
                FW_ASSERT(false, opCode);
                break;
        }
  
        // This is equivalent to &(m_txCmdArray.commands[index])
        return m_txCmdArray.commands + index;
    }
  
    bool WatchDogInterfaceComponentImpl::txCommand(FwOpcodeType opCode,
                                                   U32 cmdSeq,
                                                   uint16_t resetValue,
                                                   uint8_t* dataBuffer,
                                                   size_t dataLen,
                                                   bool sendResponse)
    {
        struct WatchdogFrameHeader frame;
        frame.magic_value = header_magic;
        frame.parity = 0;
        frame.payload_length = static_cast<uint16_t>(dataLen);
        frame.reset_val = resetValue;
        frame.sequence_number = static_cast<uint16_t>(cmdSeq);
        frame.opcode = static_cast<uint16_t>(opCode);
  
        uint8_t* frameBytes = reinterpret_cast<uint8_t*>(&frame);
        uint8_t runningParity = 0;
  
        for (size_t i = 0; i < sizeof(WatchdogFrameHeader); ++i) {
            runningParity += frameBytes[i];
        }
  
        frame.parity = ~runningParity;
  
        TxCommandStatus* cmdStatus = getTxCommandStatusFullOpcode(opCode);
  
        if (NULL == cmdStatus) {
            return false;
        }
  
        // Get the current time
        Fw::Time now = getTime();
        uint32_t nowMillis = static_cast<uint32_t>(now.get_time_ms());
        bool previousStillWaiting = false;
        bool timeout = false;
        FwOpcodeType timedOutOpcode = 0;
        U32 timedOutSeqNum = 0;
        bool timedOutSendResponse = false;
  
        
        { // We shouldn't leave this scope without unlocking the mutex
            m_txCmdArray.cmdMutex.lock();
  
            // Note: We don't want to do any comms (or anything else long or complicated) with the mutex locked.
  
            // First, check if a command of this type is already active (i.e. waiting for a response)
            if (cmdStatus->active) {
                // One is active, so check if it has timed out (this will still work even if the time wraps)
                if (nowMillis - cmdStatus->txTimeMillis >= COMMAND_TIMEOUT_MILLISECONDS) {
                    // We're timing out the previous command.
                    timeout = true;
                    timedOutOpcode = cmdStatus->opcode;
                    timedOutSeqNum = cmdStatus->seqNum;
                    timedOutSendResponse = cmdStatus->sendResponse;
                    cmdStatus->reset();
                } else {
                    // The previous tx of this command type has not timed out yet, so we won't send this command
                    previousStillWaiting = true;
                }
            }
  
            // Update the tx command structure with the current data (only if we don't already have a command
            // of this type waiting for a response)
            if (!previousStillWaiting) {
              cmdStatus->active = true;
              cmdStatus->sendResponse = sendResponse;
              cmdStatus->seqNum = cmdSeq;
              cmdStatus->txTimeMillis = nowMillis;
            }
  
            m_txCmdArray.cmdMutex.unLock();
        }
  
        // Now actually send the response about the previous iteration of this command timing out
        if (timeout) {
            this->log_WARNING_HI_WatchDogTimedOut();
  
            // Make sure we don't try to send a response about any of our fake opcodes, and don't
            // send a response if we didn't want to send one when we sent the message
            if (timedOutOpcode != STROKE_OPCODE
                && timedOutOpcode != DOWNLINK_OPCODE
                && timedOutSendResponse) {
              this->cmdResponse_out(timedOutOpcode, timedOutSeqNum, Fw::COMMAND_EXECUTION_ERROR);
            }
        }
  
        // If we're not sending this command we don't want to continue
        if (previousStillWaiting) {
            // TODO: LOG ERROR (which error?)
            return false;
        }
  
        // Finally we're ready to transmit. If this is header-only we can trasnmit it all in one go (since it's in
        // one "buffer"), and since we can transmit it all in one go we can also transmit it non-blocking. On the 
        // other hand, if there is also data then we need to transmit the header with blocking, then we can 
        // transmit the data non-blocking. However, with the non-blocking send of the header we need to make
        // sure dmaSend doesn't return false, because if it does that means a previous send is still active.
        // If that's the case then we'll return the send as blocking (even if there is no data).
        bool success = dmaSend(&frame, sizeof(frame), (dataLen != 0));
  
        if (!success) {
            dmaSend(&frame, sizeof(frame), true);
        }
  
        if (dataLen > 0) {
            // If we're sending data then we always know that the previous blocking send of the header is complete,
            // which means that this should always succeed. However, we'll handle that case anyway just in case.
            success = dmaSend(dataBuffer, dataLen, false);
  
            if (!success) {
                dmaSend(dataBuffer, dataLen, true);
            }
        }
  
        return true;
    }
  
    // FIXME: Add timeout to escape polling loop
    void WatchDogInterfaceComponentImpl::pollDMASendFinished() {
        if (!m_finished_initializing) {
            while (!((getDMAIntStatus(BTC) >> SCILIN_TX_DMA_CH) & 0x01U));
            dmaWriteBusy = false;
            sciDMASendCleanup(SCILIN_TX_DMA_CH);
        }
        while (dmaWriteBusy);    // TODO: Mutex to allow multiprogramming & TIMEOOUT
    }
  
    // Returns negative on error
    bool WatchDogInterfaceComponentImpl::dmaSend(void *buffer, int size, bool blocking) {
        if (blocking)
            while (dmaWriteBusy);
        else if (dmaWriteBusy)
            return false;
        sciDMASend(SCILIN_TX_DMA_CH, static_cast<char *>(buffer), size, ACCESS_8_BIT, &dmaWriteBusy);
        if (blocking)
            pollDMASendFinished();
        return true;
    }

} // end namespace CubeRover
