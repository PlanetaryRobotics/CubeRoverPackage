// ======================================================================
// \title  LedComponentImpl.cpp
// \author abby
// \brief  cpp file for Led component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================


#include <CubeRover/Led/LedComponentImpl.hpp>
#include "Fw/Types/BasicTypes.hpp"
#include "gio.h"

namespace CubeRover {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  LedComponentImpl ::
#if FW_OBJECT_NAMES == 1
    LedComponentImpl(
        const char *const compName
    ) :
      LedComponentBase(compName)
#else
    LedComponentImpl(void)
#endif
  {

  }

  void LedComponentImpl ::
    init(
        const NATIVE_INT_TYPE instance
    )
  {
    LedComponentBase::init(instance);
    gioSetDirection(gioPORTA, 1);
  }

  LedComponentImpl ::
    ~LedComponentImpl(void)
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void LedComponentImpl ::
    Run_handler(
        const NATIVE_INT_TYPE portNum,
        NATIVE_UINT_TYPE context
    )
  {
    gioToggleBit(gioPORTA, 1);
  }

} // end namespace CubeRover
