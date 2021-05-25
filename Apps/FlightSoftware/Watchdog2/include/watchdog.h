/**
 * @file watchdog.h
 * @brief File header containing function definitions for watchdog
 */
#ifndef __WATCHDOG_INC
#define __WATCHDOG_INC

int watchdog_init();
int watchdog_monitor();
void watchdog_handle_hercules();
void heaterControl();

#endif /* __WATCHDOG_INC */


