#ifndef __BSP_H__
#define __BSP_H__

#include <msp430.h>
#include <stdint.h>

extern uint8_t heaterStatus;
extern uint8_t hasDeployed;

void initializeGpios();

inline void enableHeater();
inline void disableHeater();
inline void enable3V3PowerRail();
inline void disable3V3PowerRail();
inline void enable24VPowerRail();
inline void disable24VPowerRail();
inline void releaseHerculesReset();
inline void setHerculesReset();
inline void releaseRadioReset();
inline void setRadioReset();
inline void releaseFPGAReset();
inline void setFPGAReset();
inline void fpgaCameraSelectHi();
inline void fpgaCameraSelectLo();
inline void releaseMotorsReset();
inline void releaseMotor1Reset();
inline void releaseMotor2Reset();
inline void releaseMotor3Reset();
inline void releaseMotor4Reset();
inline void setMotorsReset();
inline void setMotor1Reset();
inline void setMotor2Reset();
inline void setMotor3Reset();
inline void setMotor4Reset();
inline void powerOnHercules();
inline void powerOffHercules();
inline void powerOnRadio();
inline void powerOffRadio();
inline void powerOnFpga();
inline void powerOffFpga();
inline void powerOnMotors();
inline void powerOffMotors();
inline void enableBatteries();
inline void disableBatteries();
inline void setDeploy();
inline void unsetDeploy();
inline void startChargingBatteries();
inline void stopChargingBatteries();
#endif // __BSP_H__

