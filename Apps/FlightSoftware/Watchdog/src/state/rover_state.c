#include <assert.h>

#include "include/state/rover_state.h"

//###########################################################
// Private function declarations
//###########################################################

/**
 * @brief Always fails an assertion that this function should never be invoked.
 */
static RoverStateController__Status RoverState__defaultLanderDataFunc(RoverState__LanderDataFuncArgs* args);

/**
 * @brief Always fails an assertion that this function should never be invoked.
 */
static RoverStateController__Status RoverState__defaultHerculesDataFunc(RoverState__HerculesDataFuncArgs* args);

/**
 * @brief Always fails an assertion that this function should never be invoked.
 */
static RoverStateController__Status RoverState__defaultTimerTickFunc(RoverState__TimerTickFuncArgs* args);

/**
 * @brief Always fails an assertion that this function should never be invoked.
 */
static RoverStateController__Status RoverState__defaultI2cStartedFunc(RoverState__I2cStartedFuncArgs* args);

/**
 * @brief Always fails an assertion that this function should never be invoked.
 */
static RoverStateController__Status RoverState__defaultI2cDoneFunc(RoverState__I2cDoneFuncArgs* args);

/**
 * @brief Always fails an assertion that this function should never be invoked.
 */
static RoverStateController__Status RoverState__defaultHighTempFunc(RoverState__HighTempFuncArgs* args);

/**
 * @brief Always fails an assertion that this function should never be invoked.
 */
static RoverStateController__Status RoverState__defaultPowerIssueFunc(RoverState__PowerIssueFuncArgs* args);

/**
 * @brief Always fails an assertion that this function should never be invoked.
 */
static RoverStateController__Status RoverState__defaultSpinOnceFunc(RoverState__SpinOnceFuncArgs* args);

//###########################################################
// Public function definitions
//###########################################################

RoverStateController__Status RoverState__initialize(RoverStatePtr state)
{
    if (NULL == state) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // Currently this does nothing but I'm leaving it around in case we ever add anything to RoverState that
    // needs to be initialized.
    return RS_CONTROLLER__STATUS__SUCCESS;
}

RoverStateController__Status RoverState__defaultImplementation(RoverStatePtr state)
{
    if (NULL == state) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    state->currentState = ROVER_STATE__UNKNOWN;
    state->spinningForI2cRead = FALSE;

    state->landerDataFunc = RoverState__defaultLanderDataFunc;
    state->herculesDataFunc = RoverState__defaultHerculesDataFunc;
    state->timerTickFunc = RoverState__defaultTimerTickFunc;
    state->i2cStartedFunc = RoverState__defaultI2cStartedFunc;
    state->i2cDoneFunc = RoverState__defaultI2cDoneFunc;
    state->highTempFunc = RoverState__defaultHighTempFunc;
    state->powerIssueFunc = RoverState__defaultPowerIssueFunc;
    state->spinOnceFunc = RoverState__defaultSpinOnceFunc;

    return RS_CONTROLLER__STATUS__SUCCESS;
}

//###########################################################
// Private function definitions
//###########################################################

static RoverStateController__Status RoverState__defaultLanderDataFunc(RoverState__LanderDataFuncArgs* /* args */)
{
    assert(!"Default lander data event handler function invoked, this should never happen");
    return RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR; // Should never return
}

static RoverStateController__Status RoverState__defaultHerculesDataFunc(RoverState__HerculesDataFuncArgs* /* args */)
{
    assert(!"Default hercules data event handler function invoked, this should never happen");
    return RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR; // Should never return
}

static RoverStateController__Status RoverState__defaultTimerTickFunc(RoverState__TimerTickFuncArgs* /* args */)
{
    assert(!"Default timer tick event handler function invoked, this should never happen");
    return RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR; // Should never return
}

static RoverStateController__Status RoverState__defaultI2cStartedFunc(RoverState__I2cStartedFuncArgs* /* args */);
{
    assert(!"Default I2C started event handler function invoked, this should never happen");
    return RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR; // Should never return
}
static RoverStateController__Status RoverState__defaultI2cDoneFunc(RoverState__I2cDoneFuncArgs* /* args */)
{
    assert(!"Default I2C done event handler function invoked, this should never happen");
    return RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR; // Should never return
}

static RoverStateController__Status RoverState__defaultHighTempFunc(RoverState__HighTempFuncArgs* /* args */)
{
    assert(!"Default high temperature event handler function invoked, this should never happen");
    return RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR; // Should never return
}

static RoverStateController__Status RoverState__defaultPowerIssueFunc(RoverState__PowerIssueFuncArgs* /* args */)
{
    assert(!"Default power issue event handler function invoked, this should never happen");
    return RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR; // Should never return
}

static RoverStateController__Status RoverState__defaultSpinOnceFunc(RoverState__SpinOnceFuncArgs* /* args */)
{
    assert(!"Default spinOnce() function invoked, this should never happen");
    return RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR; // Should never return
}
