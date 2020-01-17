##############################################################################
# Set resolution and units for timing file:
set_time_format -unit ns -decimal_places 3

##############################################################################
# Create a clock called "yourClockNameHere" (can be whatever you want)
# set its period in nanoseconds, set the waveform as { rise fall },
# and set which port in the design it is.
# The example below describes a 50MHz clock with 50% duty cycle 
# (rise at 0ns, then fall at 10ns) driving the 'clk' top level port
create_clock -name Cy10_Clock -period "50 MHz" [get_ports { sysClk }]

# You can make more than one clock if needed.

##############################################################################
# Now that we have created the custom clocks which will be base clocks,
# derive_pll_clock is used to calculate all remaining clocks for PLLs (if any)
# and clock uncertainties.
derive_pll_clocks -create_base_clocks
derive_clock_uncertainty