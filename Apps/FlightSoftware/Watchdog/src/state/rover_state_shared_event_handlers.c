#include "include/state/rover_state_shared_event_handlers.h"

//######################################################################################################################
// Public Function Definitions
//######################################################################################################################

RoverStateController__Status RoverStateSharedHandlers__i2cStarted(RoverState__I2cStartedFuncArgs* args)
{
    if (NULL == args) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    args->state->spinningForI2cRead = TRUE;

    return RS_CONTROLLER__STATUS__SUCCESS;

}

RoverStateController__Status RoverStateSharedHandlers__i2cDone(RoverState__I2cDoneFuncArgs* args)
{
    if (NULL == args) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    args->state->spinningForI2cRead = FALSE;

    return RS_CONTROLLER__STATUS__SUCCESS;
}

RoverStateController__Status RoverStateSharedHandlers__spinSubmodulesOnce(RoverState__SpinOnceFuncArgs* args)
{
    if (NULL == args) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // TODO: Spin submodules here

    return RS_CONTROLLER__STATUS__SUCCESS;
}

RoverStateController__Status RoverStateSharedHandlers__ignoreHighTemp(RoverState__HighTempFuncArgs* args)
{
    if (NULL == args) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    return RS_CONTROLLER__STATUS__SUCCESS;
}

RoverStateController__Status RoverStateKeepAlive__ignorePowerIssue(RoverState__PowerIssueFuncArgs* args)
{
    if (NULL == args) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    return RS_CONTROLLER__STATUS__SUCCESS;
}

RoverStateController__Status RoverStateSharedHandlers__ignoreSpinOnce(RoverState__SpinOnceFuncArgs* args)
{
    if (NULL == args) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    return RS_CONTROLLER__STATUS__SUCCESS;
}
