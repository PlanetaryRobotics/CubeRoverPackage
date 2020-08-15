#include <Drv/BlockDriver/BlockDriverImpl.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/Assert.hpp>

namespace Drv {

#if FW_OBJECT_NAMES == 1
    BlockDriverImpl::BlockDriverImpl(const char* compName) :
        BlockDriverComponentBase(compName)
#else
    BlockDriverImpl::BlockDriverImpl()
#endif
    ,m_cycles(0)
    {

    }
    void BlockDriverImpl::init(NATIVE_INT_TYPE queueDepth) {
        BlockDriverComponentBase::init(queueDepth);
    }
    
    BlockDriverImpl::~BlockDriverImpl(void) {

    }

    void BlockDriverImpl::InterruptReport_internalInterfaceHandler(U32 ip) {
        // get time
        Svc::TimerVal timer;
        timer.take();
        // call output timing signal
        this->CycleOut_out(0,timer);
        // increment cycles and write channel
        this->tlmWrite_BD_Cycles(this->m_cycles);
        this->m_cycles++;
    }

    void BlockDriverImpl::BufferIn_handler(NATIVE_INT_TYPE portNum, Drv::DataBuffer& buffer) {
        // just a pass-through
        this->BufferOut_out(0,buffer);
    }
    
    void BlockDriverImpl::Sched_handler(NATIVE_INT_TYPE portNum, NATIVE_UINT_TYPE context) {
    }

    void BlockDriverImpl::callIsr(void) {
        s_driverISR(this);
    }
    
    void BlockDriverImpl::s_driverISR(void* arg) {
        FW_ASSERT(arg);
        // cast argument to component instance
        BlockDriverImpl* compPtr = static_cast<BlockDriverImpl*>(arg);
        compPtr->InterruptReport_internalInterfaceHandler(0);
    }

    void BlockDriverImpl::PingIn_handler(
            const NATIVE_INT_TYPE portNum,
            U32 key
        )
      {
        // call ping output port
        this->PingOut_out(0,key);
      }


}
