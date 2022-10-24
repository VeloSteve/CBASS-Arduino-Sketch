/********************************************************
   CBASS Control Software
   This software uses a PID controller to switch heating and
   cooling devices.  These control the water temperature in
   small aquaria used for thermal tolerance experiments on coral.

   We use "time proportioning
   control"  Tt's essentially a really slow version of PWM.
   First we decide on a window size (5000mS say.) We then
   set the pid to adjust its TempOutput between 0 and that window
   size.  Lastly, we add some logic that translates the PID
   TempOutput into "Relay On Time" with the remainder of the
   window being "Relay Off Time"

   This software, including the other files in the same directory
   are subject to copyright as described in LICENSE.txt in this
   directory.  Please read that file for terms.

   Other imported libraries may be subject to their own terms.
 ********************************************************/

// Library for the Adafruit TFT LCD Display
#include <Adafruit_ILI9341.h>

// SD card shield library
#include <SD.h>
// PID Library
#include <PID_v1.h>
//#include <PID_AutoTune_v0.h>

// Libraries for the DS18B20 Temperature Sensor
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>
#include "settings.h"

// The new display uses SPI communication (not I2C), shared with other components.
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);  // The CS and DC pins.

// Add a watchdog timer so the system restarts if it somehow hangs.  This could prevent a fire in extreme cases!
#include <avr/wdt.h>

// RTC is now DS3231, if there is trouble with older hardware, try RTC_DS1307.
RTC_DS3231  rtc;

DateTime  t;

// A file for logging the data.
File logFile;
String printdate = "Apr2022Development"; // No spaces
// Storage for the characters of keyword while reading Settings.ini.
const byte BUFMAX = 16; // INTERP is the longest keyword for now.  PINs can be 15+null terminator.
char iniBuffer[BUFMAX];

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(SENSOR_PIN);

//Setup temp sensors
DallasTemperature sensors(&oneWire);

DeviceAddress Thermometer[NT];

//Define Variables we'll Need
// Ramp plan
unsigned int rampMinutes[MAX_RAMP_STEPS];  // Time for each ramp step.
int rampHundredths[NT][MAX_RAMP_STEPS];  // Temperatures in 1/100 degree, because is half the storage of a float.
short rampSteps = 0;  // Actual defined steps, <= MAX_RAMP_STEPS
boolean interpolateT = true; // If true, interpolate between ramp points, otherwise step.  
boolean relativeStart = false;  // Start from midnight (default) or a specified time.
unsigned int relativeStartTime;  // Start time in minutes from midnight
#ifdef COLDWATER
// Light plan
unsigned int lightMinutes[MAX_LIGHT_STEPS];
byte lightStatus[MAX_LIGHT_STEPS];
short lightSteps = 0;
short lightPos = 0; // Current light state
char LightStateStr[][4] = {"LLL", "LLL", "LLL", "LLL"}; // [number of entries][characters + null terminator]
#endif // COLDWATER

//Temperature Variables
double tempT[NT];
double SetPoint[NT], TempInput[NT], TempOutput[NT], Correction[NT];
double ChillOffset;
// Time Windows: Update LCD 2/sec; Serial, Ramp Status 1/sec, TPC 1/2 sec
unsigned int LCDwindow = 500, SERIALwindow = 1000, i;
// misc.
int TPCwindow=10000;
unsigned int SerialOutCount = 101;

// Display Conversion Strings
char SetPointStr[5];  // Was an array of [NT][5], but we only need one at a time.
char TempInputStr[5];
char RelayStateStr[][4] = {"OFF", "OFF", "OFF", "OFF"}; // [number of entries][characters + null terminator]

//Specify the links and initial tuning parameters
// Perhaps with Autotune these would need to be variable, but as
// it stands there is no reason to copy the defined constants into a double.
// double kp = KP, ki = KI, kd = KD; //kp=350,ki= 300,kd=50;

// PID Controllers
PID pids[4] = {
  PID(&TempInput[0], &TempOutput[0], &SetPoint[0], KP, KI, KD, DIRECT),
  PID(&TempInput[1], &TempOutput[1], &SetPoint[1], KP, KI, KD, DIRECT),
  PID(&TempInput[2], &TempOutput[2], &SetPoint[2], KP, KI, KD, DIRECT),
  PID(&TempInput[3], &TempOutput[3], &SetPoint[3], KP, KI, KD, DIRECT)
};

// Bluetooth can be used or completely removed based on USEBLE
#ifdef USEBLE
#include "BluefruitConfig.h"
#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>
#define FACTORYRESET_ENABLE         1
const unsigned long bleDelay = (unsigned long)1 * (unsigned long)200; // Units are milliseconds.
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
boolean bleOkay = false;  // This is switched automatically during setup.
unsigned int GRAPHwindow = 60000;
unsigned long GRAPHt;
#endif // USEBLE
 
//TimeKeepers
unsigned long now_ms = millis(),SERIALt, LCDt;

void setup()
{
  // Start "reset if hung" watchdog timer. 8 seconds is the longest available interval.
  wdt_enable(WDTO_8S);
  wdt_reset();  // Call this after any long-running code or delay() call to reset to 8 seconds.

  // ***** INITALIZE OUTPUT *****`
  Serial.begin(38400);         // 9600 traditionally. 38400 saves a bit of time
  startDisplay();             // TFT display
  Serial.println("\n===== Booting CBASS =====");
  SDinit();                   // SD card

  clockInit();  // Keep this before any use of the clock for logging or ramps.

  checkTime();  // Sets global t and we save the start time.
  delay(2000); //Check that all relays are inactive at Reset
  RelaysInit();

  wdt_reset();
  readRampPlan();
  #ifdef COLDWATER
  readLightPlan();
  #endif
  wdt_reset();
  rampOffsets();  // This does not need repeating in the main loop.
  
  // Get the target temperatures as will be done in the loop.
  getCurrentTargets();
  applyTargets();
  ShowRampInfo();

  PIDinit();

  sensorsInit();

  #ifdef USEBLE
  wdt_reset();
  bleOkay = bleSetup();
  #endif // USEBLE

  wdt_reset();

  tft.fillScreen(BLACK);
  tft.setTextSize(3);
  tft.setCursor(0, 0);  // remember that the original LCD counts characters.  This counts pixels.
  tft.print("PrintDate is:");
  tft.setCursor(0, LINEHEIGHT3);
  tft.print(printdate);
  Serial.println("PrintDate is:");
  Serial.println(printdate);
  // Show RTC time so we know it's working.
  tft.setCursor(0, LINEHEIGHT*4);
  tft.print("Time: "); tft.print(gettime());
  delay(3000);
  wdt_reset();
 
  tft.setCursor(0, LINEHEIGHT3*5);
  tft.print(" 5s Pause...");
  Serial.println();
  Serial.println();
  Serial.print("Initialization sequence.");
  Serial.println();
  Serial.print(" 5s Pause...");
  Serial.println();
  delay(RELAY_PAUSE);
  wdt_reset();
  Serial.println();

  relayTest();

  Serial.println();

  // Clear the LCD screen before loop() because we may not fully clear it in the loop.
  tft.fillScreen(BLACK);

  // These timers use cumulative time, so start them at a current time, rather than zero.  
  // Subtract the repeat interval so they activate on the first pass.
  SERIALt = millis() - SERIALwindow;
  LCDt = millis() - LCDwindow;
  #ifdef USEBLE
  GRAPHt = millis() - GRAPHwindow;
  #endif
}

/**
 * This loop checks temperatures and updates the heater and chiller states.  As of 21 Apr 2022 it takes about
 * 1055 ms for the quickest loops.  When timed activities such as getting new temperature targets
 * and printing the new values trigger, this rises to about 1235 ms.
 * On this same date, the default triggers are 60000 ms for temperature target updates, 1000 ms for serial logging,
 * and 500 ms for LCD display updates.  When the latter two are smaller than the actual loop time, they simply
 * run every time.
 * Within each loop most of the time is in 4 areas: 479 ms in requestTemperatures, 193 ms getting and adjusting
 * temperatures, 199 ms logging (to serial monitor and SD card), 183 ms updating the LCD display.
 * On loops when target values are updated, 3 ms is spent doing that work  and 113 ms printing the result.
 * 
 * This loop time is effective for thermal control.  Changes to the code which affect the loop time substatially
 * should be followed by physical testing of temperature histories.
 */
unsigned long timer24h = 0;
void loop()
{
  wdt_reset();
  // ***** Time Keeping *****
  now_ms = millis();

  // ***** INPUT FROM TEMPERATURE SENSORS *****
  sensors.requestTemperatures();

  for (i=0; i<NT; i++) {
    tempT[i] = sensors.getTempCByIndex(i) - Correction[i];
    if (0.0 < tempT[i] && tempT[i] < 80.0)  TempInput[i] = tempT[i];
  }

  // Update temperature targets once per minute.
  if ((now_ms - timer24h) > 60000) {
    checkTime();

    timer24h = now_ms;
    getCurrentTargets();
    applyTargets();
#ifdef COLDWATER
    // Similar to getCurrentTargets/applyTargets, but for lighting.
    getLightState();
#endif    

    ShowRampInfo();

    wdt_reset();
  }

  // Remove the ifdef if we have other reasons to save the graphing file.  It's just a less
  // detailed version of the main log.
  #ifdef USEBLE
  if (now_ms - GRAPHt > GRAPHwindow) {
    checkTime();
    // Save temperatures and targets in a log for graphing, independent of the main science data.
    saveGraphTemps();
    wdt_reset();
    GRAPHt += GRAPHwindow;
  }
  #endif  // USEBLE
  
  // ***** UPDATE PIDs *****
  for (i=0; i<NT; i++) pids[i].Compute();


  //***** UPDATE RELAY STATE for TIME PROPORTIONAL CONTROL *****
  updateRelays();

  //***** UPDATE SERIAL MONITOR AND LOG *****

  if (now_ms - SERIALt > SERIALwindow) {
    SerialReceive();
    SerialSend();
    SERIALt += SERIALwindow;
  }

  //***** UPDATE LCD *****
  if ((now_ms - LCDt) > LCDwindow)
  {
    // Two styles - a matter of taste.  This is also where a version with graphing could be called.
    // In that case a more compact text portion would be desireable.
    //displayTemperatureStatus();
    displayTemperatureStatusBold();
    // No need to delay.  LCDwindow should regulate things.
    LCDt += LCDwindow;
  }


  #ifdef USEBLE
  // With no app connected this is nearly instant.
  // A check with an app connected but no input takes 3 to 4 ms.
  if (bleOkay && ble.isConnected()) {
    wdt_reset();
    checkBLEInput();
  }
  #endif // USEBLE

}

void SerialSend()
{
  logFile = SD.open("LOG.txt", FILE_WRITE);
  if (SerialOutCount > 100) {
    printLogHeader();
    SerialOutCount = 0;
  }
  // General data items not tied to a specific tank:
  printBoth(printdate), printBoth(","), printBoth(getdate()), printBoth(","), printBoth(now_ms), printBoth(","),
  printBoth(t.hour(), DEC), printBoth(","), printBoth(t.minute(), DEC), printBoth(","), printBoth(t.second(), DEC), printBoth(",");
  // Per-tank items
  for (i=0; i<NT; i++) {
    printBoth(SetPoint[i]), printBoth(","),
    printBoth(TempInput[i]), printBoth(","),
    printBoth(tempT[i]), printBoth(","),
    printBoth(TempOutput[i]), printBoth(","),
    printBoth(RelayStateStr[i]), printBoth(",");
  }
  printlnBoth();
  Serial.flush();
  logFile.close();
  SerialOutCount += 1;
}

/**
 * Typically this will be called once from setup() with no logFile open
 * and then periodically during loop().  Having the code here keeps 
 * the two versions in sync.
 * This assumes 4 tanks (NT=4) and 5 data items per tank after the first line.
 */
void printLogHeader() {
    printBoth(F("PrintDate,Date,N_ms,Th,Tm,Ts,"));
    printBoth(F("T1SP,T1inT,TempT1,T1outT,T1RelayState,"));
    printBoth(F("T2SP,T2inT,TempT2,T2outT,T2RelayState,"));
    printBoth(F("T3SP,T3inT,TempT3,T3outT,T3RelayState,"));
    printBoth(F("T4SP,T4inT,TempT4,T4outT,T4RelayState,"));
    printlnBoth();
}

void SerialReceive()
{
  if (Serial.available())
  {
    //char b = Serial.read();  // JSR: Unused b generates a compiler warning.
    Serial.read();
    Serial.flush();
  }
}
