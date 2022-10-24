// It is useful to have the sketch confirm success or failure.  This should always be visible on the Serial monitor, but
// also try to have it output on the most-used display types.
// Supported displays (do not edit without changing affected code):
#define LCD 1    // The original 2-line monochrome LCD
#define TFT 2    // TFT LCD, typically 320 x 240 color pixels.
#define NODISP 3

// The supported display to compile for at this time (edit as needed):
#define DISP TFT

#include <RTClib.h>
#if DISP == LCD
#include <Adafruit_RGBLCDShield.h>
  Adafruit_RGBLCDShield disp = Adafruit_RGBLCDShield();
#elif DISP == TFT
  // Library for the Adafruit TFT LCD Display
  #include <Adafruit_ILI9341.h>
  #define TFT_CS   7
  #define TFT_DC   6
  Adafruit_ILI9341 disp = Adafruit_ILI9341(TFT_CS, TFT_DC);  // The CS and DC pins.
#elif DISP == NODISP
#endif

RTC_DS1307 rtc;  // This works with the DS3231 as well.

char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int uploadLag = 8; // About how long it takes to upload the sketch and actually set the time, in seconds.

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("ClockSetter");
  // If there is a physical display, start it and print a greeting there as well.
  startAnyDisplay();

  if (!rtc.begin()) {
    // This code may not be what I thought.  It returns true even if there is
    // no RTC!  The isrunning call will catch that case.
    showText("Couldn't start RTC");
    while (1);
  }
  if (!rtc.isrunning()) {
    showText("Failed to communicate with RTC.  Is one connected?");
    while (1);
  }
  showText("Found RTC"); // New 2/8/19
  // following line sets the RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0, 0, 0, uploadLag));
  Serial.print("Set RTC to ");
  Serial.print(F(__DATE__)); Serial.print("  ");Serial.println(F(__TIME__));
  Serial.println("Install another sketch now.");
  Serial.println("Do not reset the Arduino with ClockSetter installed, or the time will be wrong.");
}

DateTime now, oldNow;
void loop() {
  now = rtc.now();
  // Reduce flickering by only displaying when the seconds digit has changed.  Note that
  // DateTime only has seconds.
  if (now.second() != oldNow.second()) showTime(now);
  oldNow = now;
  delay(50);  // Check again after 50 ms.  Overkill, but too long a delay looks choppy.
}
