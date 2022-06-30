// If BLE is not enabled at compile time this entire file can be skipped:
#ifdef USEBLE
// Length of one log line GRAPHPTS.LOG
const byte bytesPerTempLine = 52; // 5 digits for day, 4 for minute, 8 4-char floats, 9 commas, 2 for CRLF (0x0d 0x0a)
const byte bufLen = bytesPerTempLine + 5;
char logBuf[bufLen];

char *token;  // Used globally for parsing ble inputs.
/**
   Set the state of the BLE (Bluetooth Low Energy) module and do some basic checks.
*/
byte bleSetup() {
  // ===== BLE setup stuff =====
  if ( !ble.begin(VERBOSE_MODE) )
  {
    // Problem: begin returns true even if there is no BLE module!
    nonfatalError(F("Couldn't find Bluefruit, make sure it's in Command mode."));
    return false;
  }
  Serial.println( F("OK!") );
  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("BLE factory reset: "));
    if ( ! ble.factoryReset() ) {
      ble.info();
      nonfatalError(F("Couldn't factory reset.  Is firmware at 0.8.1?"));
      return false;
    }
  }
  /* Disable command echo from Bluefruit */
  ble.echo(false);

  // Set a custom advertised name
  char nameSet[2*MAXPIN+1+14] = "AT+GAPDEVNAME=";
  strcat(nameSet, BLENAME);
  //if (! ble.sendCommandCheckOK(F("AT+GAPDEVNAME=MyLab CBASS-R 1")) ) {
  if (! ble.sendCommandCheckOK(nameSet) ) {
    nonfatalError(F("Could not set device name?"));
    return false;
  }

  // We don't currently use the LED, but setting a value serves
  // as a test for updated firmware.  To see blinking use "MODE" instead of "DISABLE".
  if (! ble.sendCommandCheckOK(F("AT+HWMODELED=DISABLE")) ) {
    nonfatalError(F("Could not set BLE LED mode.  Is firmware updated?"));
    return false;
  }

  Serial.println(F("Bluefruit info:"));
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use the CBASS app to connect."));
  Serial.println();
  ble.verbose(false);  // debug info is a little annoying after this point!
  return true;
}

// Check for and respond to BLE inputs with actions, data, or both.
// This is only called if there is already a connection.
void checkBLEInput() {
  unsigned long timeCheck = millis();
  // Check for incoming characters from Bluefruit
  ble.println(F("AT+BLEUARTRX"));
  ble.readline();
  if (strcmp(ble.buffer, "OK") == 0) {
    // no data
    // In a tight loop we would add a delay here, but with CBASS this is
    // only called about once per second anyway.  This exit occurs after 3 to 4 ms.
    return;
  }
  ble.waitForOK();
  // Some data was found, it is in the buffer
  if (DEBUGBLE) {
    Serial.println(); Serial.print(F("[Recv] ")); Serial.println(ble.buffer);
  }
  handleRequest();
  timeCheck = millis() - timeCheck;
  if (timeCheck > 1000) {
    Serial.print(F("BLE request took ")); Serial.print(timeCheck);
    Serial.println(F(" ms.  Check for impact on temperature control."));
  } else if (DEBUGBLE) {
    Serial.print(F("BLE instruction handled in ")); Serial.print(timeCheck); Serial.println(F(" ms."));
  }
}

/**
   Start a transmission over BLE.  Adafruit's documentation says the maximum
   payload is 240 characters.
*/
void tx() {
  ble.print(F("AT+BLEUARTTX="));
}

/**
   Each set of prints must end with exactly one println and a waitForOK.
   Encapsulate that here for consistency.  Don't use ble.println anywhere else.
*/
void endtx() {
  ble.println();
  // check response status
  if (! ble.waitForOK() ) {
    Serial.println(F("Failed to send?"));
  }
}

/**
   handleRequest is where BLE inputs are interpreted.  The simplest requests are handled inline
   while others call functions to carry out the requested action.

   For simple parsing and minimal data transfers, each request is identified by a single letter, a-z.
   This allows 26 distinct commands, which should be more than enough.  Each identifier may be followed
   by arguments specific to that command.  This will again be very simple, with no labels, just a list of
   values.

   Run-modifying requests will require some sort of PIN or password, to be defined, so bluetooth users
   in the vicinity cannot harm the run.

   Note that when BLE is enabled we will keep a distinct temperature log file.  This serves two purposes.
   First, by containing less detail than the main log file it will be faster to parse and send.  Second,
   if any BLE-related code causes a problem with the file, experimental data in the main log will not be lost.

   Output-only requests:
   b - return a batch of requested log lines
   d - return a simple debug response with no other logic.
   r - return the range of times logged in the graphing file, number of bytes, and expected bytes per line
   s - return the time ramps start, or NA.
   t - return current time of day, 4 temperatures, and 4 targets. To stay under than 20-byte BLE limit
       this is done in parts, with argument
       1 for time, 2 for current temperature, and 3 for targets.
   l - return the entire temperature log (tbd, possibly subject to a limit based on acceptable time used)
   p - return the current schedule (tbd)

   Run-modifying requests:
   C - set the CBASS Clock to the time received
   N - set am entire New schedule based on the supplied parameters (tbd)
   S - set the Start time for the ramp, or return a warning if start time is not in use.
*/
void handleRequest() {
  // Note: don't do NVM read stuff here - it could wipe the buffer.
  // First, just acknowledge.
  if (DEBUGBLE) Serial.println(F("In BLE handleRequest"));

  // LESSONS LEARNED:
  // 1) More than one println without a matching AT_BLEUARTTX can mess up later functions!
  //    Multiple print() calls are acceptable.
  // 2) From reading the library code, it appears that input of more than 16*4 characters in a line will be truncated.

  // Act on the message.
  // Each "if" with any printing to ble MUST have exactly one AT command, one println (after any prints) and one waitForOK!
  // This is now satisfied by starting with tx() and ending with endtx(), bracketing the switch statement.  Most cases exist
  // to print() something over BLE, but even the others should probably print something.
  // Functions called within the switch structure must NOT include ble.println() unless the corresponding tx() and endtx() calls
  // are included.
  // Note that character 48 is decimal integer 0.

  char c = ble.buffer[0];  // The request identifier
 
  tx();
  switch (c) {
    case 't':
      bleCurrentTemps(); break;
    case 'b':
      // Special case: this function will typically use some tx()/endtx() pairs because there may be many lines
      // of data.  The function must know to the skip the first and last calls, since they appear outside this switch.
      bleLogBatch();
      break;
    case 'C':
      if (pinOK()) setCBASSClock(); break;
    case 's':
      bleStartTime();
      break;
    case 'r':
      sendTLogStats();
      break;
    case 'S':
      if (pinOK()) setStartTime();
      break;
    case 'd':
      if (DEBUGBLE) Serial.println(F("Hello BLE!"));
      ble.print(F("Hello BLE!\\n"));  break;
    default:
      if (DEBUGBLE) Serial.println(F("Unrecognized BLE code."));
      ble.print(c);
      ble.print(F(" invalid command\\n"));
  }
  endtx();
}

/**
   Current status suitable for display.  Time, temperatures, and targets.  Sending
   all 9 values at once exceeds the 20-byte limit for normal transfers, so expect an
   argument.
*/
void bleCurrentTemps() {
  char c = ble.buffer[1];
  if (c != ',') {
    ble.print(F("Missing comma-separated argument to t."));
    return;
  }
  c = ble.buffer[2]; // the argument
  switch (c) {
    case '1':
      t = rtc.now();
      ble.print(t.year()); ble.print("/"); ble.print(t.month()); ble.print("/"); ble.print(t.day()); ble.print(" ");
      ble.print(t.hour()); ble.print(":");
      if (t.minute() < 10) ble.print("0"); ble.print(t.minute()); ble.print(":");
      if (t.second() < 10) ble.print("0"); ble.print(t.second());
      break;
    case '2':
      for (byte i = 0; i < 4; i++) {
        ble.print(TempInput[i], 1);
        if (i < 3) ble.print(",");
      }
      break;
    case '3':
      for (byte i = 0; i < 4; i++) {
        ble.print(SetPoint[i], 1);
        if (i < 3) ble.print(",");
      }
      break;
    default:
      Serial.println("Sending t argument warning.");
      ble.print(F("Only digits 1-3 are valid for t"));
  }
}

/**
   Check that the second argument after the command type is a valid PIN.
   This must be called and must return true before any command which
   modified CBASS operations.
*/
boolean pinOK() {
  if (strlen(BLEPIN) < 4 || strlen(BLEPIN) > 15) {
    ble.print(F("CBASS commands are disabled."));
    return false;
  }
  // Use strtok to walk through the expected arguments in place
  token = strtok(ble.buffer, ",");
  // Command ID
  if (token == NULL) {
    Serial.print(F("Problem with command >")); Serial.print(token); Serial.println("<");
    ble.print(F("Must have argments."));
    return false;
  }
  // The PIN must be here (the second token)
  token = strtok(NULL, ",");
  if (token == NULL || strcmp(token, BLEPIN)) {
    Serial.print(F("Bad PIN >")); Serial.print(token); Serial.println("<");

    ble.print(F("Bad PIN."));
    return false;
  }
  // Note that the buffer now has a null terminator before and after the PIN.
  // Don't expect ble.buffer to be unchanged in subsequent functions.
  return true;
}

/**
   Set the system clock from a BLE command.  To discourage tampering a PIN is required.  This
   should be done with great caution.
*/
void setCBASSClock() {
  char dateString[15];  // expect Jan 12, 2023, for example.  Allow extra.
  // Use strtok to walk through the expected arguments in place
  // token should become whatever is after the PIN (date in this case)
  token = strtok(NULL, ",");
  if (token == NULL) {
    ble.print(F("Date required."));
    return;
  }
  strcpy(dateString, token);
  token = strtok(NULL, ",");
  if (token == NULL) {
    ble.print(F("Date,time required."));
    return;
  }
  Serial.print(F("Old time: "));
  Serial.print(t.hour(), DEC); Serial.print(","),
               Serial.print(t.minute(), DEC); Serial.print(","); Serial.println(t.second(), DEC);

  Serial.print(F("Setting date with ")); Serial.println(dateString);
  Serial.print(F("Setting time with ")); Serial.println(token);
  rtc.adjust(DateTime(dateString, token));

  //ble.print("");
  ble.print(getdate());
  ble.print(" ");
  ble.print(t.hour()); ble.print(":");
  ble.print(t.minute()); ble.print(":");
  ble.print(t.second());

  if (DEBUGBLE) {
    Serial.print(F("New time: "));
    Serial.print(t.hour(), DEC); Serial.print(","),
                 Serial.print(t.minute(), DEC); Serial.print(","); Serial.println(t.second(), DEC);
  }
}

/**
   Show the time ramps are set to start, or NA if not using relative ramps.
   NOTE that the time is sent as stored, in minutes from midnight.
*/
void bleStartTime() {
  if (relativeStart) {
    ble.print(relativeStartTime);
  } else {
    ble.print(F("NA"));
  }
}

/**
   Receive a start time in minutes from midnight, and save it if no
   using static ramps.
   Echo the time of accepted, otherwise return Using static ramps.
*/
void setStartTime() {
  int st;
  if (!relativeStart) {
    ble.print(F("Using static ramps."));
    return;
  }
  // Get the next thing after the pin.
  token = strtok(NULL, ",");
  if (token == NULL) {
    ble.print(F("S requires an integer"));
    Serial.println(F("S missing arguments."));
    return;
  }
  // Assume that the rest of the buffer is  single integer.
  st = atoi(token);   // & to send the address of character 2
  if (st >= 24 * 60) {
    ble.print(F("S minutes out of range."));
    Serial.print(F("New start time out of range: ")); Serial.println(st);
  } else {
    ble.print(F("Start "));
    ble.print(st / 60); ble.print(":");
    if (st % 60 < 10) ble.print("0"); ble.print(st % 60);
    relativeStartTime = st;
    Serial.print(F("S response is Start ")); Serial.print(st / 60); Serial.print(":");
    if (st % 60 < 10) Serial.print("0"); Serial.println(st % 60);
  }
  return;
}

/*
   Send some status useful for debugging, but also important for efficient graph data
   requests from the app.  Note that methods like this which return > 20 bytes must
   have an endpoint indicator, in this case "D".
*/
void sendTLogStats() {
  const int maxChars = bytesPerTempLine + 10;  // Extra buffer size to be safe.
  char buf[maxChars];
  long tryHere = 0;
  Serial.println(F("Sending stats"));
  File f = SD.open("GRAPHPTS.TXT", FILE_READ);

  // Early in a run we may not have two lines, and things will break.
  if (f.size() < 2 * bytesPerTempLine) {
    ble.print(F("Not enough data for stats.,D"));
    return;
  }

  readHere(f, buf, tryHere);
  if (DEBUGBLE) {
    Serial.print("DEBUG: buf of first line is >"); Serial.print(buf); Serial.println("<");
  }
  // Minus 2 because bytesPerTempLine includes  CR LF, which is not in the buffer.
  if (strlen(buf) < bytesPerTempLine - 2) {
    ble.print("D");
    if (DEBUGBLE) {
      Serial.print("Graph logs stats found short data line of ");
      Serial.print(strlen(buf));
      Serial.println(" bytes.");
    }
    return;
  }
  // Return first item, which specifies the first time.
  token = strtok(buf, ",");
  ble.print(atol(token));
  ble.print(",");

  tryHere = f.size() - bytesPerTempLine - 10; // Start before a line break and call readHere twice.
  int got = readHere(f, buf, tryHere);
  tryHere = tryHere + got + 1;
  readHere(f, buf, tryHere);
  // Now append the end time.
  token = strtok(buf, ",");
  ble.print(atol(token));
  ble.print(",");
  // The file size
  ble.print(f.size());
  ble.print(",");
  // and the assumed bytes per line.
  ble.print(bytesPerTempLine);
  ble.print(",D");
  return;
}

/*
   This receives a command like b,-10,708085103 and interprets it as
   "send 10 log lines starting backward from the first timestamp less than 708085103 seconds since 1/1/2000"
   These (or fewer if they don't all exist) are sent followed by BatchDone
   as an end marker.  Essentially all responses will exceed the 20-byte
   BLE buffer size, so the multiple transmissions will be made automatically,
   but the receiving app must collect and parse them back into log lines.

   Each line starts with "T", just to allow one more check on the receiving end.

   One oddity here is that the sends are bracketed by tx() endtx() BUT the
   first send doesn't need a tx() and the last one (normally BatchDone) doesn't need endtx().
*/
void bleLogBatch() {
  if (DEBUGBLE) {
    Serial.print(F("Free memory start: ")); Serial.println(freeMemory());
  }

  // Check time, since long sends could affect temperature control.
  unsigned long batchTic = millis();

  ///// Understand the request /////
  char c = ble.buffer[1];
  if (c != ',') {
    ble.print(F("Missing comma-separated arguments to command b."));
    return;
  }
  char* ptr = strtok(ble.buffer, ",");  // This is just the "b".  Move on.
  ptr = strtok(NULL, ",");  // strtok knows where it left off last time!
  int lines = atoi(ptr);
  ptr = strtok(NULL, ",");
  long start = atol(ptr); // In seconds since 2000.
  boolean forward = true;
  if (lines < 0) {
    lines = -lines;
    forward = false;
  }
  if (DEBUGBLE) {
    Serial.print(F("Ready to find and send ")); Serial.print(lines);
    Serial.print((forward) ? F(" lines forward from ") : F(" lines backward from  "));
    Serial.println(start);
  }
  long lineTime;

  ///// Find the start point /////

  // The key to this is finding the start line without reading more lines than necessary and
  // with minimal memory footprint.  Since the file may contain very old timestamps if not cleared
  // between runs, we will work from the newest line.  In concept:
  // - get latest timestamp
  // - calculate how far back in the file to go to get the target
  // - if < 0 just start from line 1.
  // - Walk forward or back until the target is reached or exceeded
  // - send that line and repeat until the request is satisfied or there is no data
  // - end with BatchDone
  //
  // Details
  //  - missing on the initial location calculation is normal, since log lines may not be
  //    exactly one per minute
  //  - inputs may include extreme values as a shortcut, so handle values far in the future
  //    or past correctly
  //  - after this is working, consider a smarter start guess saved in a static variable, for when
  //    sequential graphing request are made.
  //
  int got; // Used for number of bytes in last read.
  long tryHere = 0;  // File location in bytes
  int count = 0;  // Log lines sent

  File f = SD.open("GRAPHPTS.TXT", FILE_READ);
  if (!f || !f.size()) {
    // File is missing or empty.  Nothing to return.
    Serial.println(F(" returning - no graph file or no data."));
    ble.print("BatchDone");
    f.close();
    return;
  }

  tryHere = max(0, f.size() - bytesPerTempLine - 5);

  if (!tryHere) {
    // Read first line.
    got = readHere(f, logBuf, tryHere);
  } else {
    // Read to a linefeed, discard, and read the next full line.
    got = readHere(f, logBuf, tryHere);
    tryHere = tryHere + got + 1;  // Skip read characters and the linefeed (or is it CRLF?)
    got = readHere(f, logBuf, tryHere);
  }
  // Is this a full line?  (note removal of CRLF)
  if (strlen(logBuf) < bytesPerTempLine - 2) {
    Serial.print("Bad last line, < 50 bytes. >"); Serial.print(logBuf); Serial.println("<");
  }

  lineTime = getLineTime(logBuf);

  // TODO return BatchDone if lineTime is zero (meaning there was no valid time)

  // Remember that lineTime is the last line in the file (which may also be the first line).
  // Find the first line, but check in case we are already there.
  if (start != lineTime) tryHere = walkToFirst(f, tryHere, start, forward, lineTime);
  if (DEBUGBLE) {
    Serial.print(F(" Walk returns "));
    Serial.println(tryHere);
  }
  if (tryHere < 0) {
    // No eligible lines to return.
    ble.print(F("BatchDone"));
    f.close();
    return;
  }

  ///// If we get here there should be one or more lines to return. /////
  // Performance notes.
  // 1) With copious debug prints this can take more than 3 seconds per 10 lines,
  //    leading to wdt reboots.
  // 2) reads take about 30 ms, endtx/tx 51 ms, and the ble print about 72 ms.
  // 3) Try without the endtx/tx pair, since the app can collect across multiple sends.
  //    This runs about 5 times faster, as even the prints speed up to 51 ms.
  ble.setMode(BLUEFRUIT_MODE_DATA);

  wdt_reset();
  count = 0;
  // XXX int gotBytes = 0;
  boolean outOfData = false;

  while (count < lines && !outOfData) {
    // The constant here determines how often an endtx/tx pair is called during
    // long sends.  At one time the SD reads failed for values above 30 or so.
    // Try again: 100 ok.
    // 200 leads to a reboot at 115 lines.  This is just due to time, not BLE issues.
    // With a wdt_reset every 50 we get to 300 lines with no problem.
    // Remove the echo to see if it works when running faster.
    if (count % 50 == 0) {
      // Make each group of lines a separate transmission.  This could be useful
      // if (for example) a BLE send buffer is filling, but at the moment it seems
      // unnecessary.  Or was it?
      if (count > 0 && count % 200 == 0) {
        endtx();
        tx();
      }

      wdt_reset();
      if (DEBUGBLE) {
        Serial.print(F("    Elapsed ")); Serial.print(millis() - batchTic); Serial.print(F(" millis.  Lines = "));
        Serial.println(count);
      }
    }

    got = readHere(f, logBuf, tryHere);

    if (got < bytesPerTempLine - 2) {
      Serial.print(F("ERROR: giving up.  Read short temp line: >")); Serial.print(logBuf); Serial.print(F("< at c = ")); Serial.println(count);
      f.close();
      ble.print(F("BatchDone"));
      ble.setMode(BLUEFRUIT_MODE_COMMAND);
      return;
    }

    ble.print("T");
    ble.print(logBuf);

    count++;
    tryHere = tryHere + (bytesPerTempLine) * ((forward) ? 1 : -1);
    if (tryHere < 0 || tryHere >= (long)f.size()) {
      // Past one end of the file, signal done.
      outOfData = true;
    }
  }
  f.close();


  //if (count > 0) { endtx(); tx(); } // Make each line a separate transmission.
  ble.print(F("BatchDone"));
  ble.setMode(BLUEFRUIT_MODE_COMMAND);

  wdt_reset();
}

/**
    Return the location in bytes of the first line to return, or -1 to indicate
    that there is nothing to return.
    Arguments are:
    f - the log file
    pos - the starting position, always at the beginning of the last line
    start - the desired first time
    forward - true if returned lines should be toward the future
    lineTime - the time stamp of the last line in the file
*/
long walkToFirst(File f, long pos, long start, boolean forward, long lineTime) {

  long lastLinePos = pos;  // Save the start point, always at the last log line.
  if (DEBUGBLE) {
    Serial.print(F("Walking with file size ")); Serial.print(f.size()); Serial.print(F(" and original pos "));
    Serial.print(pos); Serial.print(F(" with time ")); Serial.println(lineTime);
  }

  // Catch cases where there is no walking required.
  if (forward) {
    if (start > lineTime) {
      // Request past last time.
      return -1;
    }
  } else {
    if (pos == 0 && start < lineTime) {
      // Request before first time.
      return -1;
    } else if (start > lineTime) {
      // All lines in the file are before the start time, so use the current position.
      return pos;
    }
  }

  // Estimate location of the desired line, assuming 1 log line per GRAPHwindow milliseconds.
  // Note that line times are in seconds but logging is typically 1 line per minute.
  pos = max(0, pos - (long)((float)((lineTime - start) / 60) * ((float)60000 / GRAPHwindow) * bytesPerTempLine));
  if (pos > (long)f.size()) {
    nonfatalError(F("Check data types.  Calculated start past end of file."));
    // Just start from the last line.
    pos = (long)f.size() - bytesPerTempLine;
  }
  if (DEBUGBLE) {
    Serial.print(F("Estimated walk start position = "));
    Serial.print(pos); Serial.print(F(" based on lastLinePos ")); Serial.print(lastLinePos);
    Serial.print(F(" start ")); Serial.print(start);
    Serial.print(F(" and lineTime " )); Serial.println(lineTime);
  }
  if (forward) {
    // FORWARD RETURN

    // We want to return results forward in time, so we want to provide the earliest
    // location on or just after "start".  First work back to a value <=, then
    // forward to the first one >=.
    // BACK CHECK
    boolean done = false;
    while (!done) {
      if (DEBUGBLE) Serial.print("B");
      readHere(f, logBuf, pos);
      lineTime = getLineTime(logBuf);
      if (lineTime == start) return pos;
      else if (lineTime < start || pos == 0) done = true;
      else pos -= bytesPerTempLine;
    }
    // FORWARD CHECK
    done = false;
    while (!done) {
      if (DEBUGBLE) Serial.print("F");
      readHere(f, logBuf, pos);
      lineTime = getLineTime(logBuf);
      if (lineTime == start) return pos;
      else if (lineTime > start) done = true;
      else pos = pos + bytesPerTempLine;
    }
    if (start > lineTime) {
      // Request past last time.
      Serial.print(F(" No valid lines. Start = ")); Serial.print(start); Serial.print(F(" after line time of "));
      Serial.println(lineTime);
      return -1;
    }
  } else {
    // BACKWARD RETURN


    // We want to return results backward in time, so we want to provide the latest
    // location on or just before "start".  First work forward to a value >=, then
    // backward to the first one <=.
    // FORWARD CHECK
    boolean done = false;
    while (!done) {
      if (DEBUGBLE) Serial.print("f");
      readHere(f, logBuf, pos);
      lineTime = getLineTime(logBuf);
      if (lineTime == start) return pos;
      else if (lineTime > start || pos == lastLinePos) done = true;
      else pos += bytesPerTempLine;
    }
    // BACKWARD CHECK
    done = false;
    while (!done) {
      if (DEBUGBLE) Serial.print("b");
      readHere(f, logBuf, pos);
      lineTime = getLineTime(logBuf);
      if (lineTime == start) return pos;
      else if (lineTime < start || pos == 0) done = true;
      else pos -= bytesPerTempLine;
    }
    if (start < lineTime) {
      // Request before first time.
      if (DEBUGBLE) {
        Serial.print(F(" No valid lines. Start = ")); Serial.print(start); Serial.print(F(" before time of "));
        Serial.println(lineTime);
      }
      return -1;
    }
  }
  if (DEBUGBLE) { Serial.print(F("\n Walked to byte ")); Serial.println(pos); }
  return pos;
}

/*
   Return the time of day for a temp log line in seconds since 2000.
   NOTE that this uses strtok, so the buffer is modified.
*/
long getLineTime(char* buf) {
  char* ptr = strtok(buf, ",");
  if (ptr == NULL) return 0;
  return atol(ptr);
}

// May be removed after diagnostics.
// A println of freeMemory() can help find memory leaks.
extern unsigned int __bss_end;
extern void *__brkval;
int freeMemory() {
  int free_memory;
  if ((int) __brkval)
    return ((int) &free_memory) - ((int) __brkval);
  return ((int) &free_memory) - ((int) &__bss_end);
}
#endif // USEBLE  - this brackets the entire BLE.ino file
