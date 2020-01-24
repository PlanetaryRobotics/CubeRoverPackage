
#include <CmuApp/Top/Components.hpp>

//Set base id section
void setRefIds(void){
	fatalHandler.setIdBase(1);
	rateGroup1Comp.setIdBase(21);
	rateCounter.setIdBase(41);
	chanTlm.setIdBase(61);
	cmdDisp.setIdBase(121);
	prmDb.setIdBase(141);
	rateGroup2Comp.setIdBase(161);
	rateGroup3Comp.setIdBase(241);
	fileUplink.setIdBase(261);
	fileUplinkBufferManager.setIdBase(301);
	fatalAdapter.setIdBase(341);
	health.setIdBase(361);
	sockGndIf.setIdBase(381);
	fileDownlinkBufferManager.setIdBase(401);
	eventLogger.setIdBase(421);
	linuxTime.setIdBase(441);
	rateGroupDriverComp.setIdBase(461);
	blockDrv.setIdBase(481);
	fileDownlink.setIdBase(501);
	textLogger.setIdBase(521);
	cmdSeq.setIdBase(541);
}




void constructRefArchitecture(void) {  
	setRefIds();

    // Port connections
    //None
    rateGroup3Comp.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    rateGroup3Comp.set_RateGroupMemberOut_OutputPort(0, health.get_Run_InputPort(0));
    //None
    rateGroup3Comp.set_Tlm_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    rateGroup3Comp.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    rateGroup3Comp.set_RateGroupMemberOut_OutputPort(2, blockDrv.get_Sched_InputPort(0));
    //None
    rateGroup3Comp.set_PingOut_OutputPort(0, health.get_PingReturn_InputPort(9));
    //None
    rateGroup3Comp.set_Log_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    fileUplink.set_tlmOut_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    fileUplink.set_eventOut_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    fileUplink.set_bufferSendOut_OutputPort(0, fileUplinkBufferManager.get_bufferSendIn_InputPort(0));
    //None
    fileUplink.set_pingOut_OutputPort(0, health.get_PingReturn_InputPort(5));
    //None
    fileUplink.set_timeCaller_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    fileUplinkBufferManager.set_tlmOut_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    fileUplinkBufferManager.set_eventOut_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    fileUplinkBufferManager.set_timeCaller_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    fatalAdapter.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    fatalAdapter.set_Log_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    fatalAdapter.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    rateGroup1Comp.set_PingOut_OutputPort(0, health.get_PingReturn_InputPort(7));
    //None
    rateGroup1Comp.set_RateGroupMemberOut_OutputPort(2, chanTlm.get_Run_InputPort(0));
    //None
    rateGroup1Comp.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    rateGroup1Comp.set_RateGroupMemberOut_OutputPort(2, rateCounter.get_schedIn_InputPort(0));
    //None
    rateGroup1Comp.set_RateGroupMemberOut_OutputPort(0, rateCounter.get_schedIn_InputPort(0));
    //None
    rateGroup1Comp.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    rateGroup1Comp.set_Tlm_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    rateGroup1Comp.set_Log_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    chanTlm.set_pingOut_OutputPort(0, health.get_PingReturn_InputPort(0));
    //None
    chanTlm.set_PktSend_OutputPort(0, sockGndIf.get_downlinkPort_InputPort(0));
    //None
    rateCounter.set_tlmOut_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    rateCounter.set_timeCaller_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    rateCounter.set_logOut_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    rateCounter.set_logTextOut_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    rateCounter.set_cmdResponseOut_OutputPort(0, cmdDisp.get_compCmdStat_InputPort(0));
    //None
    rateCounter.set_cmdRegOut_OutputPort(0, cmdDisp.get_compCmdReg_InputPort(6));
    //None
    health.set_PingSend_OutputPort(5, fileUplink.get_pingIn_InputPort(0));
    //None
    health.set_PingSend_OutputPort(8, rateGroup2Comp.get_PingIn_InputPort(0));
    //None
    health.set_PingSend_OutputPort(0, chanTlm.get_pingIn_InputPort(0));
    //None
    health.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    health.set_PingSend_OutputPort(1, cmdDisp.get_pingIn_InputPort(0));
    //None
    health.set_PingSend_OutputPort(11, blockDrv.get_PingIn_InputPort(0));
    //None
    health.set_PingSend_OutputPort(7, rateGroup1Comp.get_PingIn_InputPort(0));
    //None
    health.set_Log_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    health.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    health.set_PingSend_OutputPort(3, eventLogger.get_pingIn_InputPort(0));
    //None
    health.set_PingSend_OutputPort(2, cmdSeq.get_pingIn_InputPort(0));
    //None
    health.set_Tlm_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    health.set_PingSend_OutputPort(9, rateGroup3Comp.get_PingIn_InputPort(0));
    //None
    health.set_PingSend_OutputPort(6, prmDb.get_pingIn_InputPort(0));
    //None
    health.set_CmdStatus_OutputPort(0, cmdDisp.get_compCmdStat_InputPort(0));
    //None
    health.set_PingSend_OutputPort(4, fileDownlink.get_pingIn_InputPort(0));
    //None
    health.set_CmdReg_OutputPort(0, cmdDisp.get_compCmdReg_InputPort(3));
    //None
    sockGndIf.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    sockGndIf.set_Log_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    sockGndIf.set_fileUplinkBufferGet_OutputPort(0, fileUplinkBufferManager.get_bufferGetCallee_InputPort(0));
    //None
    sockGndIf.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    sockGndIf.set_fileUplinkBufferSendOut_OutputPort(0, fileUplink.get_bufferSendIn_InputPort(0));
    //None
    sockGndIf.set_fileDownlinkBufferSendOut_OutputPort(0, fileDownlinkBufferManager.get_bufferSendIn_InputPort(0));
    //None
    sockGndIf.set_uplinkPort_OutputPort(0, cmdDisp.get_seqCmdBuff_InputPort(1));
    //None
    fileDownlinkBufferManager.set_tlmOut_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    fileDownlinkBufferManager.set_eventOut_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    fileDownlinkBufferManager.set_timeCaller_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    eventLogger.set_CmdStatus_OutputPort(0, cmdDisp.get_compCmdStat_InputPort(0));
    //None
    eventLogger.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    eventLogger.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    eventLogger.set_Log_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    eventLogger.set_pingOut_OutputPort(0, health.get_PingReturn_InputPort(3));
    //None
    eventLogger.set_FatalAnnounce_OutputPort(0, fatalHandler.get_FatalReceive_InputPort(0));
    //None
    eventLogger.set_PktSend_OutputPort(0, sockGndIf.get_downlinkPort_InputPort(0));
    //None
    eventLogger.set_CmdReg_OutputPort(0, cmdDisp.get_compCmdReg_InputPort(0));
    //None
    cmdDisp.set_Log_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    cmdDisp.set_CmdStatus_OutputPort(0, cmdDisp.get_compCmdStat_InputPort(0));
    //None
    cmdDisp.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    cmdDisp.set_Tlm_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    cmdDisp.set_pingOut_OutputPort(0, health.get_PingReturn_InputPort(1));
    //None
    cmdDisp.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    cmdDisp.set_compCmdSend_OutputPort(0, eventLogger.get_CmdDisp_InputPort(0));
    //None
    cmdDisp.set_compCmdSend_OutputPort(1, fileDownlink.get_cmdIn_InputPort(0));
    //None
    cmdDisp.set_seqCmdStatus_OutputPort(0, cmdSeq.get_cmdResponseIn_InputPort(0));
    //None
    cmdDisp.set_compCmdSend_OutputPort(2, cmdDisp.get_CmdDisp_InputPort(0));
    //None
    cmdDisp.set_CmdReg_OutputPort(0, cmdDisp.get_compCmdReg_InputPort(2));
    //None
    cmdDisp.set_compCmdSend_OutputPort(3, health.get_CmdDisp_InputPort(0));
    //None
    cmdDisp.set_compCmdSend_OutputPort(4, prmDb.get_CmdDisp_InputPort(0));
    //None
    cmdDisp.set_compCmdSend_OutputPort(5, cmdSeq.get_cmdIn_InputPort(0));
    //None
    cmdDisp.set_compCmdSend_OutputPort(6, rateCounter.get_cmdIn_InputPort(0));
    //None
    prmDb.set_pingOut_OutputPort(0, health.get_PingReturn_InputPort(6));
    //None
    prmDb.set_Log_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    prmDb.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    prmDb.set_CmdStatus_OutputPort(0, cmdDisp.get_compCmdStat_InputPort(0));
    //None
    prmDb.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    prmDb.set_CmdReg_OutputPort(0, cmdDisp.get_compCmdReg_InputPort(4));
    //None
    rateGroup2Comp.set_Tlm_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    rateGroup2Comp.set_PingOut_OutputPort(0, health.get_PingReturn_InputPort(8));
    //None
    rateGroup2Comp.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    rateGroup2Comp.set_Log_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    rateGroup2Comp.set_RateGroupMemberOut_OutputPort(0, cmdSeq.get_schedIn_InputPort(0));
    //None
    rateGroup2Comp.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    rateGroupDriverComp.set_CycleOut_OutputPort(1, rateGroup2Comp.get_CycleIn_InputPort(0));
    //None
    rateGroupDriverComp.set_CycleOut_OutputPort(2, rateGroup3Comp.get_CycleIn_InputPort(0));
    //None
    rateGroupDriverComp.set_CycleOut_OutputPort(0, rateGroup1Comp.get_CycleIn_InputPort(0));
    //None
    blockDrv.set_PingOut_OutputPort(0, health.get_PingReturn_InputPort(11));
    //None
    blockDrv.set_CycleOut_OutputPort(0, rateGroupDriverComp.get_CycleIn_InputPort(0));
    //None
    blockDrv.set_Tlm_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    blockDrv.set_Time_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    cmdSeq.set_LogText_OutputPort(0, textLogger.get_TextLogger_InputPort(0));
    //None
    cmdSeq.set_cmdResponseOut_OutputPort(0, cmdDisp.get_compCmdStat_InputPort(0));
    //None
    cmdSeq.set_tlmOut_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    cmdSeq.set_logOut_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    cmdSeq.set_timeCaller_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    cmdSeq.set_pingOut_OutputPort(0, health.get_PingReturn_InputPort(2));
    //None
    cmdSeq.set_comCmdOut_OutputPort(0, cmdDisp.get_seqCmdBuff_InputPort(0));
    //None
    cmdSeq.set_cmdRegOut_OutputPort(0, cmdDisp.get_compCmdReg_InputPort(5));
    //None
    fileDownlink.set_tlmOut_OutputPort(0, chanTlm.get_TlmRecv_InputPort(0));
    //None
    fileDownlink.set_cmdResponseOut_OutputPort(0, cmdDisp.get_compCmdStat_InputPort(0));
    //None
    fileDownlink.set_pingOut_OutputPort(0, health.get_PingReturn_InputPort(4));
    //None
    fileDownlink.set_eventOut_OutputPort(0, eventLogger.get_LogRecv_InputPort(0));
    //None
    fileDownlink.set_timeCaller_OutputPort(0, linuxTime.get_timeGetPort_InputPort(0));
    //None
    fileDownlink.set_bufferSendOut_OutputPort(0, sockGndIf.get_fileDownlinkBufferSendIn_InputPort(0));
    //None
    fileDownlink.set_bufferGetCaller_OutputPort(0, fileDownlinkBufferManager.get_bufferGetCallee_InputPort(0));
    //None
    fileDownlink.set_cmdRegOut_OutputPort(0, cmdDisp.get_compCmdReg_InputPort(1));

}





