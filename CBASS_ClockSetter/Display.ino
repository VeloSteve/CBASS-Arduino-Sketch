#if DISP == TFT
  #define TFT_WIDTH 320
  #define TFT_HEIGHT 240
  #define TFT_LAND 3  // Rotation for normal text  1 = SPI pins at top, 3 = SPI at bottom.
  #define TFT_PORT 2  // Rotation for y axis label, typically 1 less than TFT_LAND
  #define LINEHEIGHT 19 // Pixel height of size 2 text is 14.  Add 1 or more for legibility.
  #define BLACK   ILI9341_BLACK
  #define WHITE   ILI9341_WHITE
  
  void startTFTDisplay() {
      disp.begin();
      disp.setRotation(TFT_LAND); 
      disp.fillScreen(BLACK);
      // A start point for text size and color.  If changed elsewhere, change it back.
      disp.setTextSize(2);  // A reasonable size. 1-3 (maybe more) are available.
      disp.setTextColor(WHITE);
  }
#endif

/**
 * Start the display type expected at compile time.  It would be ideal to check for success and
 * give a warning if needed, but these functions return void.
 */
void startAnyDisplay() {
  #if DISP == LCD
    disp.begin(16, 2);              // Start the library.  Returns void
    disp.setCursor(0, 0);
    disp.print("ClockSetter LCD");
  #elif DISP == TFT
    startTFTDisplay();

    // The TFT display is big enough to put this message at the bottom and still use
    // top lines for other things.
    disp.setTextSize(1);
    disp.setCursor(0, TFT_HEIGHT - 2*LINEHEIGHT);
    disp.print("Now install the CBASS sketch.");
    disp.setCursor(0, TFT_HEIGHT - LINEHEIGHT);
    disp.print("Do not restart ClockSetter.");
    disp.setTextSize(2);

    disp.setCursor(0, 0);
    disp.print("ClockSetter TFT LCD");
  #elif DISP == NODISP
    startTFTDisplay();
    Serial.println("No supported display is expected on the device.");
    return;
  #endif
  Serial.println("If the intalled display matches compilation settings, you should see messages there.");
}

/**
 * This simply prints the given text on one line on the display.  The complexity
 * comes from the fact that different displays have different sizes, behaviors, and command names.
 */
void showText(const char *t) {
  // Always print to the serial monitor.
  Serial.println(t);
  #if DISP == NODISP
    return;
  #elif DISP == LCD
    disp.setCursor(0, 1);
    disp.print(t);
  #elif DISP == TFT
    // Determine the current display line and move to the start of the next, wrapping to the top
    // if needed.  This crudely assumes there are no headers or other special positions.
    int16_t yNow = disp.getCursorY();
    if (TFT_HEIGHT - yNow - LINEHEIGHT > 0) {
      disp.setCursor(0, yNow + LINEHEIGHT);
    } else {
      disp.setCursor(0, 0);
    }
    disp.print(t);
  #endif
}


/** Print the time to the display.  For now this assumes that successive prints with no cursor change
 *  pick up from the most recent x location, but always at the same y (no automatic line feed).
 *  Place the time on a fixed text line, regardless of what else is on the screen.  For LCD use the 2nd of 2 lines for time and the first
 *  for the date.
 *  For TFT use the 4th line for time and the 3rd for date.
 *  These are printed in reverse order because prompt time updates take priority over the date.
 */
void showTime(const DateTime now) {
  #if DISP == NODISP
    return;
  #elif DISP == LCD
    disp.setCursor(0, 1);
    disp.print("          ");
    disp.setCursor(0, 1);
  #elif DISP == TFT    
    disp.fillRect(0, 3*LINEHEIGHT, TFT_WIDTH, LINEHEIGHT, BLACK);
    disp.setCursor(0, 3*LINEHEIGHT); 
  #endif

  // Time
  disp.print(now.hour());
  disp.print(':');
  if (now.minute() < 10) disp.print("0");
  disp.print(now.minute());
  disp.print(':');
  if (now.second() < 10) disp.print("0");
  disp.print(now.second());
  disp.print("   ");

  // Date
  #if DISP == LCD
    disp.setCursor(0, 0);
  #elif DISP == TFT
    disp.setCursor(0, 2*LINEHEIGHT);
    disp.print("                ");
    disp.setCursor(0, 2*LINEHEIGHT);
  #endif
  disp.print(daysOfTheWeek[now.dayOfTheWeek()]);
  disp.print(", ");
  disp.print(now.day());
  disp.print('/');
  disp.print(now.month());
  disp.print('/');
  disp.print(now.year());
}
