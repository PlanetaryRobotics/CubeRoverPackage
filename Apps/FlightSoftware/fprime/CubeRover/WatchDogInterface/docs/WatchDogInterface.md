<title>WatchDogInterface Component Dictionary</title>
# WatchDogInterface Component Dictionary


## Command List

|Mnemonic|ID|Description|Arg Name|Arg Type|Comment
|---|---|---|---|---|---|
|Reset_Specific|0 (0x0)|Command to reset the specific parts of rover| | |
| | | |reset_value|U8|
                    	U8 Value that specifies which components or hardware need to be reset
                    |
|Disengage_From_Lander|1 (0x1)|Command to send signal to MSP430 that it should send a signal to lander to disengage| | |

## Telemetry Channel List

|Channel Name|ID|Type|Description|
|---|---|---|---|
|LAST_COMMAND|0 (0x0)|string|The command last sent to watchdog interface|
|VOLTAGE_2_5V|2 (0x2)|int16_t|Voltage from 2.5V line from Watchdog|
|VOLTAGE_2_8V|3 (0x3)|int16_t|Voltage from 2.8V line from Watchdog|
|VOLTAGE_24V|4 (0x4)|int16_t|Voltage from 24V line from Watchdog|
|VOLTAGE_28V|5 (0x5)|int16_t|Voltage from 28V line from Watchdog|
|BATTERY_THERMISTOR|16 (0x10)|U8|Boolean for Battery Charging or not from Watchdog|
|SYSTEM_STATUS|23 (0x17)|int8_t|Boolean for Heater On/Off from Watchdog|

## Event List

|Event Name|ID|Description|Arg Name|Arg Type|Arg Size|Description
|---|---|---|---|---|---|---|
|WatchDogMSP430IncorrectResp|0 (0x0)|Warning that the WatchDog MSP430 sent back a response different than what was sent to it| | | | |
|WatchDogTimedOut|1 (0x1)|Warning that a WatchDog MSP430 watchdog timer went off| | | | |
|WatchDogCmdReceived|2 (0x2)|Notification that watchdog interface recieved a command from Cmd_Dispatcher| | | | |
| | | |Cmd|Fw::LogStringArg&|50|The cmd that watchdog interface processed|
|WatchDogCommError|3 (0x3)|Warning that a WatchDog MSP430 error has occured.| | | | |
| | | |error|U32||The watchdog error value (reference to documentation)|