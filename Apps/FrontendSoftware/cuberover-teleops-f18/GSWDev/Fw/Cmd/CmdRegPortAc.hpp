/*
 * CmdRegPort.hpp
 *
 *  Created on: Saturday, 23 March 2019
 *  Author:     raewynd
 *
 */
#ifndef CMDREGPORT_HPP_
#define CMDREGPORT_HPP_

#include <cstring>
#include <cstdio>
#include <Fw/Cfg/Config.hpp>
#include <Fw/Port/InputPortBase.hpp>
#include <Fw/Port/OutputPortBase.hpp>
#include <Fw/Comp/PassiveComponentBase.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Fw/Types/StringType.hpp>


namespace Fw {

    /// Input CmdReg port description
    /// 

    class InputCmdRegPort : public Fw::InputPortBase  {
    public:
        enum {
            SERIALIZED_SIZE = sizeof(FwOpcodeType) //!< serialized size of port arguments
        };
        typedef void (*CompFuncPtr)(Fw::PassiveComponentBase* callComp, NATIVE_INT_TYPE portNum, FwOpcodeType opCode); //!< port callback definition

        InputCmdRegPort(void); //!< constructor
        void init(void); //!< initialization function
        void addCallComp(Fw::PassiveComponentBase* callComp, CompFuncPtr funcPtr); //!< call to register a component
        void invoke(FwOpcodeType opCode); //!< invoke port interface
    protected:
    private:
        CompFuncPtr m_func; //!< pointer to port callback function
#if FW_PORT_SERIALIZATION == 1        
        void invokeSerial(Fw::SerializeBufferBase &buffer); //!< invoke the port with serialized arguments
#endif
};
    /// Input CmdReg port description
    /// 
    
    class OutputCmdRegPort : public Fw::OutputPortBase {
      public: 
        OutputCmdRegPort(void);
        void init(void);
        void addCallPort(InputCmdRegPort* callPort);
        void invoke(FwOpcodeType opCode);
      protected:
      private:
        InputCmdRegPort* m_port;
    };
} // end namespace Fw
#endif /* CMDREG_HPP_ */
