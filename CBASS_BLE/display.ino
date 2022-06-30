

void startDisplay() {
    tft.begin();
    tft.setRotation(TFT_LAND); 
    tft.fillScreen(BLACK);
    // A start point for text size and color.  If changed elsewhere, change it back.
    tft.setTextSize(2);  // A reasonable size. 1-3 (maybe more) are available.
    tft.setTextColor(WHITE);
}

void displayTemperatureStatus() {
    tft.fillScreen(BLACK);

    for (i=0; i<NT; i++) {
      tft.setCursor(0, i*LINEHEIGHT);
      tft.print("T"); tft.print(i+1); tft.print("SP ");
      dtostrf(SetPoint[i], 4, 1, SetPointStr);
      tft.print(SetPointStr);
      tft.print(" T"); tft.print(i+1); tft.print(" ");
      dtostrf(TempInput[i], 4, 1, TempInputStr);
      tft.print(TempInputStr);
      box(RelayStateStr[i], i, LINEHEIGHT);
    }
}

/**
 * Put a message on the display and Serial monitor, and then
 * wait long enough to trigger a reboot.  Note that the argument
 * must be something like F("My message").
 */
void fatalError(const __FlashStringHelper *msg) {
  Serial.print(F("Fatal error: "));
  Serial.println(msg);
  tft.fillScreen(BLACK);
  tft.setTextSize(3);
  tft.setTextColor(RED);
  tft.setCursor(0, 0);

  tft.println(F("Fatal error: "));
  tft.print(msg);
  // This is long enough to trigger the watchdog timer, causing a reboot.
  // It prevents the run from starting and allows for a retry in case (for
  // example) an SD card is inserted late.
  delay(10000);
}

void nonfatalError(const __FlashStringHelper *msg) {
  Serial.print(F("Nonfatal error: "));
  Serial.println(msg);
  tft.fillScreen(BLACK);
  tft.setTextSize(3);
  tft.setTextColor(RED);
  tft.setCursor(0, 0);

  tft.println(F("Nonfatal error: "));
  tft.println("Run will resume without this capability.");
  tft.print(msg);
  // In contrast to fatalError, wait 10 seconds without triggering
  // the watchdog timer.
  for (i=0; i<5; i++) {
    delay(2000);
    wdt_reset();
  }
  // Revert to default color.
  tft.setTextColor(WHITE);
}

void displayTemperatureStatusBold() {
    tft.setTextSize(2);
    // Only clear below the heading for less flashing.  Also don't clear the boxes, which are refreshed anyway.
    //tft.fillRect(LINEHEIGHT*2, LINEHEIGHT3, TFT_WIDTH-LINEHEIGHT3*4, TFT_HEIGHT-LINEHEIGHT3, BLACK);
    //Header
    tft.setCursor(0,0);
    tft.print("     SETPT  INTEMP   RELAY");

    tft.setTextSize(3);
    for (i=0; i<NT; i++) {
      tft.fillRect(LINEHEIGHT*2, LINEHEIGHT3*(i+1), TFT_WIDTH-LINEHEIGHT3*4, LINEHEIGHT3, BLACK);
      tft.setCursor(0, LINEHEIGHT3*(i+1));
      tft.print("T"); tft.print(i+1); tft.print(" ");
      dtostrf(SetPoint[i], 4, 1, SetPointStr);
      tft.print(SetPointStr);
      tft.print(" ");
      dtostrf(TempInput[i], 4, 1, TempInputStr);
      tft.print(TempInputStr);
      box(RelayStateStr[i], i+1, LINEHEIGHT3);
    }

    // To add diagnostics after the temperature lines:
    // tft.fillRect(LINEHEIGHT*2, LINEHEIGHT3*5, TFT_WIDTH-LINEHEIGHT3*4, LINEHEIGHT3, BLACK);
    // tft.setCursor(0, LINEHEIGHT3*5);
    // tft.print("Loop ms "); tft.print(someGlobalVariable);
}

/*
// This version eliminates screen flicker by drawing to a buffer (canvas) and drawing that
// to the screen.  Unfortunately, it increased update time from 183 ms to 1566 ms!  That's 
// too slow, but I'm keeping the code temporarily, hoping to come up with a faster buffer.
// If enabled, a canvas variable will be needed globally or as a static here:
//GFXcanvas1 canvas(TFT_WIDTH-LINEHEIGHT3*2, LINEHEIGHT3); // For blink-free line updates on the screen.
void displayTemperatureStatusBold() {
    tft.setTextSize(2);
    // Only clear below the heading for less flashing.  Also don't clear the boxes, which are refreshed anyway.
    //tft.fillRect(LINEHEIGHT*2, LINEHEIGHT3, TFT_WIDTH-LINEHEIGHT3*4, TFT_HEIGHT-LINEHEIGHT3, BLACK);
    //Header
    tft.setCursor(0,0);
    tft.print("     SETPT  INTEMP   RELAY");

    tft.setTextSize(3);
    canvas.setTextSize(3); // Drawing to a canvas first prevents display flashing.
    for (i=0; i<NT; i++) {
      canvas.fillScreen(BLACK);
      canvas.setCursor(0, 0);
      canvas.print("T"); canvas.print(i+1); canvas.print(" ");
      dtostrf(SetPoint[i], 4, 1, SetPointStr[i]);
      canvas.print(SetPointStr[i]);
      canvas.print(" ");
      dtostrf(TempInput[i], 4, 1, TempInputStr);
      canvas.print(TempInputStr);
      // The canvas is sized to overwrite the text in the old line, but not the relay box at the end.
      tft.drawBitmap(0, LINEHEIGHT3 * (i+1), canvas.getBuffer(), TFT_WIDTH-LINEHEIGHT3*2, LINEHEIGHT3, WHITE, BLACK);
      box(RelayStateStr[i], i+1, LINEHEIGHT3);
    }
}
*/

// Draw a box with color based on relay state.
// Place on the zero-based line specified.
void box(char* s, int line, int lineSize) {
  word color;
  if (strcmp(s, "OFF") == 0) {
    color = BLACK;
  } else if (strcmp(s, "HTR") == 0) {
    color = RED;
  } else if (strcmp(s, "CHL") == 0) {
    color = BLUE;
  } else {
    color = YELLOW; // Yellow = caution.  This should not happen.
  }
  // x, y, width, height, color
  tft.fillRect(TFT_WIDTH-1.5*lineSize, lineSize*line, lineSize-1, lineSize-1, color);
}
