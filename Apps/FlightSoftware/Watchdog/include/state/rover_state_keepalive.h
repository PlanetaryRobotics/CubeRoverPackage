#ifndef __WATCHDOG_ROVER_STATE_KEEPALIVE_H__
#define __WATCHDOG_ROVER_STATE_KEEPALIVE_H__

#include "include/state/rover_state.h"

RoverStateController__Status RoverStateKeepAlive__transitionTo(RoverStatePtr state);

#endif /* __WATCHDOG_ROVER_STATE_KEEPALIVE_H__ */