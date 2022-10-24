# CBASS-Arduino-Sketch
 
This software is based on a version by Dan Barshis and others at Old Dominion University.  They deserve full credit for creation of the core temperature monitoring and control features.  I have modified the code heavily enough that any bugs are probably my fault. 

The one dramatic change is the addition of the ability to connect to CBASS via Bluetooth Low Energy to monitor temperatures and make limited changes to the experimental plan.  A companion app is in an early state of development.

If you want to use this code you will need a CBASS-R "shield", an Arduino MEGA, and [Adafruit's Bluefruit LE SPI friend](https://www.adafruit.com/product/2633) and you will need to build these into a full system.  Pop "CBASS coral research" into your favorite search engine to see what's going on.  If you are serious, get in touch.

NOTE: this sketch depends on a number of specific libraries, some of which are out of date.  Newer versions may work, but have not been tested.  An issue will be filed to consider updating.  Currently you will need to install

| Name | Version used | URL from properties |
| ---|---|---|
| Adafruit_ILI934 | 1.5.8 | https://github.com/adafruit/Adafruit_ILI9341 |
| SD | 1.2.2 | http://www.arduino.cc/en/Reference/SD |
| PID | 1.2.0 | http://playground.arduino.cc/Code/PIDLibrary |
| OneWire | 2.3.5 | http://www.pjrc.com/teensy/td_libs_OneWire.html |
| DallasTemperature | 3.9.0 | https://github.com/milesburton/Arduino-Temperature-Control-Library |
| RTClib | 1.2.1 | https://github.com/adafruit/RTClib |
| Adafruit_BLE | 1.10.0 | https://github.com/adafruit/Adafruit_BluefruitLE_nRF51 |
| Adafrut_BluefruitLE_SPI | 1.10.0 | https://github.com/adafruit/Adafruit_BluefruitLE_nRF51 |

These libraries are not explictly included in the sketch, but must be present for compilation.  They may be included when you download the other libraries.

| Name | Version used | URL from properties |
| ---|---|---|
| Adafruit_BusIO | 1.7.3 | https://github.com/adafruit/Adafruit_BusIO |
| Adafruit_GFX_Library | 1.10.10 | https://github.com/adafruit/Adafruit-GFX-Library

This is opposite to those above.  It must be explicitly included, but does not need to be installed by you.  On my machine it is in
<where software gets installed>\Arduino\hardware\tools\avr\avr\include\avr
| Name | Version used? | URL from properties |
| ---|---|---|
| avr/wdt.h | arduino.avrdude=6.3.0-arduino17 <br/> arduino.arduinoOTA=1.3.0 <br/> arduino.avr-gcc=7.3.0-atmel3.6.1-arduino | n/a |

