#ifndef INCLUDE_CUBEROVERCONFIG_HPP_
#define INCLUDE_CUBEROVERCONFIG_HPP_

// Definition of rate group dividers
#define RATEGROUP_DIVIDER_LOW_FREQ  220000000   // 1Hz
#define RATEGROUP_DIVIDER_MED_FREQ  2200000     // 100Hz
#define RATEGROUP_DIVIDER_HI_FREQ   220000      // 1kHz

// Definition of component instance ID, Affinity and queue depth
#define EVENT_LOGGER_ID             0
#define EVENT_LOGGER_AFF            1
#define EVENT_LOGGER_QUEUE_DEPTH    1

#define TLM_CHAN_ID                0
#define TLM_CHAN_AFF               31
#define TLM_CHAN_QUEUE_DEPTH       1

#define PARAM_DB_ID                0
#define PARAM_DB_AFF               0
#define PARAM_DB_QUEUE_DEPTH       1

#define CMD_DISP_ID                0
#define CMD_DISP_AFF               2
#define CMD_DISP_QUEUE_DEPTH       1

#define BLOCK_DRV_ID                0
#define BLOCK_DRV_AFF               0
#define BLOCK_DRV_QUEUE_DEPTH       1

#define RG_LOW_FREQ_ID              0
#define RG_LOW_FREQ_AFF             30
#define RG_LOW_FREQ_QUEUE_DEPTH     1

#define RG_MED_FREQ_ID              1
#define RG_MED_FREQ_AFF             29
#define RG_MED_FREQ_QUEUE_DEPTH     1

#define RG_HI_FREQ_ID              2
#define RG_HI_FREQ_AFF             28
#define RG_HI_FREQ_QUEUE_DEPTH     1

#define BLK_DRV_ID                  0
#define BLK_DRV_AFF                 32
#define BLK_DRV_QUEUE_DEPTH         1

#define ACTIVE_LOGGER_ID            0
#define ACTIVE_LOGGER_AFF           26
#define ACTIVE_LOGGER_QUEUE_DEPTH   1

#define MOTOR_CONTROL_ID            0
#define MOTOR_CONTROL_AFF           25
#define MOTOR_CONTROL_QUEUE_DEPTH   1

#define RADIO_SERIAL_QUEUE_DEPTH    1
#define RADIO_SERIAL_AFF            24

#define MIN_STACK_SIZE_BYTES        1024

// Configuration of I2C interfaces address and ports
// For the motor controllers
#define FRONT_LEFT_MC_I2C_ADDR   	0x01
#define FRONT_RIGHT_MC_I2C_ADDR  	0x02
#define REAR_LEFT_MC_I2C_ADDR    	0x03
#define REAR_RIGHT_MC_I2C_ADDR   	0x04
#define MOTOR_CONTROL_I2CREG     	i2cREG1

#define CUBEROVER_WHEEL_DIAMETER_CM		20.0f
#define MOTOR_NB_PAIR_POLES				1.0f
#define MOTOR_GEAR_BOX_REDUCTION		5.0f

#endif /* INCLUDE_CUBEROVERCONFIG_HPP_ */
