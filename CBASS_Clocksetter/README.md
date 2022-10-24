# CBASS_Clocksetter

This is used to set the CBASS clock to the current time of day.  It is based on examples found online, but modified to used the displays included with CBASS in addition to the serial monitor.

To use this compile and install it on CBASS, watching the display or Serial monitor to verify that the process was successful.  There is no need to enter a time, as it uses the compilation time from your computer.  8 seconds is added to allow for data transfer, and you may change this if your setup is working faster or more slowly than that.

This should be run once from a computer with the correct time in the time zone you are using.  Follow up by loading the main CBASS sketch or some other software before powering down or resetting.  If this runs again it will reset to the time when it first installed, which is now wrong.

This can be repeated any time the clock has lost accuracy or when changing time zones.
