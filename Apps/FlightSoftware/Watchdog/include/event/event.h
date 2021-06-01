#ifndef __WATCHDOG_EVENT_H__
#define __WATCHDOG_EVENT_H__

// TODO: What about heartbeat loss/acquisition?

typedef enum Event__Type
{
    EVENT__TYPE__UNUSED = 0, //!< Unused, if encountered it means the Event__Type was not set.
    EVENT__TYPE__LANDER_DATA = 1,
    EVENT__TYPE__HERCULES_DATA = 2,
    EVENT__TYPE__TIMER_TICK = 3,
    EVENT__TYPE__I2C_STARTED = 4,
    EVENT__TYPE__I2C_DONE = 5,
    EVENT__TYPE__HIGH_TEMP = 6,
    EVENT__TYPE__POWER_ISSUE = 7
} Event__Type;

#endif /* __WATCHDOG_EVENT_H__ */