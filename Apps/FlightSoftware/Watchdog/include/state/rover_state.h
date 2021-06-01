#ifndef __WATCHDOG_ROVER_STATE_H__
#define __WATCHDOG_ROVER_STATE_H__

#include "include/state/rover_states.h"
#include "include/state/rover_state_controller.h"

typedef struct RoverState RoverState;
typedef RoverState* RoverStatePtr;

//######################################################################################################################
// State-specific Event Handling Function Arguments and Signatures
//######################################################################################################################

// Note: Passing the arguments to each of the state-specific function handlers in a structure *does* mean that there is
//       extra processing used to pack and unpack the arguments. However, the benefit gained is that if the arguments
//       needed by a given function change, it becomes **MUCH** easier to modify them using this approach. If the
//       arguments were not in structures, the function signature in this file, the function call in RoverStateController,
//       and the signature of both the declaration and definition of the implementation in *every single concrete state
//       implementation* would need to modified to reflect the new signature. By passing a struct of arguments, the only
//       changes that need to be made are adding the new field to the appropriate arguments struct in this file, adding
//       the field to the code that packs up the arguments in MotionController, and then only the state-specific
//       implementations that actually need to use the new argument need to be changed.

/**
 * @brief The arguments used by the state-specific function that is called when data is received from the lander.
 */
typedef struct RoverState__LanderDataFuncArgs {
    RoverStatePtr state; //!< The current state.
} RoverState__LanderDataFuncArgs;

/**
 * @brief The arguments used by the state-specific function that is called when data is received from the Hercules.
 */
typedef struct RoverState__HerculesDataFuncArgs {
    RoverStatePtr state; //!< The current state.
} RoverState__HerculesDataFuncArgs;

/**
 * @brief The arguments used by the state-specific function that is called when the timer ticks.
 */
typedef struct RoverState__TimerTickFuncArgs {
    RoverStatePtr state; //!< The current state.
} RoverState__TimerTickFuncArgs;

/**
 * @brief The arguments used by the state-specific function that is called when an asynchronous I2C read is started.
 */
typedef struct RoverState__I2cStartedFuncArgs {
    RoverStatePtr state; //!< The current state.
} RoverState__I2cStartedFuncArgs;

/**
 * @brief The arguments used by the state-specific function that is called when an asynchronous I2C read is completed
 *        (either because it completed successfully or because it failed or was stopped).
 */
typedef struct RoverState__I2cDoneFuncArgs {
    RoverStatePtr state; //!< The current state.
} RoverState__I2cDoneFuncArgs;

/**
 * @brief The arguments used by the state-specific function that is called when a high temperature is detected.
 */
typedef struct RoverState__HighTempFuncArgs {
    RoverStatePtr state; //!< The current state.
} RoverState__HighTempFuncArgs;

/**
 * @brief The arguments used by the state-specific function that is called when a power issue is detected.
 */
typedef struct RoverState__PowerIssueFuncArgs {
    RoverStatePtr state; //!< The current state.
} RoverState__PowerIssueFuncArgs;

/**
 * @brief The arguments used by the state-specific function that implements the behavior of invoking the spinOnce()
 *        functions of modules that need it (e.g. I2C_Sensors). This is called once per loop of the rover state
 *        controller.
 */
typedef struct RoverState__SpinOnceFuncArgs {
    RoverStatePtr state; //!< The current state.
    // TODO: Add parameters needed to spin I2C_Sensors and any other modules that need to be spun
} RoverState__SpinOnceFuncArgs;

/**
 * @brief The function pointer signature for a function that is called when data is received from the lander.
 */
typedef RoverStateController__Status(*RoverState__LanderDataFunc)(RoverState__LanderDataFuncArgs*);

/**
 * @brief The function pointer signature for a function that is called when data is received from the Hercules.
 */
typedef RoverStateController__Status(*RoverState__HerculesDataFunc)(RoverState__HerculesDataFuncArgs*);

/**
 * @brief The function pointer signature for a function that is called when the timer ticks.
 */
typedef RoverStateController__Status(*RoverState__TimerTickFunc)(RoverState__TimerTickFuncArgs*);

/**
 * @brief The function pointer signature for a function that is called when an asynchronous I2C read is started.
 */
typedef RoverStateController__Status(*RoverState__I2cStartedFunc)(RoverState__I2cStartedFuncArgs*);

/**
 * @brief The function pointer signature for a function that is called when an asynchronous I2C read is completed
 *        (either because it completed successfully or because it failed or was stopped).
 */
typedef RoverStateController__Status(*RoverState__I2cDoneFunc)(RoverState__I2cDoneFuncArgs*);

/**
 * @brief The function pointer signature for a function that is called when a high temperature is detected.
 */
typedef RoverStateController__Status(*RoverState__HighTempFunc)(RoverState__HighTempFuncArgs*);

/**
 * @brief The function pointer signature for a function that is called when a power issue is detected.
 */
typedef RoverStateController__Status(*RoverState__PowerIssueFunc)(RoverState__PowerIssueFuncArgs*);

/**
 * @brief The function pointer signature for a function that implements the behavior of invoking the spinOnce()
 *        functions of modules that need it (e.g. I2C_Sensors). This is called once per loop of the rover state
 *        controller.
 */
typedef RoverStateController__Status(*RoverState__SpinOnceFunc)(RoverState__SpinOnceFuncArgs*);

//######################################################################################################################
// State Data Types
//######################################################################################################################

/**
 * @brief The contents of the Rover Controller State. This mostly contains function pointers to state-dependent
 *        function implementations, but also some state data
 */
struct RoverState {
    RoverState_t currentState; //!< The current state.

    /**
     * @brief Whether or not the chip should stay active (i.e. out of LPM) and continue spinning the I2C module.
     */
    BOOL spinningForI2cRead;

    /**
     * @brief The state-dependent function that is called when data is received from the lander.
     */
    RoverState__LanderDataFunc landerDataFunc;

    /**
     * @brief The state-dependent function that is called when data is received from the Hercules.
     */
    RoverState__HerculesDataFunc herculesDataFunc;

    /**
     * @brief The state-dependent function that is called when the timer ticks.
     */
    RoverState__TimerTickFunc timerTickFunc;

    /**
     * @brief The state-dependent function that is called when an asynchronous I2C read is started.
     */
    RoverState__I2cStartedFunc i2cStartedFunc;

    /**
     * @brief The state-dependent function that is called when an asynchronous I2C read is completed
     *        (either because it completed successfully or because it failed or was stopped).
     */
    RoverState__I2cDoneFunc i2cDoneFunc;

    /**
     * @brief The state-dependent function that is called when a high temperature is detected.
     */
    RoverState__HighTempFunc highTempFunc;

    /**
     * @brief The state-dependent function that is called when a power issue is detected.
     */
    RoverState__PowerIssueFunc powerIssueFunc;

    /**
     * @brief The state-dependent function that implements the behavior of invoking the spinOnce() functions of
     *        modules that need it (e.g. I2C_Sensors). This is called once per loop of the rover state controller.
     */
    RoverState__SpinOnceFunc spinOnceFunc;
};

//######################################################################################################################
// Common State Functions
//######################################################################################################################

/**
 * @brief Initializes the state data of the given state.
 *
 * @note This must only be called once for a given instance of RoverState.
 *
 * @param state The state to initialize.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: |state| was NULL.
 */
RoverStateController__Status RoverState__initialize(RoverStatePtr state);

/**
 * @brief Sets the given state so that all state-dependent function pointers, the state type, and the spinning status
 *        fields are all their default values.
 *
 * @note The default value for all state-dependent function pointers are implementations that will assert that they
 *       should never be invoked.
 *
 * @note This function can be called repeatedly for a given instance of RoverState. It is expected during a state
 *       transition, prior to setting any state-dependent data or function pointers this function will be called. This
 *       allows a given state to only implement the handlers for the event that it expects to handle. In other words, if
 *       an event should not be occurring in a given state, then that state need not implement a handler for that event.
 *       Similarly, since all state-specific data is reset in this function, specific state implementations don't need
 *       to worry about resetting the state data used by other states.
 *
 * @param state The state to reset to default.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: |state| was NULL.
 */
RoverStateController__Status RoverState__defaultImplementation(RoverStatePtr state);

#endif /* __WATCHDOG_ROVER_STATE_H__ */