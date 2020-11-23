#include <Os/Task.hpp>
#include <Os/Log.hpp>
#include <HAL/include/FreeRTOS.h>
#include <Include/CubeRoverConfig.hpp>      // PrimaryFlightController/FlightMCU

#include "Topology.hpp"




// ---------------------------------------------------------------------------
// Block Driver Component
// Block driver generates trigger signal for rate group driver
Drv::BlockDriverImpl blockDriver(
#if FW_OBJECT_NAMES == 1
  "BlockDriver"
#endif
);

// ---------------------------------------------------------------------------
// Rate Group Driver Component
// That array sets the frequency divider for the rate groups
static NATIVE_INT_TYPE rgDivs[] = {RATEGROUP_DIVIDER_HI_FREQ,
                                   RATEGROUP_DIVIDER_MED_FREQ,
                                   RATEGROUP_DIVIDER_LOW_FREQ};
Svc::RateGroupDriverImpl rateGroupDriver(
#if FW_OBJECT_NAMES == 1
  "RateGroupDriver",
#endif
  rgDivs,FW_NUM_ARRAY_ELEMENTS(rgDivs));

// ---------------------------------------------------------------------------
// Rate group - Low Frequency tasks
static NATIVE_UINT_TYPE rgLoFreqContext[] = {0,0,0,0};
Svc::ActiveRateGroupImpl rateGroupLowFreq(
#if FW_OBJECT_NAMES == 1
  "RateGroupLowFreq", 
#endif
  rgLoFreqContext, FW_NUM_ARRAY_ELEMENTS(rgLoFreqContext));

// ---------------------------------------------------------------------------
// Rate group - Medium Frequency tasks
static NATIVE_UINT_TYPE rgMedFreqContext[] = {0,0,0,0};
Svc::ActiveRateGroupImpl rateGroupMedFreq(
#if FW_OBJECT_NAMES == 1
  "RateGroupMedFreq",
#endif
  rgMedFreqContext, FW_NUM_ARRAY_ELEMENTS(rgMedFreqContext));

// ---------------------------------------------------------------------------
// Rate group - High Frequency tasks
static NATIVE_UINT_TYPE rgHiFreqContext[] = {0,0,0,0};
Svc::ActiveRateGroupImpl rateGroupHiFreq(
#if FW_OBJECT_NAMES == 1
  "RateGroupHiFreq",
#endif
  rgHiFreqContext, FW_NUM_ARRAY_ELEMENTS(rgHiFreqContext));

// ---------------------------------------------------------------------------
// Time - contains current CubeRover Time used for time stamping events
Svc::CubeRoverTimeImpl cubeRoverTime(
#if FW_OBJECT_NAMES == 1
  "CubeRoverTime"
#endif
  );

// ---------------------------------------------------------------------------
// Telemetric channel component used to centralized of telemetric data
Svc::TlmChanImpl tlmChan(
#if FW_OBJECT_NAMES == 1
  "TlmChan"
#endif
  );

// ---------------------------------------------------------------------------
// command dispatcher component used to dispatch commands
Svc::CommandDispatcherImpl cmdDispatcher(
#if FW_OBJECT_NAMES == 1
        "CmdDispatcher"
#endif
  );

// ---------------------------------------------------------------------------
// Active logger data component used to log system events
Svc::ActiveLoggerImpl activeLogger(
#if FW_OBJECT_NAMES == 1
        "ActiveLogger"
#endif
  );

// ---------------------------------------------------------------------------
// Active logger data component used to log system events
CubeRover::ComLogger comLogger(
#if FW_OBJECT_NAMES == 1
        "ComLogger",
#endif
        "",
        1024
  );

/**
 * @brief      Run 1 cycle (debug)
 */
void run1cycle(void) {
  blockDriver.callIsr();
}

/**
 * @brief      Construct the F-prime application
 */
void constructApp(void){
  //Initialize the block driver
  blockDriver.init(BLK_DRV_QUEUE_DEPTH);

  // Initialize rate group driver driver (passive)
  rateGroupDriver.init();

  // Initialize rate group components (active)
  rateGroupLowFreq.init(RG_LOW_FREQ_QUEUE_DEPTH, RG_LOW_FREQ_ID);
  rateGroupMedFreq.init(RG_MED_FREQ_QUEUE_DEPTH, RG_MED_FREQ_ID);
  rateGroupHiFreq.init(RG_HI_FREQ_QUEUE_DEPTH, RG_HI_FREQ_ID);

  // Initialize cubeRover time component (passive)
  cubeRoverTime.init(0);

  activeLogger.init(ACTIVE_LOGGER_QUEUE_DEPTH, ACTIVE_LOGGER_ID);

  // Initialize the comlogger component
  comLogger.init(100,0);

  // Initialize the telemetric channel component (active)
  tlmChan.init(TLM_CHAN_QUEUE_DEPTH, TLM_CHAN_ID);

  // Construct the application and make all connections between components
  constructCubeRoverArchitecture();

  rateGroupLowFreq.start(0, /* identifier */
                       RG_LOW_FREQ_AFF, /* Thread affinity */
                       RG_LOW_FREQ_QUEUE_DEPTH*MIN_STACK_SIZE_BYTES); /* stack size */

  rateGroupMedFreq.start(0, /* identifier */
                         RG_MED_FREQ_AFF, /* Thread affinity */
                         RG_MED_FREQ_QUEUE_DEPTH*MIN_STACK_SIZE_BYTES); /* stack size */

  rateGroupHiFreq.start(0, /* identifier */
                         RG_HI_FREQ_AFF, /* Thread affinity */
                         RG_HI_FREQ_QUEUE_DEPTH*MIN_STACK_SIZE_BYTES); /* stack size */

  blockDriver.start(0, /* identifier */
                   BLK_DRV_AFF, /* Thread affinity */
                   BLK_DRV_QUEUE_DEPTH*MIN_STACK_SIZE_BYTES); /* stack size */

  tlmChan.start(0, /* identifier */
                TLM_CHAN_AFF, /* thread affinity */
                TLM_CHAN_QUEUE_DEPTH*MIN_STACK_SIZE_BYTES); /* stack size */

  activeLogger.start(ACTIVE_LOGGER_ID, /* identifier */
                     ACTIVE_LOGGER_AFF, /* CPU priority  */
                     ACTIVE_LOGGER_QUEUE_DEPTH*MIN_STACK_SIZE_BYTES); /* stack size */

}
