#include "include/state/rover_state_controller.h"
#include "include/state/event.h"
#include "include/state/event_queue.h"
#include "include/common.h"

//###########################################################
// Private types
//###########################################################

typedef struct RoverStateController
{
    BOOL initialized;
    RoverState state;

    volatile uint8_t eventQueueBuffer[EVENT_QUEUE__MAX_SIZE];

    // TODO: Add RoverTelemetry and maybe a variable for watchdog flags (if we decide to keep the same approach as
    //       the existing watchdog code), both to be used to store persistent data through event invocations. These
    //       would both needs to be added as parameters to the appropriate event handlers, and then passed when the
    //       event handler is invoked in the RoverStateController__xxxEvent() private functions at the bottom of this
    //       file.
} RoverStateController;

//###########################################################
// Private globals and constants
//###########################################################

RoverStateController theController = {
    .initialized = FALSE,
    .state = { 0 },
    .eventQueueBuffer = { 0 }
};

//###########################################################
// Private function declarations
//###########################################################

RoverStateController__Status
RoverStateController__handleEvent(RoverStateControllerPtr instance,
                                  Event__Type event);

RoverStateController__Status
RoverStateController__landerDataEvent(RoverStateControllerPtr instance);

RoverStateController__Status
RoverStateController__herculesDataEvent(RoverStateControllerPtr instance);

RoverStateController__Status
RoverStateController__timerTickEvent(RoverStateControllerPtr instance);

RoverStateController__Status
RoverStateController__i2cStartedEvent(RoverStateControllerPtr instance);

RoverStateController__Status
RoverStateController__i2cDoneEvent(RoverStateControllerPtr instance);

RoverStateController__Status
RoverStateController__highTempEvent(RoverStateControllerPtr instance);

RoverStateController__Status
RoverStateController__powerIssueEvent(RoverStateControllerPtr instance);

RoverStateController__Status
RoverStateController__spinSubmodulesOnce(RoverStateControllerPtr instance);

//###########################################################
// Public function definitions
//###########################################################

RoverStateController__Status RoverStateController__initialize(RoverStateControllerPtr* instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    if (theController.initialized) {
        return RS_CONTROLLER__STATUS__ERROR_ALREADY_INITIALIZED;
    }

    // Initialize the state struct
    RoverStateController__Status rscStatus = RoverStateController__initialize(&(theController.state));

    if (RS_CONTROLLER__STATUS__SUCCESS != rscStatus) {
        return rscStatus;
    }

    // Initialize the global event queue
    EventQueue__Status eqStatus = EventQueue__initialize(theController.eventQueueBuffer,
                                                         SIZE_OF_ARRAY(theController.eventQueueBuffer));

    if (EQ__STATUS__SUCCESS != eqStatus) {
        return RS_CONTROLLER__STATUS__ERROR_FRAMEWORK_ERROR;
    }

    // Transition to our initial state.
    rscStatus = RoverStateKeepAlive__transitionTo(&theController.state);

    if (RS_CONTROLLER__STATUS__SUCCESS != rscStatus) {
        return rscStatus;
    }

    theController.initialized = TRUE;
    *instance = &theController;

    return RS_CONTROLLER__STATUS__SUCCESS;
}

void RoverStateController__spin(RoverStateControllerPtr instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    if (!theController.initialized) {
        return RS_CONTROLLER__STATUS__ERROR_NOT_INITIALIZED;
    }

    // Main loop of the watchdog, this should never be exited unless an error occurs
    while (TRUE) {
        Event__Type event = EVENT__TYPE__UNUSED;
        EventQueue__Status eqStatus = EventQueue__get(&event);

        if (EQ__STATUS__ERROR_EMPTY == eqStatus) {
            // TODO: Add checks for additional spinning status variables if/when they are added to RoverState
            if (!instance->state.spinningForI2cRead) {
                // TODO: Enter LPM here
            }
        } else if (EQ__STATUS__SUCCESS != eqStatus) {
            // TODO: Is this the right thing to do?
            // An unexpected error occurred. Return so that the entire watchdog resets
            return;
        }

        RoverStateController__Status rscStatus =
            RoverStateController__handleEvent(instance, event);

        if (RS_CONTROLLER__STATUS__SUCCESS != rscStatus) {
            // TODO: Is this the right thing to do?
            // An unexpected error occurred. Return so that the entire watchdog resets
            return;
        }

        // Do any passive spins here. We do these regardless of whether the spinning status variables
        // are set because they should be close to no-ops when the modules aren't actively doing anything.
        rscStatus = RoverStateController__spinSubmodulesOnce(instance);

        if (RS_CONTROLLER__STATUS__SUCCESS != rscStatus) {
            // TODO: Is this the right thing to do?
            // An unexpected error occurred. Return so that the entire watchdog resets
            return;
        }
    }
}

//###########################################################
// Private function definitions
//###########################################################

RoverStateController__Status
RoverStateController__handleEvent(RoverStateControllerPtr instance,
                                  Event__Type event)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    switch (event) {
        case EVENT__TYPE__LANDER_DATA:
            return RoverStateController__landerDataEvent(instance);

        case EVENT__TYPE__HERCULES_DATA:
            return RoverStateController__herculesDataEvent(instance);

        case EVENT__TYPE__TIMER_TICK:
            return RoverStateController__timerTickEvent(instance);

        case EVENT__TYPE__I2C_STARTED:
            return RoverStateController__i2cStartedEvent(instance);

        case EVENT__TYPE__I2C_DONE:
            return RoverStateController__i2cDoneEvent(instance);

        case EVENT__TYPE__HIGH_TEMP:
            return RoverStateController__highTempEvent(instance);

        case EVENT__TYPE__POWER_ISSUE:
            return RoverStateController__powerIssueEvent(instance);

        case EVENT__TYPE__UNUSED: // fall through
        default:
            // This shouldn't ever happen
            return RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR;
    }
}

RoverStateController__Status
RoverStateController__landerDataEvent(RoverStateControllerPtr instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // Pack up the arguments
    RoverState__LanderDataFuncArgs args = {
        .state = &(instance->state)
    };

    // Invoke the handler
    return instance->state.landerDataFunc(&args);
}

RoverStateController__Status
RoverStateController__herculesDataEvent(RoverStateControllerPtr instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // Pack up the arguments
    RoverState__HerculesDataFuncArgs args = {
        .state = &(instance->state)
    };

    // Invoke the handler
    return instance->state.herculesDataFunc(&args);
}

RoverStateController__Status
RoverStateController__timerTickEvent(RoverStateControllerPtr instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // Pack up the arguments
    RoverState__TimerTickFuncArgs args = {
        .state = &(instance->state)
    };

    // Invoke the handler
    return instance->state.timerTickFunc(&args);
}

RoverStateController__Status
RoverStateController__i2cStartedEvent(RoverStateControllerPtr instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // Pack up the arguments
    RoverState__I2cStartedFuncArgs args = {
        .state = &(instance->state)
    };

    // Invoke the handler
    return instance->state.i2cStartedFunc(&args);
}

RoverStateController__Status
RoverStateController__i2cDoneEvent(RoverStateControllerPtr instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // Pack up the arguments
    RoverState__I2cDoneFuncArgs args = {
        .state = &(instance->state)
    };

    // Invoke the handler
    return instance->state.i2cDoneFunc(&args);
}

RoverStateController__Status
RoverStateController__highTempEvent(RoverStateControllerPtr instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // Pack up the arguments
    RoverState__HighTempFuncArgs args = {
        .state = &(instance->state)
    };

    // Invoke the handler
    return instance->state.highTempFunc(&args);
}

RoverStateController__Status
RoverStateController__powerIssueEvent(RoverStateControllerPtr instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // Pack up the arguments
    RoverState__PowerIssueFuncArgs args = {
        .state = &(instance->state)
    };

    // Invoke the handler
    return instance->state.powerIssueFunc(&args);
}

RoverStateController__Status
RoverStateController__spinSubmodulesOnce(RoverStateControllerPtr instance)
{
    if (NULL == instance) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    // Pack up the arguments
    RoverState__SpinOnceFuncArgs args = {
        .state = &(instance->state)
    };

    // Invoke the handler
    return instance->state.spinOnceFunc(&args);
}