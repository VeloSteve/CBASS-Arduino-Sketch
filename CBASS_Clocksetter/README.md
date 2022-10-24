# CBASS_Clocksetter
This is a modified version of example clock setting code found online.
It automatically sets the Arduino's time to the time when the program was compiled.  There is no need to enter a time.  Accuracy should be within a couple of seconds.  The program adds 8 seconds to the time from the host computer, to allow for upload time.  Change that value if you find the set time is consistently fast or slow, as the delay may depend on the speed of your connection to the Arduino.

This should be run once from a computer with the correct time in the time zone you are using.  Follow up by loading some other software before powering down or resetting.  If this runs again it will reset to the time when it installed, which is now wrong.

This can be repeated any time the clock has lost accuracy or when changing time zones.
