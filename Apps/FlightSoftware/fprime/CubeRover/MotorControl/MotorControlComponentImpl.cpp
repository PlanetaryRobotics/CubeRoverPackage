// ======================================================================
// \title  MotorControlComponentImpl.cpp
// \author cedric
// \brief  cpp file for MotorControl component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================


#include <CubeRover/MotorControl/MotorControlComponentImpl.hpp>
#include "Fw/Types/BasicTypes.hpp"

#include "i2c.h"
#include "CubeRoverConfig.hpp"

namespace CubeRover {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  MotorControlComponentImpl ::
#if FW_OBJECT_NAMES == 1
    MotorControlComponentImpl(
        const char *const compName
    ) :
      MotorControlComponentBase(compName)
#else
    MotorControlComponentImpl(void)
#endif
  {

  }

  void MotorControlComponentImpl ::
    init(
        const NATIVE_INT_TYPE queueDepth,
        const NATIVE_INT_TYPE instance
    )
  {
    MotorControlComponentBase::init(queueDepth, instance);

    // Create look-up table for I2C communications with motor controller board
    generateChecksumTable(CRC8);

    // left and right turn parameters
    m_rightSpeed = 0;
    m_leftSpeed = 0;
    m_rightAngle = 0;
    m_leftAngle = 0;

    // forward and reverse parameters
    m_fwDist = 0;
    m_reDist = 0;
    m_fwSpeed = 0;
    m_reSpeed = 0;

    m_encoderTickToCmRatio = (PI * CUBEROVER_WHEEL_DIAMETER_CM) / (MOTOR_NB_PAIR_POLES * MOTOR_GEAR_BOX_REDUCTION * 6.0);
  }

  MotorControlComponentImpl ::
    ~MotorControlComponentImpl(void)
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void MotorControlComponentImpl ::
    schedIn_handler(
        const NATIVE_INT_TYPE portNum,
        NATIVE_UINT_TYPE context
    )
  {
    // TODO
  }

  // ----------------------------------------------------------------------
  // Command handler implementations
  // ----------------------------------------------------------------------

  void MotorControlComponentImpl ::
    MC_DrivingConfiguration_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        CommandList CommandConfiguration,
        ParameterList Parameter,
        U8 Value
    )
  {
    switch(CommandConfiguration){
      case FORWARD_CFG:
        switch(Parameter){
          case DISTANCE:
            m_fwDist = value;
            break;
          case SPEED:
            m_fwSpeed = value;
            break;
          default:
            this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
            return;
        }
        break;
      case REVERSE_CFG:
        switch(Parameter){
          case DISTANCE:
            m_reDist = value;
            break;
          case SPEED:
            m_reSpeed = value;
            break;
          default:
            this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
            return;
        }
      case LEFT_CFG:
        switch(Parameter){
          case SPEED:
            m_leftSpeed = value;
            break;
          case ANGLE:
            m_leftAngle = value;
            break;
          default:
            this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
            return;
        }
        break;
      case RIGHT_CFG:
        switch(Parameter){
          case SPEED:
            m_rightSpeed = value;
            break;
          case ANGLE:
            m_rightAngle = value;
            break;
          default:
            this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
            return;
        }

        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
        break;

    }
  }

  
  /**
   * @brief      { function_description }
   *
   * @param[in]  dist  The distance
   */
  inline Motor_tick MotorControlComponentImpl :: cmToMotorTicks(const Distance_cm dist){
    return dist / m_encoderTickToCmRatio;
  }

  inline MCError MotorControlComponentImpl :: enableDrivers(){

    return MC_NO_ERROR;
  }

  inline MCError MotorControlComponentImpl :: disableDrivers(){
    if(err = writeMotorControlRegister( MOTOR_CONTROL_I2CREG, 
                                        DISABLE_DRIVER, 
                                        FRONT_LEFT_MC_I2C_ADDR,
                                        0xff /*dummy write*/) != MC_NO_ERROR)
      return err;
    if(err = writeMotorControlRegister( MOTOR_CONTROL_I2CREG, 
                                        DISABLE_DRIVER, 
                                        REAR_LEFT_MC_I2C_ADDR,
                                        0xff /*dummy write*/) != MC_NO_ERROR)
      return err;
    if(err = writeMotorControlRegister( MOTOR_CONTROL_I2CREG, 
                                        DISABLE_DRIVER, 
                                        FRONT_RIGHT_MC_I2C_ADDR,
                                        0xff /*dummy write*/) != MC_NO_ERROR)
      return err;
    if(err = writeMotorControlRegister( MOTOR_CONTROL_I2CREG, 
                                        DISABLE_DRIVER, 
                                        REAR_RIGHT_MC_I2C_ADDR,
                                        0xff /*dummy write*/) != MC_NO_ERROR)
      return err;    
    return MC_NO_ERROR;
  }


  void MotorControlComponentImpl ::
    MC_ExecuteDriving_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        DriveCommandList DrivingCommand
    )
  {
    MCError err = MC_NO_ERROR;

    switch(DrivingCommand){
      case GO_FORWARD:
      case GO_REVERSE:
        Motor_tick targetTick = cmToMotorTicks(m_fwDist);

        // Set target position in ticks
        if(err = writeMotorControlRegister( MOTOR_CONTROL_I2CREG, 
                                            RELATIVE_TARGET_POSITION, 
                                            FRONT_LEFT_MC_I2C_ADDR,
                                            targetTick) != MC_NO_ERROR){
          this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
          return;
        }

        if(err = writeMotorControlRegister( MOTOR_CONTROL_I2CREG,
                                            RELATIVE_TARGET_POSITION,
                                            FRONT_RIGHT_MC_I2C_ADDR,
                                            targetTick) != MC_NO_ERROR){
          this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
          return;
        }

        if(err = writeMotorControlRegister( MOTOR_CONTROL_I2CREG,
                                            RELATIVE_TARGET_POSITION,
                                            REAR_LEFT_MC_I2C_ADDR,
                                            targetTick) != MC_NO_ERROR){
          this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
          return;          
        }

        if(err = writeMotorControlRegister( MOTOR_CONTROL_I2CREG,
                                            RELATIVE_TARGET_POSITION,
                                            REAR_RIGHT_MC_I2C_ADDR,
                                            targetTick) != MC_NO_ERROR){
          this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
          return;            
        }
        
        // Set target speed in percentage
        if(err = writeRegister(MOTOR_CONTROL_I2CREG,
                               TARGET_SPEED,
                               FRONT_LEFT_MC_I2C_ADDR,
                               targetTick) != MC_NO_ERROR){
          this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
          return;             
        }

        if(err = writeRegister(MOTOR_CONTROL_I2CREG,
                               TARGET_SPEED,
                               FRONT_RIGHT_MC_I2C_ADDR,
                               targetTick) != MC_NO_ERROR){
          this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
          return;            
        }

        if(err = writeRegister(MOTOR_CONTROL_I2CREG,
                               TARGET_SPEED,
                               REAR_LEFT_MC_I2C_ADDR,
                               targetTick) != MC_NO_ERROR){
          this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
          return;            
        }

        if(err = writeRegister( MOTOR_CONTROL_I2CREG,
                                TARGET_SPEED,
                                REAR_RIGHT_MC_I2C_ADDR,
                                targetTick) != MC_NO_ERROR){
          this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
          return;            
        }     
        break;
      case TURN_LEFT:

        break;
      case TURN_RIGHT:

        break;
      default:
        this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_FAIL);
        return;
    }

    enableDrivers();

    writeRegister(MOTOR_CONTROL_I2CREG, EXECUTE_CMD, FRONT_LEFT_MC_I2C_ADDR, DrivingCommand);
    writeRegister(MOTOR_CONTROL_I2CREG, EXECUTE_CMD, FRONT_RIGHT_MC_I2C_ADDR, DrivingCommand);
    writeRegister(MOTOR_CONTROL_I2CREG, EXECUTE_CMD, REAR_LEFT_MC_I2C_ADDR, DrivingCommand);
    writeRegister(MOTOR_CONTROL_I2CREG, EXECUTE_CMD, REAR_RIGHT_MC_I2C_ADDR, DrivingCommand);

    disableDrivers();

    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void MotorControlComponentImpl ::
    MC_TuningParameters_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        tuningParameterList TuningParameter,
        U16 Value
    )
  {
    // TODO
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void MotorControlComponentImpl ::
    MC_AccelerationProfiles_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        accelerationParameterList AccelerationParameter,
        U16 Value
    )
  {
    // TODO
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void MotorControlComponentImpl ::
    MC_StallDetection_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        motorStallEnableList motorStallEnable
    )
  {
    // TODO
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }

  void MotorControlComponentImpl ::
    MC_PositionCounterReset_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq,
        U8 ResetPositionCounter
    )
  {
    // TODO
    this->cmdResponse_out(opCode,cmdSeq,Fw::COMMAND_OK);
  }


  /**
   * @brief      Gets the size data.
   *
   * @param[in]  id    The identifier
   *
   * @return     The size data.
   */
  uint32_t MotorControlComponentImpl :: getSizeData(const MotorControlI2cRegId id){
    switch(id){
        case I2C_ADDRESS:
          return 1;
        case RELATIVE_TARGET_POSITION:
          return 4;
        case DIRECTION:
          return 1;
        case TARGET_SPEED:
          return 1;
        case CURRENT_POSITION:
          return 4;
        case MOTOR_CURRENT:
          return 2;
        case P_CURRENT:
          return 2;
        case I_CURRENT:
          return 2;
        case P_VELOCITY:
          return 2;
        case I_VELOCITY:
          return 2;
        case D_VELOCITY:
          return 2;
        case P_POSITION:
          return 2;
        case I_POSITION:
          return 2;
        case D_POSITION:
          return 2;
        case ACCELERATION:
          return 2;
        case DECELERATION:
          return 2;
        case EXECUTE_CMD:
          return 1;
        case CURRENT_VELOCITY:
          return 2;
        case ENABLE_DRIVER:
          return 1;
        case DISABLE_DRIVER:
          return 1;
        case RESET_CONTROLLER:
          return 1;
        case FAULT_REGISTER:
          return 1;
        case CLEAR_FAULT:
          return 1;
        case STATUS_REGISTER:
          return 1;
        case POSITION_SENSOR_CURRENT_COMBINATION:
          return 1;
        case MC_MAX_NUM_OF_ELEMENTS:
          return 0;
      }   
  }


  /**
   * @brief      { function_description }
   *
   * @return     { description_of_the_return_value }
   */
  size_t MotorControlComponentImpl :: headerSize(){
    return sizeof(MotorControlI2cRegId_t) + 1 /* data size */;
  }


  /**
   * @brief      { function_description }
   *
   * @return     { description_of_the_return_value }
   */
  size_t MotorControlComponentImpl :: checksumSize(){
    return sizeof(MotorControlChecksum_t);
  }


  /**
   * @brief      { function_description }
   *
   * @param[in]  polynomial  The polynomial
   */
  void MotorControlComponentImpl :: generateChecksumTable(const ChecksumType polynomial){
  
    for(int32_t i=0; i<sizeof(checksumLookUpTable); ++i){
  
      int32_t curr=i;
  
      for(int32_t j=0; j<8; ++j){
        if((curr & 0x80) != 0){
          curr = (curr << 1) ^ (int)polynomial;
        } 
        else{
          curr <<= 1;
        }
      }
      
      checksumLookUpTable[i] = (uint8_t) curr;
    }
  }



   /**
    * @brief      Calculates the checksum 8.
    *
    * @param      data          The data
    * @param[in]  bufferLength  The buffer length
    * @param      checksum      The checksum
    *
    * @return     The checksum 8.
    */
   MCError MotorControlComponentImpl :: computeChecksum8( uint8_t *data,
                                                          const size_t bufferLength,
                                                          MotorControlChecksum *checksum){
    MotorControlChecksum_t sum = 0;

    if(data == NULL)
      return MC_UNEXPECTED_ERROR;


    for(uint32_t i=0; i<bufferLength; i++)
      sum = checksumLookUpTable[sum ^ data[i]];

    *checksum = sum;

    return MC_NO_ERROR;
  }


  /**
   * @brief      { function_description }
   *
   * @param[in]  id          The identifier
   * @param[in]  data        The data
   * @param[in]  dataLength  The data length
   *
   * @return     { description_of_the_return_value }
   */
  MCError MotorControlComponentImpl :: packTransmitBuffer(const MotorControlI2cRegId id,
                                                          const uint32_t data,
                                                          const size_t dataLength){

    size_t packetLength = headerSize() + dataLength + checksumSize();

    if(packetLength <= 0 || packetLength > MC_BUFFER_MAX_SIZE){
      return MC_UNEXPECTED_ERROR;
    }

    txData[0] = id;
    txData[1] = dataLength;
    memcpy(txData+2, data, sizeof(data));
    txData[packetLength-1] = computeChecksum8(txData, packetLength-1);

    return MC_NO_ERROR;
  }


  /**
   * @brief      Writes a register.
   *
   * @param      i2c   I 2 c
   * @param[in]  id    The identifier
   * @param[in]  add   The add
   * @param[in]  data  The data
   *
   * @return     { description_of_the_return_value }
   */
  MCError MotorControlComponentImpl :: writeMotorControlRegister( i2cBASE_t *i2c,
                                                                  const MotorControlI2cRegId id,
                                                                  const I2cSlaveAddress add,
                                                                  uint32_t data){
      MCError ret = MC_NO_ERROR;
      size_t dataLength = getSizeData(id);

      if(dataLength <= 0) 
        return MC_UNEXPECTED_ERROR;

      if(i2c == NULL)
        return MC_UNEXPECTED_ERROR;

      if(ret = packTransmitBuffer(id, data, dataLength) != MC_NO_ERROR)
        return ret;

      // Send command to all motor controllers
      if(ret = i2cMasterTransmit(i2c, add, dataLength, bufferLength) != MC_NO_ERROR)
        return ret;

      return ret;
  }


  /**
   * @brief      { function_description }
   *
   * @param      i2c     I 2 c
   * @param[in]  sadd    The sadd
   * @param[in]  length  The length
   * @param[in]  data    The data
   *
   * @return     { description_of_the_return_value }
   */
  MCError MotorControlComponentImpl :: i2cMasterTransmit(i2cBASE_t *i2c,
                                                      const I2cSlaveAddress sadd,
                                                      const uint32_t length,
                                                      const uint8_t * data){
    if(i2c == NULL)
      return MC_UNEXPECTED_ERROR;

    if(data == NULL)
      return MC_UNEXPECTED_ERROR;

    /* Configure address of Slave to talk to */
    i2cSetSlaveAdd(i2c, sadd);

    /* Set direction to Transmitter */
    i2cSetDirection(i2c, I2C_TRANSMITTER);

    /* Configure Data count */
    i2cSetCount(i2c, length);

    /* Set mode as Master */
    i2cSetMode(i2c, I2C_MASTER);

    /* Set Stop after programmed Count */
    i2cSetStop(i2c);

    /* Transmit Start Condition */
    i2cSetStart(i2c);

    /* Transmit DATA_COUNT number of data in Polling mode */
    i2cSend(i2c, length, data);

    /* Wait until Bus Busy is cleared */
    while(i2cIsBusBusy(i2c) == true);

    /* Wait until Stop is detected */
    while(i2cIsStopDetected(i2c) == 0);

    /* Clear the Stop condition */
    i2cClearSCD(i2c);

    return MC_NO_ERROR;
  }


  /**
   * @brief      { function_description }
   *
   * @param      i2c     I 2 c
   * @param[in]  sadd    The sadd
   * @param[in]  length  The length
   * @param[in]  data    The data
   *
   * @return     { description_of_the_return_value }
   */
  MCError MotorControlComponentImpl :: i2cMasterReceive(i2cBASE_t *i2c,
                                                     const uint32_t sadd,
                                                     const uint32_t length,
                                                     const uint8_t * data){
    if(i2c == NULL)
      return MC_UNEXPECTED_ERROR;

    if(data == NULL)
      return MC_UNEXPECTED_ERROR;

    /* Configure address of Slave to talk to */
    i2cSetSlaveAdd(i2c, sadd);

    /* Set direction to receiver */
    i2cSetDirection(i2c, I2C_RECEIVER);

    /* Configure Data count */
    i2cSetCount(i2c, length);

    /* Set mode as Master */
    i2cSetMode(i2c, I2C_MASTER);

    /* Set Stop after programmed Count */
    i2cSetStop(i2c);

    /* Transmit Start Condition */
    i2cSetStart(i2c);

    /* Transmit DATA_COUNT number of data in Polling mode */
    i2cReceive(i2c, length, data);

    /* Wait until Bus Busy is cleared */
    // TODO add timeout
    while(i2cIsBusBusy(i2c) == true);

    /* Wait until Stop is detected */
    while(i2cIsStopDetected(i2c) == 0);

    /* Clear the Stop condition */
    i2cClearSCD(i2c);

    return MC_NO_ERROR;
  }

} // end namespace CubeRover
