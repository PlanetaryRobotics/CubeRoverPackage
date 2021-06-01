#ifndef __WATCHDOG_ROVER_STATE_CONTROLLER_H__
#define __WATCHDOG_ROVER_STATE_CONTROLLER_H__

typedef enum RoverStateController__Status {
    RS_CONTROLLER__STATUS__SUCCESS = 0, //!< The function completed successfully.
    RS_CONTROLLER__STATUS__ERROR_NULL = -1, //!< An argument to the function was NULL.
    RS_CONTROLLER__STATUS__ERROR_NOT_INITIALIZED = -2, //!< The module was not initialized.
    RS_CONTROLLER__STATUS__ERROR_ALREADY_INITIALIZED = -3, //!< The module was already initialized.
    RS_CONTROLLER__STATUS__ERROR_SYSCALL_ERROR = -4, //!< A system call returned an error.
    RS_CONTROLLER__STATUS__ERROR_FAILED_TO_SEND_ALL = -5, //!< Failed to send all bytes.
    RS_CONTROLLER__STATUS__ERROR_TIMEOUT = -6, //!< The function timed out.
    RS_CONTROLLER__STATUS__ERROR_BAD_TRAJECTORY = -7, //!< A trajectory failed validation checks.
    RS_CONTROLLER__STATUS__ERROR_QUEUE_FULL = -8, //!< A queue was full, causing the function to fail.
    RS_CONTROLLER__STATUS__ERROR_BAD_STATE = -9, //!< The module was not in the correct state to do a certain action

    RS_CONTROLLER__STATUS__ERROR_FRAMEWORK_ERROR = -100, //!< A controller framework function failed.
    RS_CONTROLLER__STATUS__ERROR_INTERNAL_ERROR = -255, //!< An unexpected error occurred.
} RoverStateController__Status;

/**
 * A handle to a RoverStateController, which can be initialized by calling |RoverStateController__initialize|.
 */
typedef struct RoverStateController* RoverStateControllerPtr;

// TODO: Populate this with structs that will store the latest telemetry readings (heater values, ADC readings,
//       I2C readings, etc), and then add as a parameter to event handlers that produce or consume these values.
//       Add a single instance of this as a member of the RoverStateController instance, and pass it to the
//       appropriate event handlers in rover_state_controller.c
typedef struct RoverTelemetry
{

} RoverTelemetry;

/**
 * @brief Initializes the MotionController module.
 *
 * @warning The ControllerServer module must be initialized before this function is called.
 *
 * @param instance A return parameter that will be set equal to the initialized controller if the return status if
 *                 RS_CONTROLLER__STATUS__SUCCESS.
 *
 * @return MotionController__Status One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: |instance| was NULL.
 *   RS_CONTROLLER__STATUS__ERROR_ALREADY_INITIALIZED: The module was already initialized.
 *   RS_CONTROLLER__STATUS__ERROR_FRAMEWORK_ERROR: A controller framework function failed.
 */
RoverStateController__Status RoverStateController__initialize(RoverStateControllerPtr* instance);

void RoverStateController__spin(RoverStateControllerPtr instance);

#endif /* __WATCHDOG_ROVER_STATE_CONTROLLER_H__ */