/*
 * Main.cpp
 *
 *  Created on: Sep 30, 2019
 *      Author: cedric
 */

#include <CubeRoverConfig.hpp>
#include "FreeRTOS.h"
#include "os_task.h"
#include "CubeRover/Top/Topology.hpp"

#include "gio.h"
#include "i2c.h"

extern "C" {
    void vApplicationIdleHook(void);
}

void vApplicationIdleHook(void) {
    run1cycle();
    gioSetBit(gioPORTB, 1, 1);
    // for(uint32_t i=0; i<22000; i++) asm("  NOP");
}

void main(void)
{
/* USER CODE BEGIN (3) */
    gioInit();

    gioSetDirection(gioPORTB, ((uint32)1U << 1U));      // gioB[1] OUTPUT -> Idle task

    i2cInit();

    constructApp();

    vTaskStartScheduler();

    //if it reaches that point, there is a problem with RTOS.

/* USER CODE END */
}
