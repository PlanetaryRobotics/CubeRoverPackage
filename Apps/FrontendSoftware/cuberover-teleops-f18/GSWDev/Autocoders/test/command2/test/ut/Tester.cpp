// ====================================================================== 
// \title  TestCommand.hpp
// \author tim
// \brief  cpp file for TestCommand test harness implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the Office
// of Technology Transfer at the California Institute of Technology.
// 
// This software may be subject to U.S. export control laws and
// regulations.  By accepting this document, the user agrees to comply
// with all U.S. export laws and regulations.  User has the
// responsibility to obtain export licenses, or other export authority
// as may be required before exporting such information to foreign
// countries or providing access to foreign persons.
// ====================================================================== 

#include "Tester.hpp"

#define INSTANCE 0
#define MAX_HISTORY_SIZE 10
#define QUEUE_DEPTH 10

namespace AcTest {

  // ----------------------------------------------------------------------
  // Construction and destruction 
  // ----------------------------------------------------------------------

  Tester ::
    Tester(void) : 
#if FW_OBJECT_NAMES == 1
      TestCommandGTestBase("Tester", MAX_HISTORY_SIZE),
      component("TestCommand")
#else
      TestCommandGTestBase(MAX_HISTORY_SIZE),
      component()
#endif
  {
    this->initComponents();
    this->connectPorts();
  }

  Tester ::
    ~Tester(void) 
  {
    
  }

  // ----------------------------------------------------------------------
  // Tests 
  // ----------------------------------------------------------------------

  void Tester ::
    msgTest(void)
  {
      for (NATIVE_UINT_TYPE iter=0; iter <= QUEUE_DEPTH; iter++) {
        this->sendCmd_TEST_CMD_1(
                0, //!< The instance number
                10, //!< The command sequence number
                20, //!< The I32 command argument
                TestCommandComponentBase::MEMB1 //!< The ENUM argument
            );
      }
      ASSERT_EQ(this->component.m_msgsDropped,1);
  }

  // ----------------------------------------------------------------------
  // Helper methods 
  // ----------------------------------------------------------------------

  void Tester ::
    connectPorts(void) 
  {

    // aport
    this->connect_to_aport(
        0,
        this->component.get_aport_InputPort(0)
    );

    // CmdDisp
    this->connect_to_CmdDisp(
        0,
        this->component.get_CmdDisp_InputPort(0)
    );

    // CmdStatus
    this->component.set_CmdStatus_OutputPort(
        0, 
        this->get_from_CmdStatus(0)
    );

    // CmdReg
    this->component.set_CmdReg_OutputPort(
        0, 
        this->get_from_CmdReg(0)
    );

  }

  void Tester ::
    initComponents(void) 
  {
    this->init();
    this->component.init(
        QUEUE_DEPTH, INSTANCE
    );
  }

} // end namespace AcTest
