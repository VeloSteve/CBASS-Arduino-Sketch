#define NT 4  // The number of tanks supported.  This may always be 4.
#define RELAY_PAUSE 5000 // Milliseconds to wait between some startup steps - standard is 5000, but a small number is handy when testing other things.

// The original CBASS from the Barshis lab uses Iceprobe chillers.
// The Logan lab modifications use moving cold water, and add light controls.
#undef COLDWATER  //#define for liquid cooling and lights.  Omit or #undef for the original behavior.

#undef USEBLE   // #define to use BLE, #undef USEBLE to remove all related code
#ifdef USEBLE
#define DEBUGBLE false // true for verbose debugging.  Normally false
#define MAXPIN 15  // Do not change - the app and Arduino code much match.

// A PIN which the app must send to carry out any command which affects CBASS timing or other function.
// Most characters are acceptable, but not spaces or tabs.
// The PIN must be from 4 to 15 characters long.
// To disable this type of command, use "OFF", or anything < 4 or > 15 characters.
// This may be overridden from Settings.ini with a line like 
// BLEPIN 7my!#-pwd4&
char BLEPIN[MAXPIN+1] = "OFF";
// The name you will see when making a Bluetooth LE connection.
// Also limit name size, but to 30 characters.  Ideally, include "CBASS" somewhere
char BLENAME[2*MAXPIN+1] = "My CBASS A"; 
#endif

// ***** Temp Program Inputs *****
double RAMP_START_TEMP[] = {30.5, 30.75, 30.5, 30.75};  // This becomes the temperature target at any time.
const short MAX_RAMP_STEPS = 20; // Could be as low as 7, 24*12+1 allows every 5 minutes for a day, with endpoints.

#ifdef COLDWATER
#define CHILLER_OFFSET 0.0
#else
#define CHILLER_OFFSET 0.20
#endif
const double TANK_TEMP_CORRECTION[] = {0, 0, 0, 0}; // Is a temperature correction for the temp sensor, the program subtracts this from the temp readout e.g. if the sensor reads low, this should be a negative number

// ***** PID TUNING CONSTANTS ****
#define KP 2000//5000//600 //IN FIELD - Chillers had higher lag, so I adjusted the TPCwindow (now deprecated) and KP to 20 secs, kept all proportional
#define KI 10//KP/100//27417.54//240 // March 20 IN FIELD - with 1 deg steps, no momentum to take past P control, so doubled I. (10->40)
#define KD 1000//40  //

#define RELAY_ON 1
#define RELAY_OFF 0

// Arduino pin numbers for relays.
// The integers correspond to the CBASS-R v 1.2 schematic.
const int HeaterRelay[] = {14, 15, 16, 17};
#ifdef COLDWATER
const int ChillRelay[] = {40, 38, 36, 34}; 
const int LightRelay[] = {22, 23, 24, 25}; 
const short MAX_LIGHT_STEPS = 4; // typically we have only 2 for sunrise and sunset
#else
const int ChillRelay[] = {22, 23, 24, 25}; 
#endif

/* 
 *  Old version kept for DB9 pins and colors:
#define T1HeaterRelay 17  // T1 Heat DB9 pin 9 black wire Arduino Digital I/O pin number 17
#define T1ChillRelay  22  // T1 Chill DB9 pin 1 brown wire Arduino Digital I/O pin number 22
#define T2HeaterRelay 15  // T2 Heat DB9 pin 8 white wire Arduino Digital I/O pin number 15
#define T2ChillRelay  24 // T2 Heat DB9 pin 2 red wire Arduino Digital I/O pin number 24
#define T3HeaterRelay 16  // T3 Heat DB9 pin 7 purple wire Arduino Digital I/O pin number 16
#define T3ChillRelay  23  // T3 Chill DB9 pin 3 orange Arduino Digital I/O pin number 23
#define T4HeaterRelay 25 // T4 Heat DB9 pin 6 blue wire Arduino Digital I/O pin number 25
#define T4ChillRelay  14  // T4 Chill DB9 pin 4 yellow wire Arduino Digital I/O pin number 14
 */

// Other Arduino pins
#define SENSOR_PIN 8
#define TFT_CS   7
#define TFT_DC   6
// SD card.  Note that there is a second SD card on the back of the display, which we don't currently use.
// This is for the active one.
#define SD_DETECT  10
#define SD_CS  11

// Display constants
#define TFT_WIDTH 320
#define TFT_HEIGHT 240
#define TFT_LAND 3  // Rotation for normal text  1 = SPI pins at top, 3 = SPI at bottom.
#define TFT_PORT 2  // Rotation for y axis label, typically 1 less than TFT_LAND
#define LINEHEIGHT 19 // Pixel height of size 2 text is 14.  Add 1 or more for legibility.
#define LINEHEIGHT3 28 // Pixel height of size 3 text is ??.  Add 1 or more for legibility.

// Colors are RGB, but 16 bits, not 24, allocated as 5, 6, and 5 bits for the 3 channels.
// For example, a light blue could be 0x1F1FFF in RGB, but 0x1F3A in 16-bit form. To convert
// typical RBG given as a,b,c, use 2048*a*31/255 + 32*b*63/255 + c*31/255
#define DARKGREEN 0x05ED  // Normal green is too pale on white. 
#define LIGHTBLUE 0x1F3A  // Normal blue is too dark on black.
// Easier names for some of the colors in the ILI9341 header file.
#define BLACK   ILI9341_BLACK
#define BLUE    ILI9341_BLUE
#define RED     ILI9341_RED
#define GREEN   ILI9341_GREEN
#define CYAN    ILI9341_CYAN
#define MAGENTA ILI9341_MAGENTA
#define YELLOW  ILI9341_YELLOW
#define WHITE   ILI9341_WHITE

// Keep this since it may be nice in a display, but comment to save memory until we need it.
/*
const byte degree[8] = // define the degree symbol
{
  B00110,
  B01001,
  B01001,
  B00110,
  B00000,
  B00000,
  B00000,
  B00000
};
 */
