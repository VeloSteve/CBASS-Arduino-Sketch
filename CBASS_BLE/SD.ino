
void SDinit()
{
  if (!SD.begin(SD_CS)) {
    fatalError(F("SD initialization failed!"));
    while (1);
  }
  Serial.println("SD initialization done.");
}

/**
   Read the content of a settings file.  Lines can be comments, ramp points, or key-value pairs as follows:

  // A comment - must start at the beginning of a line
  3:00 30 31.1 32.123 21
  START 14:30
  INTERP LINEAR|STEP

   START, if provided, causes ramp times to be interpreted as relative to that time.  For example
  START 15:00
  0:00 30 30 30 30
  3:00 30 30 33 33
   Means to have all tanks at 30 degrees at 3 PM and ramp tanks 3 and 4 to 33 degrees at 6 PM.  Times
   not falling within the range specified in ramp steps will have a target temperature equal to the last step.
   If INTERP is LINEAR temperatures will be interpolated between the two times.  If STEP, temperatures
   will stay at 30 until 6 PM and then attempt to jump to 33 degrees.  STEP is only for backward
   compatibility.  LINEAR is the default as of 25 Apr 2022.
*/
void readRampPlan() {
  Serial.println("Reading ramp plan");
  rampSteps = 0;
  // State machine:
  const byte ID = 0;  // Identifying what type of line this is.
  const byte KEY = 1;    // Keyword such as START or INTERP
  const byte TEMP = 2;  // Reading temperature as an integer or decimal (not scientific notation)
  const byte START = 4; // Reading start time
  const byte INTERP = 5; // Read interpolation style.  For now just LINEAR or STEP, setting a boolean.
#ifdef USEBLE
  const byte PIN = 6;  // Read the PIN for commands received over BLE
  const byte NAME = 8; // Read the advertised BLE name for this CBASS
#endif
  const byte NEXT = 7;  // Going to the start of the next line, ignoring anything before.
  byte state = 0;
  byte tempCount = 0;
  File f = SD.open("Settings.ini", FILE_READ);
  if (SD.exists("Settings.ini")) {
    Serial.println("The ramp plan exists.");
  } else {
    fatalError(F("---ERROR--- No ramp plan file!"));
  }
  char c;
  while (f.available()) {
    c = f.read();
    switch (state) {
      case ID:
        // Valid characters are a letter (of a keyword), digit of a number, slash, or whitespace
        if (c == '/') {
          state = NEXT;
        }
        else if (isDigit(c)) {
          // Time at the start of a line must be the beginning of a ramp point (typically 5 values)
          // Check that there is space to store a new step.
          if (rampSteps >= MAX_RAMP_STEPS) {
            fatalError(F("Not enough ramp steps allowed!  Increase MAX_RAMP_STEPS in Settings.ini"));
            return;
          }
          rampMinutes[rampSteps] = timeInMinutes(c, f);
          state = TEMP;
          tempCount = 0;
        }
        else if (isSpace(c))  {
          state = ID;  // ignore space or tab, keep going
        }
        else if (isAlpha(c)) {
          state = KEY;
          fillBuffer(c, f);
          if (strcmp(iniBuffer, "START") == 0) {
            state = START;
          } else if (strcmp(iniBuffer, "INTERP") == 0) {
            state = INTERP;
#ifdef USEBLE
          } else if (strcmp(iniBuffer, "BLEPIN") == 0) {
            state = PIN;
          } else if (strcmp(iniBuffer, "BLENAME") == 0) {
            state = NAME;
#endif
          } else {
            Serial.print("---ERROR invalid keyword in settings--->"); Serial.print(iniBuffer); Serial.println("<");
            fatalError(F("Invalid keyword in Settings.ini"));
          }
        }
        else {
          Serial.print("---ERROR in settings.txt.  Invalid line with ");
          Serial.print(c);
          Serial.println("---");
        }
        break;

      case START:
        // Get a start time or fail.
        if (isSpace(c)) break;  // keep looking
        if (isDigit(c)) {
          relativeStartTime = timeInMinutes(c, f);
          relativeStart = true;
          state = NEXT;
          Serial.print("Relative start time = "); Serial.println(relativeStartTime);
        } else {
          Serial.print("---ERROR invalid character in start time >"); Serial.print(c); Serial.println("<");
          fatalError(F("Bad time value in Settings.ini"));
        }
        break;
      case INTERP:
        // Value should be LINEAR or STEP.  Maybe CUBIC in the future.
        if (isSpace(c) || c == ',') break;  // keep looking
        if (isAlpha(c)) {
          fillBuffer(c, f);
          if (strcmp(iniBuffer, "LINEAR") == 0) {
            interpolateT = true;
            state = NEXT;
          } else if (strcmp(iniBuffer, "STEP") == 0) {
            interpolateT = false;  // The default
            state = NEXT;
          } else {
            Serial.print("---ERROR invalid value in INTERP settings--->"); Serial.print(iniBuffer); Serial.println("<");
            fatalError(F("Bad INTERP parameter"));
          }

        } else {
          Serial.print("---ERROR in settings.txt--- INTERP type needed here. Got "); Serial.println(c);
          fatalError(F("Missing INTERP parameter"));

        }
        break;
#ifdef USEBLE
      case PIN:
        // Value should be text between 4 and 15 characters.  Almost anything except whitespace is acceptable.
        if (isSpace(c)) break;  // keep looking
        // From here, assume that any characters are acceptable - unless we find exceptions.
        fillBuffer(c, f, true, false);
        if (strlen(iniBuffer) < 4 || strlen(iniBuffer) > MAXPIN) {
          // If "OFF" or zero length consider PIN-required commands intentionally disabled.  Otherwise, flag an error.
          if (strlen(iniBuffer) > 0 && strcmp(iniBuffer, "OFF") != 0)
            Serial.print(F("ERROR: BLEPIN outside length bounds."));
          strcpy(BLEPIN, "OFF");
        } else {
          strcpy(BLEPIN, iniBuffer);
        }
        // For security, do not print the PIN.
        Serial.println("Read new PIN from Setup.ini.");
        state = NEXT;
        break;

      case NAME:
        // Value should be text 1 and 30 characters.
        // Unline the PIN, here we accept spaces, so read to end of line.
        if (isSpace(c)) break;  // keep looking
        // From here, assume that any characters are acceptable - unless we find exceptions.
        fillBuffer(c, f, false, true);
        if (strlen(iniBuffer) < 1 || strlen(iniBuffer) > MAXPIN * 2) {
          Serial.print(F("ERROR: BLENAME outside length bounds."));
          strcpy(BLENAME, "unnamed CBASS");
        } else {
          strcpy(BLENAME, iniBuffer);
        }
        Serial.print("Read device name "); Serial.println(BLENAME);
        state = NEXT;
        break;
#endif // USEBLE              
      case TEMP:
        // Get a temperature or fail.  Repeat until we have exactly NT values.
        if (isSpace(c) || c == ',') break;  // keep looking
        if (isDigit(c)) {
          // We are on the first digit of a temperature.  tempInHundredths reads it.
          rampHundredths[tempCount++][rampSteps] = tempInHundredths(c, f);
          // There can be many lines, taking more than 8 seconds to parse (at least with diagnostics on) so:
          wdt_reset();
        } else {
          Serial.print("---ERROR in settings.txt--- Temperature needed here. Got c = "); Serial.println(c);
          fatalError(F("Incomplete ramp line."));
        }
        if (tempCount == NT) {
          tempCount = 0;
          rampSteps++;
          if (rampSteps > MAX_RAMP_STEPS) {
            fatalError(F("NOT enough ramp steps allowed!  Increase MAX_RAMP_STEPS in Settings.ini"));
            return;
          }
          state = NEXT;
        }
        break;
      case NEXT:
        // Consume everything until any linefeed or characters have been removed, ready for  new line.
        if (c == 10 || c == 13) { // LF or CR
          // In case we have CR and LF consume the second one.
          c = f.peek();
          if (c == 10 || c == 13) {
            c = f.read();
          }
          state = ID;
        }
        break;
      default:
        Serial.println("---ERROR IN SETTINGS LOGIC---");
        fatalError(F("Bad settings logic"));

    }


  }
  // It is okay to end in the NEXT or ID state, but otherwise we are mid-line and it's an error.
  if (state != NEXT && state != ID) {
    Serial.print("---ERROR out of settings date with step incomplete.  state = "); Serial.println(state);
    fatalError(F("Bad settings line"));
  }

  f.close();
  printRampPlan();
}

// If no third argument, expect a keyword, not a pin.
void fillBuffer(byte c, File f) {
  fillBuffer(c, f, false, false);
}
/**
   We are expecting a keyword, PIN, or BLE name
   If pin, accept anything except whitespace.
   Spaces are allowed in BLE names, so in that case read to end of line.
   If !pin and !spaceOK it's a keyword so only accept letters.
   Fill the global buffer with any acceptable characters.
   The buffer starts with c, which has already been read.
   Leave the file ready to read from the first unwanted character read, so use "peek".
   isWhitespace: space or tab
   isSpace: also true for things like linefeeds
*/
void fillBuffer(byte c, File f, boolean pin, boolean spaceOK) {
  byte pos;
  iniBuffer[0] = c;
  pos = 1;
  while (f.available()) {
    c = f.peek();
    if ((!pin && !spaceOK && isAlpha(c)) || (pin && !isSpace(c)) || (spaceOK && (!isSpace(c) || isWhitespace(c)))) {
      iniBuffer[pos++] = c;
      c = f.read(); // consume the character
      if (pos >= BUFMAX) fatalError(F("Settings.ini keyword too long for BUFMAX."));
    } else {
      iniBuffer[pos] = 0;  // null terminate
      return;
    }
  }
}

/**
   We expect a time like 1:23 in hours and minutes, with optional leading zeros and
   possibly on the hour.
   Return it in minutes, which may be interpreted either relative to a base time
   or relative to midnight, depending on context.
   Leave the file ready to read from the first unwanted character read, so use "peek".
*/
unsigned long timeInMinutes(byte c, File f) {
  byte num;
  unsigned int mins = 0;
  // Read hours, which are mandatory.
  num = c - '0';
  while (f.available()) {
    c = f.peek();
    if (isDigit(c)) {
      num = num * 10 + c - '0';
      c = f.read();
    } else {
      // Must be at end of hour.
      mins = num * 60;
      break;
    }
  }
  // See if there are minutes
  if (f.peek() != ':') return mins;
  f.read();  // consume the colon.
  num = 0;
  while (f.available()) {
    c = f.peek();
    if (isDigit(c)) {
      num = num * 10 + c - '0';
      c = f.read();
    } else {
      // Must be at end of minute.
      mins = mins + num;
      return mins;
    }
  }
  return mins;  // We should get here only if there was a colon at the end of the file.
}


/**
   We expect a temperature like 1.23 in degrees C, either an integer or with decimal places.
   Leave the file ready to read from the first unwanted character read, so use "peek".

   We return hundredths of a degree, so +-327 degrees is supported by a 2-byte int.
*/
int tempInHundredths(byte c, File f) {
  float divisor;
  byte num;
  float tValue = 0;
  // Read integer degrees, which are mandatory.
  num = c - '0';
  while (f.available()) {
    c = f.peek();
    if (isDigit(c)) {
      num = num * 10 + c - '0';
      c = f.read();
    } else {
      // Must be at end of integer part.
      tValue = (float)num;
      break;
    }
  }
  // See if there are decimals
  if (f.peek() != '.') return round(tValue * 100);
  f.read();  // consume the period.
  num = 0;
  divisor = 10;
  while (f.available()) {
    c = f.peek();
    if (isDigit(c)) {
      num =  + c - '0';
      tValue = tValue + (double)(c - '0') / divisor;
      divisor = divisor * 10;
      c = f.read();
    } else {
      // Must be at end of decimal places.
      return round(tValue * 100);
    }
  }
  return round(tValue * 100);  // We should get here only if there was a period at the end of the file.
}

void printRampPlan() {
  printBoth("Temperature Ramp Plan");
  printlnBoth();
  if (relativeStart) {
    printBoth("Settings will be applied relative to start time ");
    printAsHM(relativeStartTime);
    printlnBoth();
  }
  printBoth("Time    Tank 1  Tank 2  Tank3  Tank 4"); printlnBoth();
  for (short k = 0; k < rampSteps; k++) {
    printAsHM(rampMinutes[k]);
    for (int j = 0; j < NT; j++) {
      printBoth("   "); printBoth(((double)(rampHundredths[j][k]) / 100.0), 2);
    }
    printlnBoth();
  }
}

// Print minutes has hh:mm, e.g. 605 becomes 10:05
void printAsHM(unsigned int t) {
  if (t < 10 * 60) printBoth("0");
  printBoth((int)floor(t / 60)); printBoth(":");
  if (t % 60 < 10) printBoth("0");
  printBoth(t % 60);
}


#ifdef COLDWATER
//This block of code is only for systems with lights controlled by relays as specified in LightRelay[]
/**
   Read the content of LightPln.ini  Lines can be comments or an on or off time.
   All tanks switch together, so there is only one on or off per line.
   It is assumed that we want the last state specified, wrapping to the previous day if needed.  In the 
   example below, if the system starts at 6 AM, lights should be off.

  // A comment - must start at the beginning of a line
  7:00 1
  19:00 0
  // The lines above mean on at 7 AM, off at 7 PM.
*/
void readLightPlan() {
  Serial.println("Reading light plan");
  lightSteps = 0;
  // State machine:
  const byte ID = 0;  // Identifying what type of line this is.
  const byte SWITCH = 2;  // Read on or off as 1 or 0.
  const byte NEXT = 3;  // Going to the start of the next line, ignoring anything before.
  byte state = 0;
  File f = SD.open("LightPln.ini", FILE_READ);
  if (SD.exists("LightPln.ini")) {
    Serial.println("The light plan exists.");
  } else {
    fatalError(F("---ERROR--- No light plan file!"));
  }
  char c;
  while (f.available()) {
    c = f.read();
    switch (state) {
      case ID:
        // Valid characters are a digit of a number, slash, or whitespace
        if (c == '/') {
          state = NEXT;
        }
        else if (isDigit(c)) {
          // Time at the start of a line must be the beginning of a switch point 
          // Check that there is space to store a new step.
          if (lightSteps >= MAX_LIGHT_STEPS) {
            fatalError(F("Not enough light steps allowed!  Increase MAX_LIGHT_STEPS in Settings.ini"));
            return;
          }
          lightMinutes[lightSteps] = timeInMinutes(c, f);
          state = SWITCH;
        }
        else if (isSpace(c))  {
          state = ID;  // ignore space or tab, keep going
        }
        else {
          Serial.print("---ERROR in settings.txt.  Invalid line with ");
          Serial.print(c);
          Serial.println("---");
        }
        break;             
      case SWITCH:
        // Get an on off state or fail.
        if (isSpace(c) || c == ',') break;  // keep looking
        if (isDigit(c) && (c == '0' || c == '1')) {
          lightStatus[lightSteps] = (c == '0') ? 0 : 1;
          wdt_reset();
        } else {
          Serial.print("---ERROR in settings.txt--- Light switch state needed here. Got c = "); Serial.println(c);
          fatalError(F("Incomplete light line."));
        }
        lightSteps++;
        state = NEXT;
        break;
      case NEXT:
        // Consume everything until any linefeed or characters have been removed, ready for  new line.
        if (c == 10 || c == 13) { // LF or CR
          // In case we have CR and LF consume the second one.
          c = f.peek();
          if (c == 10 || c == 13) {
            c = f.read();
          }
          state = ID;
        }
        break;
      default:
        Serial.println("---ERROR IN LIGHT SETTINGS LOGIC---");
        fatalError(F("Bad light settings logic"));

    }

  }
  // It is okay to end in the NEXT or ID state, but otherwise we are mid-line and it's an error.
  if (state != NEXT && state != ID) {
    Serial.print("---ERROR out of light data with step incomplete.  state = "); Serial.println(state);
    fatalError(F("Bad light settings line"));
  }

  f.close();
  printLightPlan();
}

void printLightPlan() {
  printBoth("Lighting Plan");
  printlnBoth();
  printBoth("Settings are based on time of day.");
  printlnBoth();
  printBoth("Time    Light State"); printlnBoth();
  for (short k = 0; k < lightSteps; k++) {
    printAsHM(lightMinutes[k]);
    printBoth("   "); printBoth(lightStatus[k]);
    printlnBoth();
  }
}

#endif // COLDWATER






/**
    Save just time and temperature for easier parsing when graphing temperatures on the fly.

    Several ways of saving time are possible.  Milliseconds since boot is fast and easy, but graphs
    would break across reboots.  The RTClib functions work with time since 1 Jan 2000, but they are NOT
    aware of time zones.  There is no advantage in converting to human-friendly units at this point,
    we we'll save seconds since 2000.  This can be used directly in RTClib functions if we graph
    on CBASS, and converted to standard time units when on the app.
    Save the seconds in 10 digits.  This allows times up to about the year 2317 without changing
    the number of bytes.  An unsigned long will hold up to 4,294,967,295 is good until about 2136,
    which should be plenty.  Actually, the secondstime function uses a long, limiting the current
    version to "only" 2068.
    As an example, as this is written the timestamp is 708085103.

    Eight temperatures are saved. First come the latest measured values for the 4 tanks and then
    their setpoints.  Only tenths of degrees are saved, compared to the main log file which has
    hundredths.

*/
#ifdef USEBLE
void saveGraphTemps() {
  logFile = SD.open("GRAPHPTS.TXT", FILE_WRITE);
  // Trust time "t" to be close enough, since it was set just before saving new
  // temperatures and setpoints.

  // Serial.print("secondsTime = "); Serial.print(t.secondstime()); Serial.println(" time since 2000");
  if (t.secondstime() < 1000000000) logFile.print("0");  // true until about 2031
  logFile.print(t.secondstime()); logFile.print(",");

  Serial.print("Saving temps at "); Serial.print(t.hour()); Serial.print(":"); Serial.print(t.minute()); Serial.print(":"); Serial.println(t.second());
  byte i;
  for (i = 0; i < 4; i++) {
    if (TempInput[i] < 10.0) logFile.print("0");  // In case of single-digit temps.  We assume >= 0.
    logFile.print(TempInput[i], 1); logFile.print(",");
  }
  for (i = 0; i < 4; i++) {
    if (SetPoint[i] < 10.0) logFile.print("0");  // In case of single-digit temps.  We assume >= 0.
    logFile.print(SetPoint[i], 1);  if (i < 3) logFile.print(",");
  }
  logFile.println();
  logFile.close();
}
#endif // USEBLE

/*
   Read from tryHere to a newline, into the provided buffer.
   This is only used for getting data for BLE operations, so
   other wise don't compile it.
   This will change if we start to do onboard graphing, which
   will require having this and moving logBuf and bufLen to
   always exist.
*/
#ifdef USEBLE
int readHere(File f, char* buf, long tryHere) {
  f.seek(tryHere);
  int n = f.readBytesUntil('\n', buf, bufLen - 1);
  buf[max(0, n)] = '\0';
  return n;
}
#endif // USEBLE

/**
   If called from setup(), delete the plotting file on restart for easier testing.  Note that
   we do not want this during "real" runs, in case of accidental reboots.
   This can also be called on demand, if implemented.
*/
void clearTemps() {
  SD.remove("GRAPHPTS.TXT");
}


/**
   The following printBoth functions allow printing identical output to the serial monitor and log file
   without having to put both types of print everywhere in the code.  It also moves checking for an
   available log file here.

   Note that there is only one println version, so the line feed calls need to be separate from data.
*/
void printBoth(const char *str)
{
  if (logFile) logFile.print(str);
  Serial.print(str);
}
void printBoth(unsigned int d)
{
  if (logFile) logFile.print(d);
  Serial.print(d);
}
void printBoth(int d)
{
  if (logFile) logFile.print(d);
  Serial.print(d);
}
void printBoth(byte d)
{
  if (logFile) logFile.print(d);
  Serial.print(d);
}
void printBoth(unsigned long d)
{
  if (logFile) logFile.print(d);
  Serial.print(d);
}
void printBoth(long d)
{
  if (logFile) logFile.print(d);
  Serial.print(d);
}
void printBoth(double d)
{
  if (logFile) logFile.print(d);
  Serial.print(d);
}
void printBoth(double d, int places)
{
  if (logFile) logFile.print(d, places);
  Serial.print(d, places);
}
void printBoth(String str)
{
  if (logFile) logFile.print(str);
  Serial.print(str);
}
void printBoth(char c)
{
  if (logFile) logFile.print(c);
  Serial.print(c);
}
// Format time as decimal (for example)
void printBoth(uint8_t n, int f) {
  if (logFile) logFile.print(n, f);
  Serial.print(n, f);
}
void printlnBoth()
{
  if (logFile) logFile.println();
  Serial.println();
}
