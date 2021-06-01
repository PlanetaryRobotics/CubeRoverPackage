#ifndef __WATCHDOG_ROVER_STATE_SHARED_EVENT_HANDLERS_H__
#define __WATCHDOG_ROVER_STATE_SHARED_EVENT_HANDLERS_H__

#include "include/state/rover_state.h"

/**
 * @brief Sets the |spinningForI2cRead| field of the RoverState to TRUE so that the controller will continue
 *        to spin I2C until it's done, rather than entering LPM once it handles all events.
 *
 * @param args The arguments to this handler.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: An argument was NULL.
 */
RoverStateController__Status RoverStateSharedHandlers__i2cStarted(RoverState__I2cStartedFuncArgs* args);

/**
 * @brief Sets the |spinningForI2cRead| field of the RoverState to FALSE so that the controller can enter LPM
 *        (if nothing else it making it continue to spin).
 *
 * @param args The arguments to this handler.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: An argument was NULL.
 */
RoverStateController__Status RoverStateSharedHandlers__i2cDone(RoverState__I2cDoneFuncArgs* args);

/**
 * @brief Spins all submodules.
 *
 * // TODO: Update args and actually spin submodules in the implementation
 *
 * @param args The arguments to this handler.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: An argument was NULL.
 */
RoverStateController__Status RoverStateSharedHandlers__spinSubmodulesOnce(RoverState__SpinOnceFuncArgs* args);

/**
 * @brief Ignores the high temperature, doing nothing.
 *
 * It only makes sense to react to this (disabling the heaters) when the rover is attached to the lander. When that
 * is not the case then we do nothing.
 *
 * @param args The arguments to this handler.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: An argument was NULL.
 */
RoverStateController__Status RoverStateSharedHandlers__ignoreHighTemp(RoverState__HighTempFuncArgs* args);

/**
 * @brief Ignores the power issue, doing nothing.
 *
 * TODO: Existing code has a "TODO" to handle this, but only when in MISSION mode. Is that correct?
 *
 * @param args The arguments to this handler.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: An argument was NULL.
 */
RoverStateController__Status RoverStateSharedHandlers__ignorePowerIssue(RoverState__PowerIssueFuncArgs* args);

/**
 * @brief Does nothing.
 *
 * @param args The arguments to this handler.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: An argument was NULL.
 */
RoverStateController__Status RoverStateSharedHandlers__ignoreSpinOnce(RoverState__SpinOnceFuncArgs* args);

#endif /* __WATCHDOG_ROVER_STATE_SHARED_EVENT_HANDLERS_H__ */