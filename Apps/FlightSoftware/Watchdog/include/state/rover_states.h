#ifndef __WATCHDOG_ROVER_STATES_H__
#define __WATCHDOG_ROVER_STATES_H__

// TODO: This naming convention isn't consistent, but I couldn't come up with a better one on the spot.
//       I'm open to suggestions.
typedef enum RoverState_t
{
    ROVER_STATE__UNKNOWN = 0, //!< Should only ever be used transiently during setup and state changes.
    ROVER_STATE__SLEEP = 1,
    ROVER_STATE__SERVICE = 2,
    ROVER_STATE__KEEPALIVE = 3,
    ROVER_STATE__INIT_MISSION = 4,
    ROVER_STATE__MISSION = 5,
    ROVER_STATE__FAULT = 6,
} RoverState_t;

#endif /* __WATCHDOG_ROVER_STATES_H__ */