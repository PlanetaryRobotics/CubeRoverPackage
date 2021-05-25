# Watchdog

This branch contains code to run on the MSP430FR5994 acting as a watchdog in the CubeRover.

Documents that may be useful when reading/developing this code:

* [MSP430FR5994 user guide](https://www.ti.com/lit/ug/slau367o/slau367o.pdf)
* [MSP430FR5994 datasheet](https://www.ti.com/lit/ds/symlink/msp430fr5994.pdf)
* SBC schematics
* Various behaviour specification documents (likely located in the GDrive)

## Development notes

### Clocks

TODO: Check which clocks are disabled in LPM
TODO: Consult clock section in the user guide

* MCLK is 16MHz and is the clock that drives the CPU
* SMCLK is ??? MHz (most probably 8 MHz) and is a fractional portion of MCLK.
  It ticks every "n" MCLK ticks and is usually used to drive peripherals.
* DCO is ??? MHz (16MHz? 8MHz?); the Digital Crystal Oscillator. 

