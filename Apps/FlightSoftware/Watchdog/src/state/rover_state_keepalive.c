#include "include/state/rover_state_keepalive.h"
#include "include/state/rover_state_shared_event_handlers.h"
#include "include/drivers/bsp.h"

//######################################################################################################################
// EVENT HANDLING Private Function Forward Declarations
//######################################################################################################################

/**
 * @brief Attempts to get a message from the lander data, if a full message worth of data has been received. If it has,
 *        this processes that message, potentially transitioning to a new state.
 *
 * @param args The arguments to this handler.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: An argument was NULL.
 */
static RoverStateController__Status RoverStateKeepAlive__landerData(RoverState__LanderDataFuncArgs* args);

/**
 * @brief Sample the ADC, send earth heartbeat every three ticks, and calculate PWM duty cycle for heater if heating
 *        control enabled.
 *
 * @param args The arguments to this handler.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: An argument was NULL.
 */
static RoverStateController__Status RoverStateKeepAlive__timerTick(RoverState__TimerTickFuncArgs* args);

/**
 * @brief Disable the heater, as we should be connected to the lander in this state.
 *
 * @param args The arguments to this handler.
 *
 * @return One of the following:
 *   RS_CONTROLLER__STATUS__SUCCESS: The function completed successfully.
 *   RS_CONTROLLER__STATUS__ERROR_NULL: An argument was NULL.
 */
static RoverStateController__Status RoverStateKeepAlive__highTemp(RoverState__HighTempFuncArgs* args);


//######################################################################################################################
// Public Function Definitions
//######################################################################################################################

RoverStateController__Status RoverStateKeepAlive__transitionTo(RoverStatePtr state)
{
    /* power everything off and set resets */
    powerOffFpga();
    powerOffMotors();
    powerOffRadio();
    powerOffHercules();
    setRadioReset();
    setFPGAReset();
    setMotorsReset();
    setHerculesReset();
    unsetDeploy();

    /* turn off voltage rails */
    disable3V3PowerRail();
    disable24VPowerRail();
    disableBatteries();

    /* monitor only lander voltages */
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // TODO: Either change adc_setup_lander() to have a constant runtime or add a new
    //       state called "INIT_KEEPALIVE" that we stay in until the work currently done
    //       in adc_setup_lander() is complete, and THEN transition to KEEPALIVE. State
    //       transitions should have a constant runtime, and the while() loop in the
    //       current implementation of adc_setup_lander() makes that not the case for
    //       this transition function.
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    adc_setup_lander();
    enableHeater();
    startChargingBatteries();

    // If we are in the middle of an I2C measurement we need to stop it since we don't service I2C in KEEPALIVE.
    // TODO: Implement I2C__stop() and I2C_Sensors__stop() functions and call them here if an I2C read is active.

    RoverStateController__Status rscState = RoverState__defaultImplementation(state);

    if (RS_CONTROLLER__STATUS__SUCCESS != rscState) {
        return rscState;
    }

    state->currentState = ROVER_STATE__KEEPALIVE;
    state->spinningForI2cRead = FALSE;

    // This state has unique handling for these events
    state->landerDataFunc = RoverStateKeepAlive__landerData;
    state->timerTickFunc = RoverStateKeepAlive__timerTick;
    state->highTempFunc = RoverStateKeepAlive__highTemp;

    // Leave the Hercules data event function as default, since the Hercules should be off in this state
    // Leave the two I2C event functions as default, as we don't expect them to be called in this state

    // Use shared implentations for these events.
    state->powerIssueFunc = RoverStateSharedHandlers__ignorePowerIssue;
    state->spinOnceFunc = RoverStateSharedHandlers__spinSubmodulesOnce;
    // TODO: Do we want to spin our submodules here or should we be ignoring the spinOnce() and doing nothing?

    return RS_CONTROLLER__STATUS__SUCCESS;
}


//######################################################################################################################
// EVENT HANDLING Private Function Definitions
//######################################################################################################################

static RoverStateController__Status RoverStateKeepAlive__landerData(RoverState__LanderDataFuncArgs* args)
{
    // TODO: Get data from LanderComms, parse it, and handle it. Depending on the contents we may invoke
    //       the transitionTo() function of another state from within this function.
}

static RoverStateController__Status RoverStateKeepAlive__timerTick(RoverState__TimerTickFuncArgs* args)
{
    /* TODO: convert the code below (from existing main.c and ground_cmd.c) and change it so that it's all based
             on parameters we pass in, rather than accessing globals.

    ##########################################################################################################

    adc_sample();

    ///////////////////////////
    // Below here is the common and KEEPALIVE specific code from send_earth_heartbeat()
    ///////////////////////////

    static uint8_t counter = 0;
    uint8_t send_buf[32];

    // build the packet
    send_buf[0] = 0xFF;

    ////  Flight-spec heartbeats
    if ((counter % 3) != 2) {
        // only send every 3 timer ticks (15s)
        counter++;
        return;
    }
    counter = 0;

    send_buf[1] = (uint8_t)(i2cReadings->batt_charge_telem << 1);
    // send heater on status
    send_buf[1] |= heaterStatus & 0x1;
    // battery current
    send_buf[2] = (uint8_t)(i2cReadings->batt_curr_telem << 1);
    // send voltage nominal status (1=good, 0=too low)
    // check if batt voltage is above 16.59 V (~10% above discharge cutoff)
    send_buf[2] |= (i2cReadings->raw_battery_voltage[0] > 0x3B); // check if batt voltage is above 16.59 V (~10% above discharge cutoff)

    // send the thermistor temperature (12 bits to 8 bits)
    send_buf[3] = (uint8_t)(adc_values[ADC_TEMP_IDX] >> 4);

    // send the packet!
    ipudp_send_packet(send_buf, 4);

    ///////////////////////////
    // End of code from send_earth_heartbeat()
    ///////////////////////////

    if (heatingControlEnabled) heaterControl(); // calculate PWM duty cycle (if any) to apply to heater

    ##########################################################################################################
     */
}

static RoverStateController__Status RoverStateKeepAlive__highTemp(RoverState__HighTempFuncArgs* args)
{
    if (NULL == args) {
        return RS_CONTROLLER__STATUS__ERROR_NULL;
    }

    disableHeater();

    return RS_CONTROLLER__STATUS__SUCCESS;
}