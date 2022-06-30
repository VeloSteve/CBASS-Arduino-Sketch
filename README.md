# CBASS-Arduino-Sketch
 
This software is based on a version by Dan Barshis and others at Old Dominion University.  They deserve full credit for creation of the core temperature monitoring and control features.  I have modified the code heavily enough that any bugs are probably my fault. 

The one dramatic change is the addition of the ability to connect to CBASS via Bluetooth Low Energy to monitor temperatures and make limited changes to the experimental plan.  A companion app is in an early state of development.

If you want to use this code you will need a CBASS-R "shield", an Arduino MEGA, and [Adafruit's Bluefruit LE SPI friend](https://www.adafruit.com/product/2633) and you will need to build these into a full system.  Pop "CBASS coral research" into your favorite search engine to see what's going on.  If you are serious, get in touch.

NOTE: this sketch depends on a number of specific libraries, some of which are out of date.  An issue will be filled to consider updating.  Currently you will need to install

(sorry - testing table preview before completing it)

| Name | Version used | URL from properties |
| Adafruit_ILI934 | 1.5.8 | https://github.com/adafruit/Adafruit_ILI9341 |
| SD | 1.2.2 | http://www.arduino.cc/en/Reference/SD |
- SPI (where)
- PID
- OneWire
- DallasTemperature
- RTClib
- avr/wdt.h
- Adafruit_BLE
- Adafrut_BluefruitLE_SPI
