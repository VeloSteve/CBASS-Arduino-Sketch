

void applyTargets() {
  // Copy temperatures - it is not clear why we have both.  It may keep the PID values from 
  // unwanted adjustments while the RAMP_START_TEMP values are being updated and adjusted.
  for (i=0; i<NT; i++) SetPoint[i] = RAMP_START_TEMP[i];
}

void rampOffsets() {
  for (i=0; i<NT; i++) Correction[i] = TANK_TEMP_CORRECTION[i];
  ChillOffset = CHILLER_OFFSET;  //the offset below the set temp when the chiller kicks in
}

/**
 * Get the desired temperatures for the current time.
 */
void getCurrentTargets() {
// Ramp temperatures would naturally be stored in a float (4 bytes), but the requirement of up to 12*24+1 points
// per day uses too much memory.  Use an int (2 bytes), and consider it hundredths of a degree.  This saves
// about 28% of possible global variable storage when the maximum points are used.

  static short rampPos = 0; // latest index in the ramp plan. "static" makes it persist between calls.

  t = rtc.now();  // May be redundant with checkTime() in the main loop.
  unsigned int dayMin = t.minute() + 60 * t.hour();

  //Apply relative time - dayMin can't go negative.
  // Note that 
  if (relativeStart) {
    if (dayMin > relativeStartTime) dayMin -= relativeStartTime;
    else dayMin = (dayMin + 24*60) - relativeStartTime;
  }
  

  //Serial.print(" Relative start = "); Serial.println(relativeStartTime);
  //Serial.print(" Adjusted dayMin = "); Serial.println(dayMin);
  // We can be before the specified points, between two, or after all of them.
  // rampPos is 0 at the start of a run, and the points to the latest position.
  // If the time is less than the current start point, we are either before all
  // points or wrapping past midnight.
  if (rampMinutes[rampPos] > dayMin) rampPos = 0;

  // Move up if necessary.
  while (rampMinutes[rampPos+1] <= dayMin && rampPos < rampSteps - 1) {
    rampPos++;
  }

  // Case 0: Before all points
  if (dayMin < rampMinutes[rampPos]) {
    for (i=0; i<NT; i++) RAMP_START_TEMP[i] = (double)rampHundredths[i][0] / 100.0;    
    return;
  }
  // Case 1: Between specified points.  This is the common case when ramps are active.
  if (rampPos < rampSteps - 1) {
    if (!interpolateT) {
      for (i=0; i<NT; i++) RAMP_START_TEMP[i] = (double)rampHundredths[i][rampPos] / 100.0;    
      return;
    }
    double dayValue = (float)dayMin + t.second()/60.0;
    double frac = (dayValue - rampMinutes[rampPos]) / (rampMinutes[rampPos+1] - rampMinutes[rampPos]);
    for (i=0; i<NT; i++) {
      RAMP_START_TEMP[i] = ((double)rampHundredths[i][rampPos] + frac * ((double)rampHundredths[i][rampPos+1] - (double)rampHundredths[i][rampPos])) / 100.0;
    }
    return;
  }
  // Case 2: past the last step, still on the same day.
  if (rampPos == rampSteps - 1 && dayMin >= rampMinutes[rampPos]) {
    for (i=0; i<NT; i++) RAMP_START_TEMP[i] = (double)rampHundredths[i][rampPos] / 100.0;   
    return;
  }
  printBoth("==ERROR== no current target found at minutes = "); 
  printBoth(dayMin);
  printBoth(" relativeStartTime = ");
  printBoth(relativeStartTime);
  printlnBoth();
}



void ShowRampInfo()
{
  for (i=0; i<NT; i++) {
    Serial.print("The ramp"); Serial.print(i+1); Serial.print(" temp:\t");
    Serial.println (SetPoint[i]);    
  }
}

/*
void ShowRampInfoDummy()
{
  for (i=0; i<NT; i++) {
    Serial.print("New ramp"); Serial.print(i+1); Serial.print(" temp:\t");
    Serial.println (RAMP_START_TEMP[i]);
  }
  // Temporarily, log both sets of ramp data in one line.
  logFile = SD.open("LOG.txt", FILE_WRITE);

  printBoth("TCOMP,");
  printBoth(t.hour(), DEC), printBoth(","), printBoth(t.minute(), DEC), printBoth(","), printBoth(t.second(), DEC), printBoth(",");
  for (i=0; i<NT; i++) {
    printBoth(SetPoint[i]); printBoth(","); printBoth(RAMP_START_TEMP[i]); printBoth(",");
  }
  printlnBoth();
  logFile.close();
}
 */

void PIDinit()
{
  //tell the PID to range between 0 and the full window size.
  //turn the PID on
  for (i=0; i<NT; i++) {
    pids[i].SetMode(AUTOMATIC);
    pids[i].SetOutputLimits(-TPCwindow, TPCwindow); //cooling range = -TPCwindow->0,  heating range = 0->TPCwindow
    pids[i].SetTunings(KP, KI, KD);
  }
}

#ifdef LOGANMODE
/**
 * Get the desired lighting state (on/off) for the current time.
 * Note that we don't use relative time for lighting - it's assumed to be
 * chosen relative to local daylight.
 */
void getLightState() {
  t = rtc.now();  // May be redundant with checkTime() in the main loop.
  unsigned int dayMin = t.minute() + 60 * t.hour();

  // We can be before the specified points, between two, or after all of them.
  // Unlike temperatures, we just want to use the most recent specified state, which
  // may wrap back to the previous midnight.
  // lightPos is 0 at the start of a run, and then points to the latest position used.

  // Delete this:
  // light states
  
  // before first
  // 0700
  // between values
  // 1900
  // after last

  // If the actual time is less than the current start point, we are wrapping past midnight.
  // Point to the last time of day.
  if (lightMinutes[lightPos] > dayMin) lightPos = lightSteps;
  else {
    // Move up if necessary.
    while (lightMinutes[lightPos+1] <= dayMin && lightPos < rampSteps - 1) {
      lightPos++;
    }
  }
  // lightPos now points to the correct lightStatus with no further calculations.
}
#endif // LOGANMODE
